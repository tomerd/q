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

class Job
{
    
private:
    string _uid;
    string _data;
    long _at;
    long _timestamp;
    
    void init(const string& uid, const string& data, const long at);
    string generate_job_uid();    
    
public:
    
    Job(const string& uid, const string& data, const long at);
    Job(const string& data, const long at=0);    
    ~Job();
    
    string const& uid() const;
    string const& data() const;
    long at() const;
    long timestamp() const;
};

#endif /* defined(__q__Job__) */
