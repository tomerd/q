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
void q_post(void* pq, const char* queue, const char* data, const long at, char** puid)
{
    if (NULL == pq) return;
    if (NULL == queue) return;
    if (NULL == data) return;
    string uid = ((Q*)pq)->post(queue, data, at);
    if (NULL != puid)
    {
        *puid = new char[uid.size()+1];
        strcpy(*puid, uid.c_str());
        (*puid)[uid.size()] = '\0';
    }
}

EXPORT
void q_worker(void* pq, const char* queue, void (*delegate)(const char**))
{
    if (NULL == pq) return;
    if (NULL == queue) return;
    if (NULL == delegate) return;
    //((Q*)q)->worker(queue, [=](Job* job, JobError** error)
    ((Q*)pq)->worker(queue, WorkerDelegate(new std::function<void (const Job*, JobError**)>([=](const Job* job, JobError** error)
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
    ((Q*)pq)->observer(queue, ObserverDelegate(new std::function<void (const Job*, JobError**)>([=](const Job* job, JobError** error)
    {
        // TODO: deal with error
        const char* data = job->data().c_str();
        delegate(&data);
        
    })));
}

EXPORT
void q_flush(void* pq)
{
    if (NULL == pq) return;
    ((Q*)pq)->flush();
}
