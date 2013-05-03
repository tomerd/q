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

#include "Q.h"
// FIXME: use better linking & header inclusion stratgey
#include "hiredis.h"

using namespace std;

struct RedisConfig;

class RedisQ : public Q
{
    
public:
    
    RedisQ(const Json::Value& configuration);
    ~RedisQ();
    
    bool connect();
    void disconnect();
    
protected:
    
    unsigned long size(const string& queue);
    Job* peek(const string& queue);
    Job* take(const string& queue);
    void push(const string& queue, Job* job);
    //Job* find(const string& queue, const string& uid);
    //void remove(const string& queue, const string& uid);
    
    Job* find_job(const string& uid);
    Job* update_job_status(const string& uid, const JobStatus status, const string& status_description);
    void delete_job(const string& uid);
    
private:
    
    static mutex* redis_mutex;
    
    RedisConfig* config;
    redisContext* context;
    
    redisReply* runRedisCommand(const char* command, ...);    
    //void handle_reply_error();
    
    string build_queue_key(const string& queue_name);
    string build_queue_size_key(const string& queue_name);
    string build_job_key(const string& job_uid);
    
};

#endif /* defined(__q__RedisQ__) */
