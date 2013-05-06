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

Q::Q(const Json::Value& configuration) : configuration(configuration), active(false)
{
}

Q::~Q()
{
}

bool Q::connect()
{
    start();
    return true;
}

void Q::disconnect()
{
    stop();
}

const string Q::post(const string& queue_name, const string& data, const long at)
{
    if (!this->active) return "";
    
    Job job = Job(data, JSPending, at);
    push_back(queue_name, job);
    q_log("posted [%s] on [%s] as [%s]", data.c_str(), queue_name.c_str(), job.uid().c_str());
    return job.uid();
}

void Q::worker(const string& queue_name, const WorkerDelegate* delegate)
{
    if (!this->active) return;
        
    q_log("registering worker for [%s]", queue_name.c_str());        
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
    list->push_back(shared_ptr<const WorkerDelegate>(delegate));
    workers_mutex->unlock();
}

void Q::observer(const string& queue_name, const ObserverDelegate* delegate)
{
    if (!this->active) return;
    
    q_log("registering observer on [%s]", queue_name.c_str());
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
    list->push_back(shared_ptr<const ObserverDelegate>(delegate));
    observers_mutex->unlock();
}

vector<string> Q::queues()
{
    if (!this->active) vector<string>();
    
    vector<string> result;
    for (Monitors::iterator it = monitors.begin(); it != monitors.end(); it++)
    {
        result.push_back(it->first);
    }
    return result;
}

void Q::start()
{
    this->active = true;
    q_log("connected");
}

void Q::stop()
{
    this->active = false;
    q_log("disconnected");
}

/*
void Q::handle_job_result(const string& queue_name, const Job& job, const JobError* error)
{
    bool success = NULL == error;
    q_log("job %s completed %s", job.uid().c_str(), (success ? "successfully" : "with error"));
    return update_job_status(job.uid(), success ? JSComplete : JSFailed, success ? "" : error->description());
}
*/

void Q::verify_queue_monitor(const string& queue_name)
{
    Monitors::iterator it = this->monitors.find(queue_name);
    if (this->monitors.end() != it) return;
    thread* monitor = new thread(Q::monitor_queue, this, queue_name);
    this->monitors.insert(it, pair<string, thread*>(queue_name, monitor));
}

void Q::monitor_queue(Q* q, const string& queue_name)
{    
    while (q->active)
    {
        //q_log("checking queue [%s]", queue_name.c_str());
        
        // find the next job
        JobOption job;
        unsigned long size = q->size(queue_name);
        for (unsigned long index=0; index < size; index++)
        {
            JobOption candidate = q->pop_front(queue_name);
            if (candidate.empty()) continue;
            
            long now = 0;
            time(&now);            
            if (candidate.get().at() <= now)
            {
                job = candidate;
                break;
            }
            else
            {
                // put back in queue
                q->push_back(queue_name, candidate.get());
            }
        }
    
        if (job.empty())
        {
            q_log("[%s] - no jobs", queue_name.c_str());            
            sleep(1);
            continue;
        }
        
        string job_uid = job.get().uid();

        workers_mutex->lock();
        Workers::iterator workers_it = q->workers.find(queue_name);
        if (q->workers.end() == workers_it || workers_it->second.first.empty())
        {
            workers_mutex->unlock();
            q_log("[%s] - no workers", queue_name.c_str());
            // put back in queue                
            q->push_back(queue_name, job.get());
            sleep(1);
            continue;
        }

        // update job to active
        JobOption updated_job1 = q->update_job_status(job_uid, JSActive, "");
        if (updated_job1.empty())
        {
            workers_mutex->unlock();
            q_log("failed updating job [%s] status", job_uid.c_str());
            // put back in queue - have not been processes yet
            q->push_back(queue_name, job.get());
            continue;
        }
        
        // call workers in round-rubin fashion
        JobError* job_error = NULL;
        try
        {
            //q_log("[%s] running worker on [%s]", queue_name.c_str(), job_uid.c_str());
            pair<WorkersList, uint>* workers_info = &workers_it->second;
            WorkersList* workers_list = &workers_info->first;
            uint* worker_index = &workers_info->second;
            if (*worker_index >= workers_list->size()) *worker_index=0;;
            shared_ptr<const WorkerDelegate>& worker = workers_list->at(*worker_index);
            worker.get()->operator()(&updated_job1.get(), &job_error);
            ++*worker_index;
        }
        catch (exception& e)
        {
            q_log("[%s] worker failed on [%s]. %s", queue_name.c_str(), job_uid.c_str(), e.what());
            job_error = new JobError(e.what());
        }
        catch (...)
        {
            q_log("[%s] worker failed on [%s]. unknown error", queue_name.c_str(), job_uid.c_str());
            job_error = new JobError("unknown error");
        }

        workers_mutex->unlock();

        // update job to complete / failed
        bool job_success = NULL == job_error;
        JobOption updated_job2 = q->update_job_status(job_uid, job_success ? JSComplete : JSFailed, job_success ? "" : job_error->description());
        if (updated_job2.empty())
        {
            // job has already been processed - only reporting issue to log
            q_log("failed updating job [%s] status", job_uid.c_str());
            continue;
        }
        
        // job failed - no need to notify observers
        if (!job_success) continue;
        
        // notify observers
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
                    shared_ptr<const ObserverDelegate> observer = *observer_it;
                    observer.get()->operator()(&updated_job2.get(), &observer_error);
                }
                catch (exception& e)
                {
                    q_log("[%s] observer failed on [%s]. %s", queue_name.c_str(), job_uid.c_str(), e.what());
                }
                catch (...)
                {
                    q_log("[%s] observer failed on [%s]. unknown error", queue_name.c_str(), job_uid.c_str());
                }
            }
        }
        observers_mutex->unlock();

        //usleep(100);
    }
    
    //q_log("exiting queue [%s] loop", queue_name.c_str());
}