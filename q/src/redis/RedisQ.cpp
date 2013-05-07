//
//  RedisQ.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "RedisQ.h"

#include "json.h"
#include "JobCodec.h"
#include "Logger.h"

static string build_key(const char* format, ...);
static RedisConfig parse_config(const Json::Value& configuration);

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
        if (NULL == context) throw q_exception("failed creating hiredis context");
        if (context->err) throw q_exception(context->errstr);
        
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

void RedisQ::flush()
{
    if (!this->active) return;
    
    vector<string> keys;
    
    string queue_size_key = build_queue_size_key("*");
    redisReply* reply1 = runRedisCommand("KEYS %s", queue_size_key.c_str());
    if (NULL == reply1) return;
    if (REDIS_REPLY_ARRAY != reply1->type)
    {
        q_error("redis KEYS on [%s] failed. invalid reply type", queue_size_key.c_str());
        freeReplyObject(reply1);
        return;
    }
    for (uint index=0; index < reply1->elements; index++)
    {
        keys.push_back(((redisReply*)reply1->element[index])->str);
    }
    
    string queue_key = build_queue_key("*");
    redisReply* reply2 = runRedisCommand("KEYS %s", queue_key.c_str());
    if (NULL == reply2) return;
    if (REDIS_REPLY_ARRAY != reply2->type)
    {
        q_error("redis KEYS on [%s] failed. invalid reply type", queue_key.c_str());
        freeReplyObject(reply2);
        return;
    }
    for (uint index=0; index < reply2->elements; index++)
    {
        keys.push_back(((redisReply*)reply2->element[index])->str);
    }
    
    string job_key = build_job_key("*");
    redisReply* reply3 = runRedisCommand("KEYS %s", job_key.c_str());
    if (NULL == reply3) return;
    if (REDIS_REPLY_ARRAY != reply3->type)
    {
        q_error("redis KEYS on [%s] failed. invalid reply type", job_key.c_str());
        freeReplyObject(reply3);
        return;
    }
    for (uint index=0; index < reply3->elements; index++)
    {
        keys.push_back(((redisReply*)reply3->element[index])->str);
    }
    
    for (vector<string>::iterator it = keys.begin(); it != keys.end(); it++)
    {
        redisReply* reply4 = runRedisCommand("DEL %s", (*it).c_str());
        if (NULL == reply4) return;
        if (REDIS_REPLY_INTEGER != reply4->type)
        {
            q_error("redis DEL on [%s] failed. invalid reply type", job_key.c_str());
        }
        freeReplyObject(reply4);
    }
    
    freeReplyObject(reply1);
    freeReplyObject(reply2);
    freeReplyObject(reply3);    
}

#pragma mark - protected

unsigned long RedisQ::size(const string& queue_name)
{
    if (!this->active) return -1;
    
    unsigned long size = 0;
    string key = build_queue_size_key(queue_name);
    redisReply* reply = runRedisCommand("GET %s", key.c_str());
    if (NULL == reply) return 0;
    if (REDIS_REPLY_STRING == reply->type)
    {
        char* end;
        size = strtol(reply->str, &end, 10);
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
    return size;
}

JobOption RedisQ::peek(const string& queue_name)
{
    if (!this->active) return JobOption();
    
    JobOption job;
    string key = build_queue_key(queue_name);
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
    
    string key = build_queue_key(queue_name);
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
        
    string queue_size_key = build_queue_size_key(queue_name);
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
    
    string job_key = build_job_key(job.uid());
    string job_json = JobCodec::encode(job);
    redisReply* reply1 = runRedisCommand("SET %s %s", job_key.c_str(), job_json.c_str());
    if (NULL == reply1) return;    
    if (REDIS_REPLY_STATUS != reply1->type)
    {
        q_error("redis SET on [%s] failed. invalid reply type", job_key.c_str());
        return freeReplyObject(reply1);
    }
    
    string queue_key = build_queue_key(queue_name);
    redisReply* reply2 = runRedisCommand("RPUSH %s %s", queue_key.c_str(), job.uid().c_str());
    if (NULL == reply2) return;
    if (REDIS_REPLY_INTEGER != reply2->type)
    {
        q_error("redis RPUSH on [%s] failed. invalid reply type", queue_key.c_str());
        return freeReplyObject(reply2);
    }
    
    string queue_size_key = build_queue_size_key(queue_name);
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
    string key = build_job_key(uid);
    redisReply* reply = runRedisCommand("GET %s", key.c_str());
    if (NULL == reply) return JobOption();
    if (REDIS_REPLY_STRING == reply->type)
    {
        job = JobCodec::decode(reply->str);
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
    string key = build_job_key(updated_job.uid());
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
    string key = build_job_key(updated_job.uid());
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
    
    string job_key = build_job_key(uid);
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
        string queue_key = build_queue_key(queue_name);
        redisReply* reply2 = runRedisCommand("LREM %s 0 %s", queue_key.c_str(), uid.c_str());
        if (NULL == reply2) break;
        if (REDIS_REPLY_INTEGER != reply2->type)
        {
            q_error("redis LREM on [%s] failed. invalid reply type", queue_key.c_str());
        }
        
        string queue_size_key = build_queue_size_key(queue_name);
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

string RedisQ::build_queue_key(const string& queue_name)
{
    return build_key("%s:q:%s", this->config.prefix.c_str(), queue_name.c_str());
}

string RedisQ::build_queue_size_key(const string& queue_name)
{
    return build_key("%s:q:%s:s", this->config.prefix.c_str(),queue_name.c_str());
}

string RedisQ::build_job_key(const string& job_uid)
{
    return build_key("%s:j:%s", this->config.prefix.c_str(), job_uid.c_str());
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