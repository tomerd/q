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

long get_current_time();
string generate_job_uid();

void Job::init(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long at, const unsigned long timestamp)
{
    _uid = uid;
    _data = data;
    _at = at;
    _status = status;
    _status_description = status_description;
    _timestamp = timestamp > 0 ? timestamp : get_current_time();
}

Job::Job(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long at, const unsigned long timestamp)
{
    init(uid, data, status, status_description, at, timestamp);
}

Job::Job(const string& data, const JobStatus status, const unsigned long at)
{
    string uid = generate_job_uid();
    init(uid, data, status, "", at, 0);
}

Job::~Job()
{
}

Job* Job::withStatus(JobStatus status, const string& status_description)
{
    return new Job(this->uid(), this->data(), status, status_description, this->at(), this->timestamp());
}

string const& Job::uid() const
{
    return _uid;
}

string const& Job::data() const
{
    return _data;
}

JobStatus Job::status() const
{
    return _status;
}

string const& Job::status_description() const
{
    return  _status_description;
}

long Job::at() const
{
    return _at;
}

long Job::timestamp() const
{
    return _timestamp;
}

long get_current_time()
{
    long now = 0;
    time(&now);
    return now;
}

string generate_job_uid()
{
    uuid_t uuid ;
    uuid_generate_random( uuid ) ;
    char temp[37] ;
    uuid_unparse( uuid, temp ) ;
    return temp;
}
