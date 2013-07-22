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
        bool transient;
        string path;
        
        LMDBConfig(bool transient, string path) : transient(transient), path(path)
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
    };
}

#endif /* defined(__q__lmdbQ__) */
