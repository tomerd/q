//
//  TransientQ.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "TransientQ.h"

#include <algorithm>

namespace Q
{
    Queues TransientQ::queues = Queues();
    mutex* TransientQ::queues_mutex = new mutex();

    Jobs TransientQ::jobs = Jobs();
    mutex* TransientQ::jobs_mutex = new mutex();

    TransientQ::TransientQ(const Json::Value& configuration) : Q::Q(configuration)
    {
    }

    TransientQ::~TransientQ()
    {
    }

    void TransientQ::flush()
    {
        queues_mutex->lock();
        queues.clear();
        queues_mutex->unlock();
        
        jobs_mutex->lock();
        jobs.clear();
        jobs_mutex->unlock();
    }

    unsigned long TransientQ::size(const string& queue_name)
    {
        queues_mutex->lock();
        Queues::iterator queue_it = queues.find(queue_name);
        unsigned long size = (queues.end() == queue_it) ? 0 : queue_it->second.size();
        queues_mutex->unlock();
        return size;
    }

    JobOption TransientQ::peek(const string& queue_name)
    {
        queues_mutex->lock();
        Queues::iterator queue_it = queues.find(queue_name);
        Jobs::iterator job_it = queues.end() == queue_it || queue_it->second.empty() ? jobs.end() : *queue_it->second.begin();
        Job* job = (jobs.end() == job_it) ? NULL : &job_it->second;
        queues_mutex->unlock();
        return NULL != job ? JobOption(*job) : JobOption();
    }

    JobOption TransientQ::pop_front(const string& queue_name)
    {
        queues_mutex->lock();
        Queues::iterator queue_it = queues.find(queue_name);
        Jobs::iterator job_it = queues.end() == queue_it || queue_it->second.empty() ? jobs.end() : *queue_it->second.begin();
        Job* job = (jobs.end() == job_it) ? NULL : &job_it->second;
        queue_it->second.erase(queue_it->second.begin());
        queues_mutex->unlock();    
        return NULL != job ? JobOption(*job) : JobOption();
    }

    void TransientQ::push_back(const string& queue_name, const Job& job)
    {
        jobs_mutex->lock();
        Jobs::iterator job_it = jobs.insert(jobs.end(), pair<string, Job>(job.uid(), job));
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

    JobOption TransientQ::find_job(const string& uid)
    {
        jobs_mutex->lock();
        Jobs::iterator job_it = jobs.find(uid);
        Job* job = jobs.end() != job_it ? &job_it->second : NULL;
        jobs_mutex->unlock();
        return job != NULL ? JobOption(*job) : JobOption();
    }

    JobOption TransientQ::update_job_status(const string& uid, const JobStatus status, const string& status_description)
    {
        JobOption updated_job;
        jobs_mutex->lock();
        Jobs::iterator job_it = jobs.find(uid);
        if (jobs.end() != job_it)
        {
            Job new_job = job_it->second.withStatus(status, status_description);
            jobs.erase(job_it);
            jobs.insert(jobs.end(), pair<string, Job>(new_job.uid(), new_job));        
            updated_job = JobOption(new_job);
        }
        jobs_mutex->unlock();
        return updated_job;
    }

    JobOption TransientQ::update_job_run_at(const string& uid, const long run_at)
    {
        JobOption updated_job;
        jobs_mutex->lock();
        Jobs::iterator job_it = jobs.find(uid);
        if (jobs.end() != job_it)
        {
            Job new_job = job_it->second.withRunAt(run_at);
            jobs.erase(job_it);
            jobs.insert(jobs.end(), pair<string, Job>(new_job.uid(), new_job));
            updated_job = JobOption(new_job);
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
}
