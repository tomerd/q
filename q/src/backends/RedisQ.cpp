//
//  RedisQ.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <mutex>
#include <cstdarg>

#include "RedisQ.h"

#include "../3rd-party/json/json.h"
#include "../core/JobCodec.h"
#include "../core/Logger.h"

namespace Q
{
    static RedisConfig parse_config(const Json::Value& configuration);
    
    static string build_queue_key(const string& prefix, const string& queue_name);
    static string build_queue_size_key(const string& prefix, const string& queue_name);
    static string build_job_key(const string& prefix, const string& job_uid);
    static string build_key(const char* format, ...);

    mutex* RedisQ::redis_mutex = new mutex();

    RedisQ::RedisQ(const Json::Value& configuration) : Q::Q(configuration), config(parse_config(configuration))
    {
        context = NULL;
    }

    RedisQ::~RedisQ()
    {
    }

    bool RedisQ::connect()
    {
        if (NULL != context) return true;
        
        try
        {
            context = redisConnect(this->config.host.c_str(), this->config.port);
            if (NULL == context) throw QException("failed creating hiredis context");
            if (context->err) throw QException(context->errstr);
            
            start();
        }
        catch (exception& e)
        {
            context = NULL;
            q_error("failed connecting to redis. %s", e.what());
        }
        catch (...)
        {
            context = NULL;
            q_error("failed connecting to redis. unknown error");
        }
        
        return (NULL != context);
    }

    void RedisQ::disconnect()
    {
        stop();
        
        if (NULL == context) return;
        
        try
        {
            redisFree(context);
            context = NULL;
        }
        catch (exception& e)
        {
            q_error("failed disconnecting from redis. %s", e.what());
        }
        catch (...)
        {
            q_error("failed disconnecting from redis. unknown error");
        }
    }

    // FIXME: implement this
    void RedisQ::clear(const string& queue)
    {
    }
    
    void RedisQ::drop()
    {
        if (!this->active) return;
     
        redisReply* reply = runRedisCommand("FLUSHDB");
        if (NULL == reply) return;
        if (REDIS_REPLY_STATUS != reply->type)
        {
            q_error("redis FLUSHDB failed. invalid reply type");
        }
        
        freeReplyObject(reply);        
    }

    #pragma mark - protected

    unsigned long RedisQ::size(const string& queue_name)
    {
        if (!this->active) return -1;
        
        unsigned long result = 0;
        string key = build_queue_size_key(this->config.prefix, queue_name);
        redisReply* reply = runRedisCommand("GET %s", key.c_str());
        if (NULL == reply) return 0;
        if (REDIS_REPLY_STRING == reply->type)
        {
            char* end;
            result = strtol(reply->str, &end, 10);
            if (end == reply->str || *end != '\0' || errno == ERANGE)
            {
                q_error("redis GET on [%s] failed. invalid value", key.c_str());
            }
        }
        else if (REDIS_REPLY_NIL != reply->type)
        {
            q_error("redis GET on [%s] failed. invalid reply type", key.c_str());
        }
        freeReplyObject(reply);
        return result;
    }

    JobOption RedisQ::peek(const string& queue_name)
    {
        if (!this->active) return JobOption();
        
        JobOption job;
        string key = build_queue_key(this->config.prefix, queue_name);
        redisReply* reply = runRedisCommand("LRANGE %s 0 0", key.c_str());
        if (NULL == reply) return JobOption();
        if (REDIS_REPLY_STRING == reply->type)
        {
            string uid = reply->str;
            job = find_job(uid);
        }
        else
        {
            q_error("redis LRANGE on [%s] failed. invalid reply type", key.c_str());
        }
        freeReplyObject(reply);
        return job;
    }

