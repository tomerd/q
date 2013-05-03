//
//  TransientQ.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "TransientQ.h"

#include <algorithm>

Queues TransientQ::queues = Queues();
mutex* TransientQ::queues_mutex = new mutex();

Jobs TransientQ::jobs = Jobs();
mutex* TransientQ::jobs_mutex = new mutex();

TransientQ::TransientQ()
{
}

TransientQ::~TransientQ()
{
}

unsigned long TransientQ::size(const string& queue_name)
{
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    unsigned long size = (queues.end() == queue_it) ? 0 : queue_it->second.size();
    queues_mutex->unlock();
    return size;
}

Job* TransientQ::peek(const string& queue_name)
{
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    Jobs::iterator job_it = queues.end() == queue_it || queue_it->second.empty() ? jobs.end() : *queue_it->second.begin();
    Job* job = (jobs.end() == job_it) ? NULL : job_it->second;
    queues_mutex->unlock();
    return job;
}

Job* TransientQ::take(const string& queue_name)
{
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    Jobs::iterator job_it = queues.end() == queue_it || queue_it->second.empty() ? jobs.end() : *queue_it->second.begin();
    Job* job = (jobs.end() == job_it) ? NULL : job_it->second;
    queue_it->second.erase(queue_it->second.begin());
    queues_mutex->unlock();    
    return job;    
}

void TransientQ::push(const string& queue_name, Job* job)
{
    jobs_mutex->lock();
    Jobs::iterator job_it = jobs.insert(jobs.end(), pair<string, Job*>(job->uid(), job));
    jobs_mutex->unlock();
    
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    vector<Jobs::iterator>* jobs_index;
    if (queues.end() == queue_it)
    {
        jobs_index = &queues.insert(queue_it, pair<string, vector<Jobs::iterator>>(queue_name, vector<Jobs::iterator>()))->second;
    }
    else
    {
        jobs_index = &queue_it->second;
    }
    jobs_index->push_back(job_it);
    queues_mutex->unlock();    
}

/*
Job* TransientQ::find(const string& queue_name, const string& uid)
{
    Job* job = NULL;
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    if (queues.end() != queue_it)
    {
        JobsIndex* jobs_index = queue_it->second.second;
        JobsIndex::iterator job_it = jobs_index->find(uid);
        if (jobs_index->end() != job_it) job = *job_it->second;
    }
    queues_mutex->unlock();
    return job;
}
*/

/*
void TransientQ::remove(const string& queue_name, const string& uid)
{
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    if (queues.end() != queue_it)
    {
        JobsList* jobs_list = queue_it->second.first;
        JobsIndex* jobs_index = queue_it->second.second;
        JobsIndex::iterator job_it = jobs_index->find(uid);
        if (jobs_index->end() != job_it)
        {
            // erase from index
            jobs_index->erase(job_it);
            // erase from list
            jobs_list->erase(job_it->second);            
        }
    }
    queues_mutex->unlock();
}
*/

Job* TransientQ::find_job(const string& uid)
{
    jobs_mutex->lock();
    Jobs::iterator job_it = jobs.find(uid);
    Job* job = jobs.end() != job_it ? job_it->second : NULL;
    jobs_mutex->unlock();
    return job;
}

Job* TransientQ::update_job_status(const string& uid, const JobStatus status, const string& status_description)
{
    Job* updated_job = NULL;
    jobs_mutex->lock();
    Jobs::iterator job_it = jobs.find(uid);
    if (jobs.end() != job_it)
    {
        updated_job = job_it->second->withStatus(status, status_description);
        jobs.insert(jobs.end(), pair<string, Job*>(updated_job->uid(), updated_job));
        jobs.erase(job_it);
    }
    jobs_mutex->unlock();
    return updated_job;
}

void TransientQ::delete_job(const string& uid)
{
    jobs_mutex->lock();
    Jobs::iterator job_it = jobs.find(uid);
    if (jobs.end() != job_it) jobs.erase(job_it);
    jobs_mutex->unlock();
    
    if (jobs.end() == job_it) return;
    
    queues_mutex->lock();
    for (Queues::iterator queue_it = queues.begin(); queue_it != queues.end(); queue_it++)
    {
        vector<Jobs::iterator>* jobs_list = &queue_it->second;
        vector<Jobs::iterator>::iterator jobs_list_it = std::find(jobs_list->begin(), jobs_list->end(), job_it);
        if (jobs_list->end() != jobs_list_it) jobs_list->erase(jobs_list_it);
    }
    queues_mutex->unlock();    
}
