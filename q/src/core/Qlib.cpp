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
void q_connect(void** ppq, const char* configuration)
{
    if (NULL == ppq) return;
    if (!QFactory::createQ((Q**)ppq, NULL != configuration ? configuration : "")) return;
    if (!(((Q*)*ppq)->connect())) *ppq = NULL;
}

EXPORT
void q_disconnect(void* pq)
{
    if (NULL == pq) return;
    ((Q*)pq)->disconnect();
}

EXPORT
const char* q_post(void* pq, const char* queue, const char* data, const long at)
{
    if (NULL == pq) return NULL;
    if (NULL == queue) return NULL;
    if (NULL == data) return NULL;
    Job* job = ((Q*)pq)->post(queue, data, at);
    return job->uid().c_str();
}

EXPORT
void q_worker(void* pq, const char* queue, void (*delegate)(const char**))
{
    if (NULL == pq) return;
    if (NULL == queue) return;
    if (NULL == delegate) return;
    //((Q*)q)->worker(queue, [=](Job* job, JobError** error)
    ((Q*)pq)->worker(queue, std::shared_ptr<std::function<void (Job*, JobError** error)>>(new std::function<void (Job*, JobError**)>([=](Job* job, JobError** error)
    {
        // TODO: deal with error
        const char* data = job->data().c_str();
        delegate(&data);
    })));
}

EXPORT
void q_observer(void* pq, const char* queue, void (*delegate)(const char**))
{
    if (NULL == pq) return;
    if (NULL == queue) return;
    if (NULL == delegate) return;
    //((Q*)q)->observer(queue, [=](Job* job, JobError** error)
    ((Q*)pq)->observer(queue, std::shared_ptr<std::function<void (Job*, JobError**)>>(new std::function<void (Job*, JobError**)>([=](Job* job, JobError** error)
    {
        // TODO: deal with error        
        const char* data = job->data().c_str();
        delegate(&data);
    })));
}



