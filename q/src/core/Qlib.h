//
//  Qlib.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__Qlib__
#define __q__Qlib__

#include <JavaVM/jni.h>

extern "C" const char* q_version();

extern "C" void q_connect(void** pq, const char* configuration);

extern "C" void q_disconnect(void* q);

extern "C" void q_post(void* q, const char* queue, const char* uid, const char* data, const long run_at=0, char** puid=NULL);

extern "C" bool q_reschedule(void* q, const char* uid, const long run_at);

extern "C" bool q_cancel(void* q, const char* uid);

extern "C" void q_worker(void* q, const char* queue, void (*delegate)(const char**));

extern "C" void q_observer(void* q, const char* queue, void (*delegate)(const char**));

extern "C" void q_flush(void* q);

#endif /* defined(__q__Qlib__) */
