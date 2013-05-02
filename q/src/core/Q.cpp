//
//  Q.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "Q.h"

#include <unistd.h>
#include "Logger.h"

mutex* Q::workers_mutex = new mutex();
mutex* Q::observers_mutex = new mutex();

Q::Q()
{
    this->started = false;
}

Q::~Q()
{
}

void Q::start()
{
    this->started = true;
    q_log("connected");
}

void Q::stop()
{
    this->started = false;
    q_log("disconnected");
}

Job* Q::post(const string& queue_name, const string& data, const long at)
{
    Job* job = new Job(data, at);
    push(queue_name, job);
    q_log("posted %s on %s (%s)", data.c_str(), queue_name.c_str(), job->uid().c_str());
    return job;
}

void Q::worker(const string& queue_name, WorkerDelegate delegate)
{
    q_log("registering worker for %s", queue_name.c_str());
    verify_queue_monitor(queue_name);
    workers_mutex->lock();
    Workers::iterator it = this->workers.find(queue_name);
    WorkersList* list = NULL;
    if (this->workers.end() == it)
    {
        list = &this->workers.insert(it, pair<string, pair<WorkersList, uint>>(queue_name, pair<WorkersList, uint>(WorkersList(), 0)))->second.first;
    }
    else
    {
        list = &it->second.first;
    }
    list->push_back(delegate);
    workers_mutex->unlock();
}

void Q::observer(const string& queue_name, ObserverDelegate delegate)
{
    q_log("registering observer on %s", queue_name.c_str());
    verify_queue_monitor(queue_name);
    observers_mutex->lock();
    Observers::iterator it = this->observers.find(queue_name);
    ObserversList* list = NULL;
    if (this->observers.end() == it)
    {
        list = &this->observers.insert(it, pair<string, ObserversList>(queue_name, ObserversList()))->second;
    }
    else
    {
        list = &it->second;
    }
    list->push_back(delegate);
    observers_mutex->unlock();
}

void Q::handle_job_result(const string& queue_name, const Job* job, const JobError* error)
{
    q_log("job %s completed %s", job->uid().c_str(), (NULL == error ? "successfully" : "with error"));
    if (NULL != error) q_error(error->description());
}

void Q::verify_queue_monitor(const string& queue_name)
{
    Monitors::iterator it = this->monitors.find(queue_name);
    if (this->monitors.end() != it) return;
    thread* monitor = new thread(Q::monitor_queue, this, queue_name);
    this->monitors.insert(it, pair<string, thread*>(queue_name, monitor));
}

void Q::monitor_queue(Q* q, const string& queue_name)
{    
    while (q->started)
    {
        q_log("checking queue '%s'", queue_name.c_str());
        
        long now = 0;
        time(&now);
        Job* job = NULL;

        unsigned long size = q->size(queue_name);        
        for (unsigned long index=0; index < size; index++)
        {
            Job* candidate = q->take(queue_name);
            if (NULL != candidate && candidate->at() < now)
            {
                job = candidate;
                break;
            }
            else if (NULL != candidate)
            {
                q->push(queue_name, candidate);
            }
        }
        
        workers_mutex->lock();
        Workers::iterator workers_it = q->workers.find(queue_name);
        
        if (NULL == job
            ||
            (q->workers.end() == workers_it) || workers_it->second.first.empty())
        {
            
            workers_mutex->unlock();
            sleep(1);
        }
        else
        {
            JobError* job_error = NULL;
            try
            {
                // round-rubin
                pair<WorkersList, uint>* workers_info = &workers_it->second;
                WorkersList* workers_list = &workers_info->first;
                uint* worker_index = &workers_info->second;
                if (*worker_index >= workers_list->size()) *worker_index=0;;
                WorkerDelegate worker = workers_list->at(*worker_index);
                worker->operator()(job, &job_error);
                ++*worker_index;
            }
            catch (exception& e)
            {
                job_error = new JobError(e.what());
            }
            q->handle_job_result(queue_name, job, job_error);
            workers_mutex->unlock();
            
            // observers
            observers_mutex->lock();
            Observers::iterator observers_it = q->observers.find(queue_name);
            if (q->observers.end() != observers_it)
            {
                ObserversList* observers_list = &observers_it->second;
                for (ObserversList::iterator observer_it = observers_list->begin(); observer_it != observers_list->end(); observer_it++)
                {
                    try
                    {
                        JobError* observer_error = NULL;
                        ObserverDelegate observer = *observer_it;
                        observer->operator()(job, &observer_error);
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