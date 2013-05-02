//
//  q.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

//#ifnvirtual void q_q_h
//#virtual voidine q_q_h

#ifndef q_q_h
#define q_q_h

#include <iostream>
#include <vector>
#include <map>
#include <thread>
#include <thread>

#include "Job.h"
#include "JobError.h"

using namespace std;

//typedef function<void (Job*, JobError**)> WorkerDelegate;
//typedef function<void (Job*, JobError**)> ObserverDelegate;
typedef shared_ptr<function<void (Job*, JobError**)>> WorkerDelegate;
typedef shared_ptr<function<void (Job*, JobError**)>> ObserverDelegate;

typedef vector<WorkerDelegate> WorkersList;
typedef vector<ObserverDelegate> ObserversList;
typedef map<string, pair<WorkersList, uint>> Workers;
typedef map<string, ObserversList> Observers;

typedef map<string, thread*> Monitors;

class Q
{
    
public:
    
    Q();
    ~Q();
    
    virtual void start();
    virtual void stop();
    
    Job* post(const string& queue, const string& data, const long at);
    void worker(const string& queue, WorkerDelegate delegate);
    void observer(const string& queue, ObserverDelegate delegate);

protected:
    
    virtual unsigned long size(const string& queue) = 0;
    virtual Job* peek(const string& queue) = 0;
    virtual Job* take(const string& queue) = 0;
    virtual void push(const string& queue, Job* job) = 0;
    virtual Job* find(const string& queue, const string& uid) = 0;
    virtual void erase(const string& queue, const string& uid) = 0;
    
private:
    
    static mutex* workers_mutex;
    static mutex* observers_mutex;
    
    Workers workers;
    
    Observers observers;
    
    Monitors monitors;
    
    bool started;
    
    static void monitor_queue(Q* q, const string& queue);
    
    void verify_queue_monitor(const string& queue);
    
    void handle_job_result(const string& queue, const Job* job, const JobError* error);    
};

#endif

