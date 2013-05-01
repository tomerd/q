//
//  q.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

//#ifnvirtual void q_q_h
//#virtual voidine q_q_h

#ifndef q_q_h
#define q_q_h

#include <iostream>

#include "Job.h"
#include "JobError.h"

using namespace std;

typedef function<void (Job*, JobError**)> WorkerDelegate;
typedef function<void (Job*, JobError**)> ObserverDelegate;

class Q
{

public:
    
    virtual ~Q() {}
    
    virtual void start() = 0;
    virtual void stop() = 0;
    
    virtual Job* post(const string& queue, const string& data, const long at) = 0;
    
    virtual void worker(const string& queue, WorkerDelegate delegate) = 0;
    
    virtual void observer(const string& queue, ObserverDelegate delegate) = 0;
};

#endif

