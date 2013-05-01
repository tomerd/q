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
#include <thread>

#include "Q.h"

typedef vector<Job*> JobsList;
typedef map<string, JobsList*> Queues;

typedef vector<WorkerDelegate> WorkersList;
typedef vector<ObserverDelegate> ObserversList;
typedef map<string, pair<WorkersList*, uint>> Workers;
typedef map<string, ObserversList*> Observers;

typedef map<string, thread*> Monitors;

class TransientQ : public Q
{
    
private:
    
    static mutex* queues_mutex;
    static mutex* workers_mutex;
    static mutex* observers_mutex;
    
    static Queues queues;
    
    Workers workers;
    
    Observers observers;
    
    Monitors monitors;
    
    bool started;
        
    static void monitor_queue(TransientQ* q, const string& queue);
    
    void verify_queue_monitor(const string& queue);
    
    void handle_job_result(const string& queue, const Job* job, const JobError* error);    
    
public:
    
    TransientQ();
    ~TransientQ();
    
    void start();
    void stop();
    
    Job* post(const string& queue, const string& data, const long at);
    
    void worker(const string& queue, WorkerDelegate delegate);
    
    void observer(const string& queue, ObserverDelegate delegate);
    
    Job* peek(const string& queue);    
};

#endif /* defined(__q__TransientQ__) */
