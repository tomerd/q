//
//  KyotoCabinetQ.h
//  q
//
//  Created by Tomer Doron on 7/17/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__KyotoCabinetQ__
#define __q__KyotoCabinetQ__

#include <iostream>

#include "../core/Q.h"
// FIXME: use better linking & header inclusion stratgey
#include "../3rd-party/kyotocabinet/kcpolydb.h"

using namespace std;
using namespace kyotocabinet;

namespace Q
{
    typedef struct KyotoCabinetConfig
    {        
        KyotoCabinetConfig()
        {}
        
    } KyotoCabinetConfig;
    
    class KyotoCabinetQ : public Q
    {
        
    public:
        
        KyotoCabinetQ(const Json::Value& configuration);
        ~KyotoCabinetQ();
        
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
                
        KyotoCabinetConfig config;
        PolyDB db;
                
    };
}

#endif /* defined(__q__KyotoCabinetQ__) */
