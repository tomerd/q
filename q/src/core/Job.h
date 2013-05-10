//
//  Job.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__Job__
#define __q__Job__

#include <iostream>

using namespace std;

namespace Q
{
    typedef enum JobStatus
    {
        JSUnknown, JSPending, JSDelayed, JSActive, JSComplete, JSFailed
    } JobStatus;

    class Job
    {
        
    public:
        
        Job(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long run_at, unsigned long timestamp);
        Job(const string& xuid, const string& data, const JobStatus status=JSUnknown, const unsigned long run_at=0);
        Job(const Job& other);
        ~Job();
        
        const Job withStatus(JobStatus status, const string& status_description) const;
        const Job withRunAt(const unsigned long run_at) const;
        
        const string& uid() const;
        const string& data() const;
        JobStatus status() const;
        const string& status_description() const;
        long run_at() const;
        long timestamp() const;
        
    private:
        
        string _uid;
        string _data;
        JobStatus _status;
        string _status_description;
        unsigned long _run_at;
        unsigned long _timestamp;
        
        void init(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long run_at, unsigned long timestamp);
        
    };

    class JobOption
    {
        
    public:
        
        JobOption();
        JobOption(const Job& job);
        bool empty();
        const Job& get() const;
        
    private:
        
        bool _empty;
        Job _job;
    };
}

#endif /* defined(__q__Job__) */
