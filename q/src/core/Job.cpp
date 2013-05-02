//
//  Job.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <uuid/uuid.h>
#include <vector>

#include "Job.h"

string generate_job_uid();

void Job::init(const string& uid, const string& data, const unsigned long at, const unsigned long timestamp)
{
    long now = 0;
    time(&now);
    
    _uid = uid;
    _data = data;
    _at = at;
    _timestamp = timestamp > 0 ? timestamp : now;
}

Job::Job(const string& uid, const string& data, const unsigned long at, const unsigned long timestamp)
{
    init(uid, data, at, timestamp);
}

Job::Job(const string& data, const unsigned long at)
{
    string uid = generate_job_uid();
    init(uid, data, at, 0);
}

Job::~Job()
{
}

string const& Job::uid() const
{
    return _uid;
}

string const& Job::data() const
{
    return _data;
}

long Job::at() const
{
    return _at;
}

long Job::timestamp() const
{
    return _timestamp;
}

string generate_job_uid()
{
    uuid_t uuid ;
    uuid_generate_random( uuid ) ;
    char temp[37] ;
    uuid_unparse( uuid, temp ) ;
    return temp;
}
