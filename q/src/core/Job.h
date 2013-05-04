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

typedef enum JobStatus
{
	JSUnknown, JSPending, JSDelayed, JSActive, JSComplete, JSFailed
} JobStatus;

class Job
{
    
public:
    
    Job(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long at, unsigned long timestamp);
    Job(const string& data, const JobStatus status=JSUnknown, const unsigned long at=0);
    Job(const Job& other);
    ~Job();
    
    const Job withStatus(JobStatus status, const string& status_description) const;
    
    const string& uid() const;
    const string& data() const;
    JobStatus status() const;
    const string& status_description() const;
    long at() const;
    long timestamp() const;
    
private:
    
    string _uid;
    string _data;
    JobStatus _status;
    string _status_description;
    unsigned long _at;
    unsigned long _timestamp;
    
    void init(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long at, unsigned long timestamp);
    
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

#endif /* defined(__q__Job__) */
