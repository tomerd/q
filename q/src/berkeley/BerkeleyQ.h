//
//  BerkeleyQ.h
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__BerkeleyQ__
#define __q__BerkeleyQ__

#include <iostream>
#include <vector>

#include "../core/Q.h"
// FIXME: use better linking & header inclusion stratgey
#include "db_cxx.h"

using namespace std;

namespace Q
{
    class BerkeleyQ : public Q
    {
        
    public:
        
        BerkeleyQ(const Json::Value& configuration);
        ~BerkeleyQ();
        
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
        
        Db* db;
        uint locker_id;
        
        DbLock acquire_lock(const string& lock_name, const db_lockmode_t lock_mode);
        void release_lock(DbLock* lock);
        
        unsigned long load_queue_size(const string& queue_name);
        void save_queue_size(const string& queue_name, unsigned long size);
        
        string load_queue_raw(const string& queue_name);
        void save_queue_raw(const string& queue_name, const string& data);
        
        vector<string> load_queue_vector(const string& queue_name);
        void save_queue_vector(const string& queue_name, const vector<string> data);
        
        JobOption load_job_record(const string& uid);
        void save_job_record(const Job& job);
        void delete_job_record(const string& uid);
        
        string load_record(const string& key);
        void save_record(const string& key, const string& value, const uint flags);
        void delete_record(const string& key, const uint flags);
    };
}

#endif /* defined(__q__BerkeleyQ__) */
