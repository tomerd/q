//
//  Qlib.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "Qlib.h"

#include "QFactory.h"
#include "Logger.h"

#define EXPORT __attribute__((visibility("default")))

EXPORT
const char* q_version()
{
    return "0.0.1";
}

EXPORT
void q_connect(void** pq)
{
    *pq = QFactory::createQ("");
    ((Q*)(*pq))->start();
}

EXPORT
void q_disconnect(void* q)
{
    ((Q*)q)->stop();
}

EXPORT
const char* q_post(void* q, const char* queue, const char* data, const long at)
{
    Job* job = ((Q*)q)->post(queue, data, at);
    return job->uid().c_str();
}

EXPORT
void q_worker(void* q, const char* queue, void (*delegate)(const char**))
{
    //((Q*)q)->worker(queue, [=](Job* job, JobError** error)
    ((Q*)q)->worker(queue, std::shared_ptr<std::function<void (Job*, JobError** error)>>(new std::function<void (Job*, JobError**)>([=](Job* job, JobError** error)
    {
        // TODO: deal with error
        const char* data = job->data().c_str();
        delegate(&data);
    })));
}

EXPORT
void q_observer(void* q, const char* queue, void (*delegate)(const char**))
{
    //((Q*)q)->observer(queue, [=](Job* job, JobError** error)
    ((Q*)q)->observer(queue, std::shared_ptr<std::function<void (Job*, JobError**)>>(new std::function<void (Job*, JobError**)>([=](Job* job, JobError** error)
    {
        // TODO: deal with error        
        const char* data = job->data().c_str();
        delegate(&data);
    })));
}