    JobOption RedisQ::pop_front(const string& queue_name)
    {
        if (!this->active) return JobOption();
        
        string key = build_queue_key(this->config.prefix, queue_name);
        redisReply* reply1 = runRedisCommand("LPOP %s", key.c_str());
        if (NULL == reply1) return JobOption();
        if (REDIS_REPLY_NIL == reply1->type)
        {
            freeReplyObject(reply1);
            return JobOption();
        }
        else if (REDIS_REPLY_STRING != reply1->type)
        {
            q_error("redis LPOP on [%s] failed. invalid reply type", key.c_str());
            freeReplyObject(reply1);
            return JobOption();
        }
            
        string queue_size_key = build_queue_size_key(this->config.prefix, queue_name);
        redisReply* reply2 = runRedisCommand("DECR %s", queue_size_key.c_str());
        if (NULL == reply2) return JobOption();
        if (REDIS_REPLY_INTEGER != reply2->type)
        {
            q_error("redis DECR on [%s] failed. invalid reply type", queue_size_key.c_str());
        }

        string uid = reply1->str;
        JobOption job = find_job(uid);
        
        freeReplyObject(reply1);
        freeReplyObject(reply2);
        return job;
    }

    void RedisQ::push_back(const string& queue_name, const Job& job)
    {
        if (!this->active) return;
        
        string job_key = build_job_key(this->config.prefix, job.uid());
        string job_json = JobCodec::encode(job);
        redisReply* reply1 = runRedisCommand("SET %s %s", job_key.c_str(), job_json.c_str());
        if (NULL == reply1) return;    
        if (REDIS_REPLY_STATUS != reply1->type)
        {
            q_error("redis SET on [%s] failed. invalid reply type", job_key.c_str());
            return freeReplyObject(reply1);
        }
        
        string queue_key = build_queue_key(this->config.prefix, queue_name);
        redisReply* reply2 = runRedisCommand("RPUSH %s %s", queue_key.c_str(), job.uid().c_str());
        if (NULL == reply2) return;
        if (REDIS_REPLY_INTEGER != reply2->type)
        {
            q_error("redis RPUSH on [%s] failed. invalid reply type", queue_key.c_str());
            return freeReplyObject(reply2);
        }
        
        string queue_size_key = build_queue_size_key(this->config.prefix, queue_name);
        redisReply* reply3 = runRedisCommand("INCR %s", queue_size_key.c_str());
        if (NULL == reply3) return;    
        if (REDIS_REPLY_INTEGER != reply3->type)
        {
            q_error("redis INCR on [%s] failed. invalid reply type", queue_size_key.c_str());
        }    

        freeReplyObject(reply1);
        freeReplyObject(reply2);
        freeReplyObject(reply3);
    }

    JobOption RedisQ::find_job(const string& uid)
    {
        if (!this->active) return JobOption();
        
        JobOption job;
        string key = build_job_key(this->config.prefix, uid);
        redisReply* reply = runRedisCommand("GET %s", key.c_str());
        if (NULL == reply) return JobOption();
        if (REDIS_REPLY_STRING == reply->type)
        {
            job = JobCodec::decode(reply->str);
        }
        else if (REDIS_REPLY_NIL == reply->type)
        {
            // do nothing
        }
        else
        {
            q_error("redis GET on [%s] failed. invalid reply type", key.c_str());
        }
        freeReplyObject(reply);
        return job;
    }

    JobOption RedisQ::update_job_status(const string& uid, const JobStatus status, const string& status_description)
    {
        if (!this->active) return JobOption();
        
        JobOption job = find_job(uid);
        if (job.empty()) return JobOption();
        Job updated_job = job.get().withStatus(status, status_description);
        string key = build_job_key(this->config.prefix, updated_job.uid());
        string job_json = JobCodec::encode(updated_job);
        redisReply* reply = runRedisCommand("SET %s %s", key.c_str(), job_json.c_str());
        if (NULL == reply) return JobOption();
        if (REDIS_REPLY_STATUS != reply->type)
        {
            q_error("redis SET on [%s] failed. invalid reply type", key.c_str());
        }
        freeReplyObject(reply);
        return JobOption(updated_job);
    }

