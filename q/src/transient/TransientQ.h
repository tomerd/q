//
//  TransientQ.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__TransientQ__
#define __q__TransientQ__

#include <iostream>
#include <map>
//#include <list>
#include <vector>

#include "Q.h"

typedef vector<Job*> JobsList;
typedef map<string, JobsList::iterator> JobsIndex;
typedef map<string, pair<JobsList*, JobsIndex*>> Queues;

class TransientQ : public Q
{
            
public:
    
    TransientQ();
    ~TransientQ();
    
protected:
    
    unsigned long size(const string& queue);
    Job* peek(const string& queue);
    Job* take(const string& queue);
    void push(const string& queue, Job* job);
    Job* find(const string& queue, const string& uid);    
    void erase(const string& queue, const string& uid);
    
private:
    
    static Queues queues;
    static mutex* queues_mutex;
    
};

#endif /* defined(__q__TransientQ__) */
