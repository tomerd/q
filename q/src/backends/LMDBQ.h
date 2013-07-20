//
//  LMDBQ.h
//  q
//
//  Created by Tomer Doron on 7/18/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__lmdbQ__
#define __q__lmdbQ__

#include <iostream>

#include "../core/Q.h"
// FIXME: use better linking & header inclusion stratgey
#include "../3rd-party/mdb/libraries/liblmdb/lmdb.h"

using namespace std;

namespace Q
{
    typedef struct LMDBConfig
    {
        LMDBConfig()
        {}
        
    } LMDBConfig;
    
    class LMDBQ : public Q
    {
        
    public:
        
        LMDBQ(const Json::Value& configuration);
        ~LMDBQ();
        
        bool connect();
        void disconnect();
        
        void clear(const string& queue);
        void drop();        
        
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
                
        LMDBConfig config;
        MDB_env* env;
        MDB_dbi db;
        
        unsigned long load_queue_size(MDB_txn* txn, MDB_dbi dbi, const string& queue_name);
        void save_queue_size(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, unsigned long size);
        
        string load_queue_raw(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, size_t size_hint);
        void save_queue_raw(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, const string& data);
        
        vector<string> load_queue_vector(MDB_txn* txn, MDB_dbi dbi, const string& queue_name);
        void save_queue_vector(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, const vector<string> data);
        
        JobOption load_job_record(MDB_txn* txn, MDB_dbi dbi, const string& uid);
        void save_job_record(MDB_txn* txn, MDB_dbi dbi, const Job& job);
        void delete_job_record(MDB_txn* txn, MDB_dbi dbi, const string& uid);
        
        string load_string_record(MDB_txn* txn, MDB_dbi dbi, const string& key, size_t size_hint=0);
        void save_string_record(MDB_txn* txn,MDB_dbi dbi,  const string& key, const string& value, const uint flags);
        void delete_record(MDB_txn* txn, MDB_dbi dbi, const string& key);
    };
}

#endif /* defined(__q__lmdbQ__) */
