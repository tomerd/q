//
//  BerkeleyQ.h
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__BerkeleyQ__
#define __q__BerkeleyQ__

#include <iostream>
#include <vector>

#include "Q.h"
// FIXME: use better linking & header inclusion stratgey
#include "db_cxx.h"

using namespace std;

class BerkeleyQ : public Q
{
    
public:
    
    BerkeleyQ();
    ~BerkeleyQ();
    
    void start();
    void stop();
    
protected:
    
    unsigned long size(const string& queue);
    Job* peek(const string& queue);
    Job* take(const string& queue);
    void push(const string& queue, Job* job);
    Job* find(const string& queue, const string& uid);
    void erase(const string& queue, const string& uid);
    
private:
    
    bool active;
    Db* db;
    uint locker_id;
    
    DbLock acquire_lock(const string& lock_name, const db_lockmode_t lock_mode);
    void release_lock(DbLock* lock);
    
    vector<string> load_queue(const string& queue_name);
    void save_queue(const string& queue_name, const vector<string> data);
    
    string load_queue_raw(const string& queue_name);
    void save_queue_raw(const string& queue_name, const string& data);
    
    unsigned long load_queue_size(const string& queue_name);
    void save_queue_size(const string& queue_name, unsigned long size);
    
    Job* load_job(const string& uid);
    void save_job(const Job* job);
    void delete_job(const string& uid);
    
    string load_record(const string& key);
    void save_record(const string& key, const string& value, const uint flags);
    void delete_record(const string& key, const uint flags);
};


#endif /* defined(__q__BerkeleyQ__) */