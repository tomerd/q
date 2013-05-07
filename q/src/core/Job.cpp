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

static long get_current_time();
static string generate_job_uid();

void Job::init(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long run_at, const unsigned long timestamp)
{
    _uid = uid.empty() ? generate_job_uid() : uid;
    _data = data;
    _run_at = run_at;
    _status = status;
    _status_description = status_description;
    _timestamp = timestamp > 0 ? timestamp : get_current_time();
}

Job::Job(const string& uid, const string& data, const JobStatus status, const string& status_description, const unsigned long at, const unsigned long timestamp)
{
    init(uid, data, status, status_description, at, timestamp);
}

Job::Job(const string& uid, const string& data, const JobStatus status, const unsigned long run_at)
{    
    init(uid, data, status, "", run_at, 0);
}

Job::Job(const Job& other)
{
    init(other.uid(), other.data(), other.status(), other.status_description(), other.run_at(), other.timestamp());
}

Job::~Job()
{
}

const Job Job::withStatus(JobStatus status, const string& status_description) const
{
    return Job(this->uid(), this->data(), status, status_description, this->run_at(), this->timestamp());
}

const Job Job::withRunAt(const unsigned long run_at) const
{
    return Job(this->uid(), this->data(), this->status(), this->status_description(), run_at, this->timestamp());
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

long Job::run_at() const
{
    return _run_at;
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

# pragma mark - job option

JobOption::JobOption() : _job(Job("", "", JSUnknown, "", 0, 0)), _empty(true)
{
}

JobOption::JobOption(const Job& job) : _job(job), _empty(false)
{
}

bool JobOption::empty()
{
    return _empty;
}

const Job& JobOption::get() const
{
    return _job;
}