    JobOption RedisQ::update_job_run_at(const string& uid, const long run_at)
    {
        if (!this->active) return JobOption();
        
        JobOption job = find_job(uid);
        if (job.empty()) return JobOption();
        Job updated_job = job.get().withRunAt(run_at);
        string key = build_job_key(this->config.prefix, updated_job.uid());
        string job_json = JobCodec::encode(updated_job);
        redisReply* reply = runRedisCommand("SET %s %s", key.c_str(), job_json.c_str());
        if (NULL == reply) return JobOption();
        if (REDIS_REPLY_STATUS != reply->type)
        {
            q_error("redis SET on [%s] failed. invalid reply type", key.c_str());
        }
        freeReplyObject(reply);
        return JobOption(updated_job);
    }

    void RedisQ::delete_job(const string& uid)
    {
        if (!this->active) return;
        
        string job_key = build_job_key(this->config.prefix, uid);
        redisReply* reply1 = runRedisCommand("DEL %s", job_key.c_str());
        if (NULL == reply1) return;
        if (REDIS_REPLY_INTEGER != reply1->type)
        {
            q_error("redis DEL on [%s] failed. invalid reply type", job_key.c_str());
        }
        
        vector<string> all_queues = this->queues();
        for (vector<string>::iterator queue_it = all_queues.begin(); queue_it != all_queues.end(); queue_it++)
        {
            string queue_name = *queue_it;        
            string queue_key = build_queue_key(this->config.prefix, queue_name);
            redisReply* reply2 = runRedisCommand("LREM %s 0 %s", queue_key.c_str(), uid.c_str());
            if (NULL == reply2) break;
            if (REDIS_REPLY_INTEGER != reply2->type)
            {
                q_error("redis LREM on [%s] failed. invalid reply type", queue_key.c_str());
            }
            
            string queue_size_key = build_queue_size_key(this->config.prefix, queue_name);
            redisReply* reply3 = runRedisCommand("DECR %s", queue_size_key.c_str());
            if (NULL == reply3) break;
            if (REDIS_REPLY_INTEGER != reply3->type)
            {
                q_error("redis DECR on [%s] failed. invalid reply type", queue_size_key.c_str());
            }
            
            freeReplyObject(reply2);
            freeReplyObject(reply3);
        }
        
        freeReplyObject(reply1);
    }

    #pragma mark - private

    redisReply* RedisQ::runRedisCommand(const char* command, ...)
    {
        va_list args;
        va_start(args, command);
        redis_mutex->lock();
        redisReply* reply = (redisReply*)redisvCommand(this->context, command, args);
        redis_mutex->unlock();
        if (NULL == reply)
        {
            q_error("redis received bad (NULL) reply");
        }
        va_end(args);
        return reply;
    }

    string build_queue_key(const string& prefix, const string& queue_name)
    {
        return build_key("%s:q:%s", prefix.c_str(), queue_name.c_str());
    }

    string build_queue_size_key(const string& prefix, const string& queue_name)
    {
        return build_key("%s:q:%s:s", prefix.c_str(),queue_name.c_str());
    }

    string build_job_key(const string& prefix, const string& job_uid)
    {
        return build_key("%s:j:%s", prefix.c_str(), job_uid.c_str());
    }

    string build_key(const char* format, ...)
    {
        char key[1024];
        va_list args;
        va_start(args, format);
        int written = vsnprintf(key, 1024, format, args);
        key[written] = '\0';
        va_end(args);
        return key;
    }

    RedisConfig parse_config(const Json::Value& configuration)
    {
        string host = configuration.get("host", "").asString();
        int port = configuration.get("port", 6379).asInt();
        string prefix = configuration.get("namespace", "").asString();
        prefix = prefix.empty() ? "q" : string("q:").append(prefix);
        return RedisConfig(host, port, prefix);
    }
}