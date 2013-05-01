//
//  Job.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <uuid/uuid.h>

#include "Job.h"

void Job::init(const string& uid, const string& data, const long at)
{
    _uid = uid;
    _data = data;
    _at = at;
    time(&_timestamp);
}

Job::Job(const string& uid, const string& data, const long at)
{
    init(uid, data, at);
}

Job::Job(const string& data, const long at)
{
    string uid = generate_job_uid();
    init(uid, data, at);
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

string Job::generate_job_uid()
{
    uuid_t uuid;
    uuid_generate_random(uuid);
    char s[37];
    uuid_unparse(uuid, s);
    return string(s);
}