//
//  Qlib.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__Qlib__
#define __q__Qlib__

#include <iostream>

#include <JavaVM/jni.h>

extern "C" char* q_version();

extern "C" void q_connect(void** q);

extern "C" void q_disconnect(void* q);

extern "C" const char* q_post(void* q, const char* queue, const char* data, const long at);

extern "C" void q_worker(void* q, const char* queue, void (*delegate)(const char**));

extern "C" void q_observer(void* q, const char* queue, void (*delegate)(const char**));

#endif /* defined(__q__Qlib__) */
