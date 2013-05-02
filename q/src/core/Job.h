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
    unsigned long _at;
    unsigned long _timestamp;
    
    void init(const string& uid, const string& data, const unsigned long at, unsigned long timestamp);
    
public:
    
    Job(const string& uid, const string& data, const unsigned long at, unsigned long timestamp);
    Job(const string& data, const unsigned long at=0);    
    ~Job();
    
    string const& uid() const;
    string const& data() const;
    long at() const;
    long timestamp() const;
};

#endif /* defined(__q__Job__) */
