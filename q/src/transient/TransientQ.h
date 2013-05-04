//
//  TransientQ.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__TransientQ__
#define __q__TransientQ__

#include <iostream>
#include <map>
//#include <list>
#include <vector>

#include "Q.h"

typedef map<string, Job> Jobs;
//typedef map<string, JobsList::iterator> JobsIndex;
//typedef map<string, pair<JobsList*, JobsIndex*>> Queues;
//typedef map<string, Jobs::iterator> JobsIndex;
typedef map<string, vector<Jobs::iterator>> Queues;

class TransientQ : public Q
{
            
public:
    
    TransientQ(const Json::Value& configuration);
    ~TransientQ();
    
    void flush();
    
protected:
    
    unsigned long size(const string& queue);
    JobOption peek(const string& queue);
    JobOption take(const string& queue);
    void push(const string& queue, const Job& job);
    //Job* find(const string& queue, const string& uid);
    //void remove(const string& queue, const string& uid);
    
    JobOption find_job(const string& uid);
    JobOption update_job_status(const string& uid, const JobStatus status, const string& status_description);
    void delete_job(const string& uid);
    
private:
    
    static Queues queues;
    static mutex* queues_mutex;
    
    static Jobs jobs;
    static mutex* jobs_mutex;
    
};

#endif /* defined(__q__TransientQ__) */
