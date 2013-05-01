//
//  TransientQ.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "TransientQ.h"

#include <unistd.h>
#include "Logger.h"

mutex* TransientQ::queues_mutex = new mutex();
mutex* TransientQ::workers_mutex = new mutex();
mutex* TransientQ::observers_mutex = new mutex();
Queues TransientQ::queues = Queues();

TransientQ::TransientQ()
{
    this->started = false;
}

TransientQ::~TransientQ()
{
}

void TransientQ::start()
{
    this->started = true;
}

void TransientQ::stop()
{
    this->started = false;
}

Job* TransientQ::post(const string& queue_name, const string& data, const long at)
{
    queues_mutex->lock();
    Queues::iterator it = queues.find(queue_name);
    JobsList* jobs_list = NULL;
    if (queues.end() == it)
    {
        jobs_list = queues.insert(it, pair<string, JobsList*>(queue_name, new JobsList()))->second;
    }
    else
    {
        jobs_list = it->second;
    }
    Job* job = new Job(data, at);
    jobs_list->push_back(job);
    queues_mutex->unlock();
    return job;
}

void TransientQ::worker(const string& queue_name, WorkerDelegate delegate)
{
    verify_queue_monitor(queue_name);
    workers_mutex->lock();
    Workers::iterator it = this->workers.find(queue_name);
    WorkersList* list = NULL;
    if (this->workers.end() == it)
    {
        list = this->workers.insert(it, pair<string, pair<WorkersList*, uint>>(queue_name, pair<WorkersList*, uint>(new WorkersList(), 0)))->second.first;
    }
    else
    {
        list = it->second.first;
    }
    list->push_back(delegate);
    workers_mutex->unlock();
}

void TransientQ::observer(const string& queue_name, ObserverDelegate delegate)
{
    verify_queue_monitor(queue_name);
    observers_mutex->lock();
    Observers::iterator it = this->observers.find(queue_name);
    ObserversList* list = NULL;
    if (this->observers.end() == it)
    {
        list = this->observers.insert(it, pair<string, ObserversList*>(queue_name, new ObserversList()))->second;
    }
    else
    {
        list = it->second;
    }
    list->push_back(delegate);
    observers_mutex->unlock();
}

Job* TransientQ::peek(const string& queue_name)
{
    queues_mutex->lock();
    Queues::iterator queue_it = queues.find(queue_name);
    Job* job = (queues.end() == queue_it) ? NULL : queue_it->second->empty() ? NULL : *queue_it->second->begin();
    queues_mutex->unlock();
    return job;
}

void TransientQ::handle_job_result(const string& queue_name, const Job* job, const JobError* error)
{
    q_log("job %s completed %s", job->uid().c_str(), (NULL == error ? "successfully" : "with error"));
    if (NULL != error) q_error(error->description());
}

void TransientQ::verify_queue_monitor(const string& queue_name)
{
    Monitors::iterator it = this->monitors.find(queue_name);
    if (this->monitors.end() != it) return;
    thread* monitor = new thread(TransientQ::monitor_queue, this, queue_name);
    this->monitors.insert(it, pair<string, thread*>(queue_name, monitor));
}

void TransientQ::monitor_queue(TransientQ* q, const string& queue_name)
{
    while (q->started)
    {
        q_log("checking queue '%s'", queue_name.c_str());
        
        long now = 0;
        time(&now);
        Job* job = NULL;
        
        queues_mutex->lock();
        Queues::iterator queue_it = queues.find(queue_name);
        if (queues.end() != queue_it)
        {
            int attempts = 0;
            while (NULL == job && attempts < queue_it->second->size())
            {
                JobsList::iterator jobs_it = queue_it->second->begin();
                Job* candidate = *jobs_it;
                if (candidate->at() < now)
                {
                    job = candidate;
                    queue_it->second->erase(jobs_it);
                }
                else
                {
                    queue_it->second->erase(jobs_it);
                    queue_it->second->push_back(candidate);
                    ++attempts;
                }
            }
        }
        queues_mutex->unlock();
        
        workers_mutex->lock();
        Workers::iterator workers_it = q->workers.find(queue_name);
        
        if (NULL == job
            ||
            (q->workers.end() == workers_it) || workers_it->second.first->empty())
        {
            
            workers_mutex->unlock();            
            sleep(1);
        }
        else
        {
            JobError* error = NULL;            
            try
            {
                // round rubin worker
                uint* index = &workers_it->second.second;
                if (*index >= workers_it->second.first->size()) *index=0;;
                WorkerDelegate worker = workers_it->second.first->at(*index);
                worker(job, &error);
                ++*index;
            }
            catch (exception& e)
            {
                error = new JobError(e.what());
            }
            q->handle_job_result(queue_name, job, error);
            workers_mutex->unlock();
            
            // observers
            observers_mutex->lock();
            Observers::iterator observers_it = q->observers.find(queue_name);
            if (q->observers.end() != observers_it)
            {
                ObserversList* observers_list = observers_it->second;
                for (ObserversList::iterator observer_it = observers_list->begin(); observer_it != observers_list->end(); observer_it++)
                {
                    try
                    {
                        JobError* error = NULL;
                        ObserverDelegate observer = *observer_it;
                        observer(job, &error);
                    }
                    catch (exception& e)
                    {
                        // TODO
                    }
                }
            }
            observers_mutex->unlock();
        }
    }
}

