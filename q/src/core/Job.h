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
    ~Job();
    
    Job* withStatus(JobStatus status, const string& status_description);
    
    string const& uid() const;
    string const& data() const;
    JobStatus status() const;
    string const& status_description() const;
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

#endif /* defined(__q__Job__) */
