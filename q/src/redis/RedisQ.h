//
//  RedisQ.h
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__RedisQ__
#define __q__RedisQ__

#include <iostream>
#include <mutex>

#include "../core/Q.h"
// FIXME: use better linking & header inclusion stratgey
#include "../3rd-party/hiredis/hiredis.h"

using namespace std;

namespace Q
{
    typedef struct RedisConfig
    {
        string host;
        int port;
        string prefix;
        
        RedisConfig(string host, int port, string prefix) : host(host), port(port), prefix(prefix)
        {}
        
    } RedisConfig;

    class RedisQ : public Q
    {
        
    public:
        
        RedisQ(const Json::Value& configuration);
        ~RedisQ();
        
        bool connect();
        void disconnect();
        void flush();
        
    protected:
        
        unsigned long size(const string& queue);
        JobOption peek(const string& queue);
        JobOption pop_front(const string& queue);
        void push_back(const string& queue, const Job& job);
        
        JobOption find_job(const string& uid);
        JobOption update_job_status(const string& uid, const JobStatus status, const string& status_description);
        JobOption update_job_run_at(const string& uid, const long run_at);
        void delete_job(const string& uid);
        
    private:
        
        static mutex* redis_mutex;
        
        RedisConfig config;
        redisContext* context;
        
        redisReply* runRedisCommand(const char* command, ...);    
        //void handle_reply_error();
        
        string build_queue_key(const string& queue_name);
        string build_queue_size_key(const string& queue_name);
        string build_job_key(const string& job_uid);
        
    };
}

#endif /* defined(__q__RedisQ__) */
