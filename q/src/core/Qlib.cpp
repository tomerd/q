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
char* q_version()
{
    return  "0.0.1";
}

EXPORT
void q_connect(void** q)
{
    *q = QFactory::createQ();
    ((Q*)(*q))->start();
    q_log("connected");
}

EXPORT
void q_disconnect(void* q)
{
    ((Q*)q)->stop();
    q_log("disconnected");
}

EXPORT
const char* q_post(void* q, const char* queue, const char* data, const long at)
{
    Job* job = ((Q*)q)->post(queue, data, at);
    q_log("posted %s on %s (%s)", data, queue, job->uid().c_str());    
    return job->uid().c_str();
}

EXPORT
void q_worker(void* q, const char* queue, void (*delegate)(const char**))
{
    q_log("registering worker for %s", queue);
    ((Q*)q)->worker(queue, [=](Job* job, JobError** error)
    {
        const char* data = job->data().c_str();
        delegate(&data);
    });
}

EXPORT
void q_observer(void* q, const char* queue, void (*delegate)(const char**))
{
    q_log("registering observer on %s", queue);
    ((Q*)q)->observer(queue, [=](Job* job, JobError** error)
    {
        const char* data = job->data().c_str();
        delegate(&data);
    });
}



