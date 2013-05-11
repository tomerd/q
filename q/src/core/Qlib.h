//
//  Qlib.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__Qlib__
#define __q__Qlib__

extern "C"
{
    const char* q_version();

    void q_connect(void** pq, const char* configuration);

    void q_disconnect(void* q);

    void q_post(void* q, const char* queue, const char* uid, const char* data, const long run_at=0, char** puid=0);

    bool q_reschedule(void* q, const char* uid, const long run_at);

    bool q_cancel(void* q, const char* uid);

    void q_worker(void* q, const char* queue, void (*delegate)(const char**));

    void q_observer(void* q, const char* queue, void (*delegate)(const char**));

    void q_flush(void* q);
}

#endif /* defined(__q__Qlib__) */
