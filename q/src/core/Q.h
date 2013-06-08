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
//#include <list>
#include <map>
#include <thread>
#include <mutex>

#include "../3rd-party/json/json.h"

#include "Job.h"
#include "JobError.h"

using namespace std;

namespace Q
{
    //typedef function<void (const Job*, JobError**)> WorkerDelegate;
    //typedef function<void (const Job*, JobError**)> ObserverDelegate;
    typedef function<void (const Job*, JobError**)> WorkerDelegate;
    typedef function<void (const Job*, JobError**)> ObserverDelegate;

    typedef vector<shared_ptr<const WorkerDelegate>> WorkersList;
    typedef vector<shared_ptr<const ObserverDelegate>> ObserversList;
    typedef map<string, pair<WorkersList, uint>> Workers;
    typedef map<string, ObserversList> Observers;

    typedef map<string, thread*> Monitors;
    
    class Q
    {
        
    public:
        
        Q(const Json::Value& configuration);
        ~Q();
        
        virtual bool connect();
        virtual void disconnect();
        virtual void flush() = 0;
        
        const string post(const string& queue, const string& uid, const string& data, const long run_at);
        bool reschedule(const string& uid, const long run_at);
        bool cancel(const string& uid);
        bool exists(const string& uid);
        
        void worker(const string& queue, const WorkerDelegate* delegate);
        void observer(const string& queue, const ObserverDelegate* delegate);
        
        vector<string> queues();
        
    protected:    
        
        bool active;
        const Json::Value configuration;
        
        void start();
        void stop();
        
        virtual unsigned long size(const string& queue) = 0;
        virtual JobOption peek(const string& queue) = 0;
        virtual JobOption pop_front(const string& queue) = 0;
        virtual void push_back(const string& queue, const Job& job) = 0;
        
        virtual JobOption find_job(const string& uid) = 0;
        virtual JobOption update_job_status(const string& uid, const JobStatus status, const string& status_description) = 0;
        virtual JobOption update_job_run_at(const string& uid, const long run_at) = 0;
        virtual void delete_job(const string& uid) = 0;
        
    private:
        
        static mutex* workers_mutex;
        static mutex* observers_mutex;
        
        static void monitor_queue(Q* q, const string& queue);
        
        Workers workers;
        
        Observers observers;
        
        Monitors monitors;
            
        void verify_queue_monitor(const string& queue);
        
        //JobOption handle_job_result(const string& queue, const Job& job, const JobError* error);    
    };

    struct q_exception : public exception
    {
        string _description;
        q_exception(string description) : _description(description) {}
        const char* what() const throw() { return _description.c_str(); }
    };
    
}

#endif

