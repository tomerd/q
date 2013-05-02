//
//  TransientQ.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "TransientQ.h"

Queues TransientQ::queues = Queues();
mutex* TransientQ::queues_mutex = new mutex();

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
    unsigned long size = (queues.end() == queue_it) ? 0 : queue_it->second.first->size();
    queues_mutex->unlock();
    return size;
}

Job* TransientQ::peek(const string& queue_name)
{
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    Job* job = (queues.end() == queue_it) ? NULL : queue_it->second.first->empty() ? NULL : *queue_it->second.first->begin();
    queues_mutex->unlock();
    return job;
}

Job* TransientQ::take(const string& queue_name)
{
    Job* job = NULL;
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    if (queues.end() != queue_it && !queue_it->second.first->empty())
    {
        JobsList* jobs_list = queue_it->second.first;
        JobsIndex* jobs_index = queue_it->second.second;
        // take from list - we want the head
        JobsList::iterator jobs_list_it = jobs_list->begin();
        job = *jobs_list_it;
        // erase from list
        jobs_list->erase(jobs_list_it);
        // erase from index
        JobsIndex::iterator jobs_index_it = jobs_index->find(job->uid());
        if (jobs_index->end() != jobs_index_it) jobs_index->erase(jobs_index_it);        
    }
    queues_mutex->unlock();
    return job;
}

void TransientQ::push(const string& queue_name, Job* job)
{
    queues_mutex->lock();
    Queues::iterator it = queues.find(queue_name);
    pair<JobsList*, JobsIndex*> jobs_pair;
    if (queues.end() == it)
    {
        jobs_pair = queues.insert(it, pair<string, pair<JobsList*, JobsIndex*>>(queue_name, pair<JobsList*, JobsIndex*>(new JobsList(), new JobsIndex())))->second;
    }
    else
    {
        jobs_pair = it->second;
    }
    jobs_pair.first->push_back(job);
    jobs_pair.second->insert(pair<string, JobsList::iterator>(job->uid(), jobs_pair.first->end()));
    queues_mutex->unlock();
}

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

void TransientQ::erase(const string& queue_name, const string& uid)
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
