//
//  BerkleyQ.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "BerkleyQ.h"

#include <sstream>
#include <algorithm>

#include "JobCodec.h"
#include "Logger.h"

string encode_queue(const vector<string> data);
vector<string> decode_queue(const string& data);

string build_queue_key(const string& queue_name);
string build_queue_size_key(const string& queue_name);
string build_job_key(const string& job_uid);
const char* build_key(const char* format, ...);

#define VALUE_SIZE 1024*1024

BerkleyQ::BerkleyQ()
{
    active = false;
    db = NULL;
    queue_lock_id = 0;
}

BerkleyQ::~BerkleyQ()
{
}

void BerkleyQ::start()
{
    if (NULL != db) return;
        
    try
    {
        uint env_flags =    DB_CREATE     |  /* Create the environment if it does not exist */
                            DB_INIT_LOCK  |  /* Initialize the locking subsystem */
                            //DB_INIT_LOG   |  /* Initialize the logging subsystem */
                            //DB_INIT_TXN   |  /* Initialize the transactional subsystem. This
                            //                  * also turns on logging. */
                            DB_INIT_MPOOL |  /* Initialize the memory pool (in-memory cache) */
                            DB_PRIVATE;       /* Region files are not backed by the
                                              * filesystem. Instead, they are backed by
                                              * heap memory.  */
        
        DbEnv* env = new DbEnv(0);
        env->open(NULL, env_flags, 0);
        
        db = new Db(env, 0); // Instantiate the Db object                    
        db->open(NULL, // Transaction pointer
                 NULL, // Database file name
                 NULL, // Optional logical database name
                 DB_BTREE, // Database access method
                 DB_CREATE, // Open flags
                 0); // File mode (using defaults)
        
        
        db->get_env()->lock_id(&queue_lock_id);
        
        active = true;
    }
    catch(DbException &e)
    {
        q_error(e.what());
        db = NULL;
    }
    catch(std::exception &e)
    {
        q_error(e.what());
        db = NULL;
    }
    
    Q::start();
}

void BerkleyQ::stop()
{
    active = false;
    
    if (NULL == db) return;
    
    try
    {
        if (queue_lock_id != 0)
        {
            db->get_env()->lock_id_free(queue_lock_id);
            queue_lock_id = 0;
        }
        
        db->get_env()->close(0);
        //db->close(0);
        db = NULL;        
    }
    catch(DbException &e)
    {
        q_error(e.what());
    }
    catch(std::exception &e)
    {
        q_error(e.what());
    }
    
    Q::stop();
}

unsigned long BerkleyQ::size(const string& queue_name)
{
    if (!active) return -1;
    
    DbLock lock = acquire_queue_lock(queue_name, DB_LOCK_READ);
    unsigned long queue_size = load_queue_size(queue_name);
    release_queue_lock(&lock);
    return queue_size;
}

Job* BerkleyQ::peek(const string& queue_name)
{
    if (!active) return NULL;
    
    DbLock lock = acquire_queue_lock(queue_name, DB_LOCK_READ);
    vector<string> queue = load_queue(queue_name);
    release_queue_lock(&lock);    
    if (queue.empty()) return NULL;
    Job* job = load_job(*queue.begin());
    return job;
}

Job* BerkleyQ::take(const string& queue_name)
{
    if (!active) return NULL;
    
    DbLock lock = acquire_queue_lock(queue_name, DB_LOCK_WRITE);
    vector<string> queue = load_queue(queue_name);
    if (queue.empty())
    {
        release_queue_lock(&lock);
        return NULL;
    }
    vector<string>::iterator head = queue.begin();
    string uid = *head;
    queue.erase(head);
    save_queue(queue_name, queue);
    save_queue_size(queue_name, queue.size());
    release_queue_lock(&lock);
    return load_job(uid);
}

void BerkleyQ::push(const string& queue_name, Job* job)
{
    if (!active) return;
    
    save_job(job);
    
    DbLock lock = acquire_queue_lock(queue_name, DB_LOCK_WRITE);
    vector<string> queue = load_queue(queue_name);
    queue.insert(queue.end(), job->uid());
    save_queue(queue_name, queue);
    save_queue_size(queue_name, queue.size());
    release_queue_lock(&lock);    
}

Job* BerkleyQ::find(const string& queue_name, const string& uid)
{
    if (!active) return NULL;
    
    DbLock lock = acquire_queue_lock(queue_name, DB_LOCK_READ);
    vector<string> queue = load_queue(queue_name);
    vector<string>::iterator it = std::find(queue.begin(), queue.end(), uid);
    Job* job = it != queue.end() ? JobCodec::decode(*it) : NULL;
    release_queue_lock(&lock);
    return job;
}

void BerkleyQ::erase(const string& queue_name, const string& uid)
{
    if (!active) return;

    delete_job(uid);
    
    DbLock lock = acquire_queue_lock(queue_name, DB_LOCK_WRITE);
    vector<string> queue = load_queue(queue_name);
    vector<string>::iterator it = std::find(queue.begin(), queue.end(), uid);
    if (it == queue.end())
    {
        release_queue_lock(&lock);
        return;
    }
    queue.erase(it);
    save_queue(queue_name, queue);
    save_queue_size(queue_name, queue.size());
    release_queue_lock(&lock);
}

DbLock BerkleyQ::acquire_queue_lock(const string& queue_name, const db_lockmode_t lock_mode)
{
    DbLock lock;
    Dbt lock_info((void*)queue_name.c_str(), (uint)queue_name.size()+1);
    switch (int ret = db->get_env()->lock_get(queue_lock_id, 0, &lock_info, lock_mode, &lock))
    {
        case 0:
            //q_log("berkeley queue [%s] lock acquired", queue_name.c_str());
            break;
        default:
            q_error("berkeley lock_put: unknown error");
    }
    return lock;
}

void BerkleyQ::release_queue_lock(DbLock* lock)
{
    switch (int ret = db->get_env()->lock_put(lock))
    {
        case 0:
            //q_log("berkeley queue lock released");
            break;
        default:
            q_error("berkeley lock_put: unknown error");
    }
}

vector<string> BerkleyQ::load_queue(const string& queue_name)
{
    string key = build_queue_key(queue_name);
    string result = load_record(key);
    return decode_queue(result);
}

void BerkleyQ::save_queue(const string& queue_name, vector<string> data)
{
    string key = build_queue_key(queue_name);
    string value = encode_queue(data);
    save_record(key, value, 0);
}

unsigned long BerkleyQ::load_queue_size(const string& queue_name)
{
    string key = build_queue_size_key(queue_name);
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
    
    Dbt value_accessor;
    unsigned long value = 0;
    value_accessor.set_data(&value);
    value_accessor.set_ulen(sizeof(unsigned long));
    value_accessor.set_flags(DB_DBT_USERMEM);
    
    string result;
    switch (int ret = db->get(NULL, &key_accessor, &value_accessor, 0))
    {
        case 0:
            //q_log("berkeley get: [%s] key found", key.c_str());
            break;
        case DB_NOTFOUND:
            //q_log("berkeley get: [%s] key not found", key.c_str());
            break;
        default:
            //db->err(ret, "Put failed because key %s already exists", key);
            q_error("berkeley get: unknown error");
    }
    return value;
}

void BerkleyQ::save_queue_size(const string& queue_name, unsigned long size)
{
    string key = build_queue_size_key(queue_name);
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
    Dbt value_accessor((void*)&size, sizeof(unsigned long));
    
    switch (int ret = db->put(NULL, &key_accessor, &value_accessor, DB_OVERWRITE_DUP))
    {
        case 0:
            //q_log("berkeley put: [%s] key stored", key.c_str());
            break;
        default:
            //db->err(ret, "Put failed because key %s already exists", key);
            q_error("berkeley put: unknown error");
    }
}

Job* BerkleyQ::load_job(const string& uid)
{
    string key = build_job_key(uid);
    string result = load_record(key);
    return JobCodec::decode(result);
}

void BerkleyQ::save_job(const Job* job)
{
    string key = build_job_key(job->uid());
    string value = JobCodec::encode(job);
    save_record(key, value, DB_OVERWRITE_DUP);
}

void BerkleyQ::delete_job(const string& uid)
{
    string key = build_job_key(uid);
    delete_record(key, 0);
}

string BerkleyQ::load_record(const string& key)
{
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
    
    Dbt value_accessor;
    char value[VALUE_SIZE+1];
    value_accessor.set_data(value);
    value_accessor.set_ulen(VALUE_SIZE+1);
    value_accessor.set_flags(DB_DBT_USERMEM);
    
    string result;
    switch (int ret = db->get(NULL, &key_accessor, &value_accessor, 0))
    {
        case 0:
        {
            //q_log("berkeley get: [%s] key found", key.c_str());
            uint length = value_accessor.get_size();
            result.append(value, 0, length);
            break;
        }
        case DB_NOTFOUND:
            q_log("berkeley get: [%s] key not found", key.c_str());
            break;
        default:
            //db->err(ret, "Put failed because key %s already exists", key);
            q_error("berkeley get: unknown error");
    }
    return result;
}

void BerkleyQ::save_record(const string& key, const string& value, const uint flags)
{
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
    Dbt value_accessor((void*)value.c_str(), (uint)value.size());
    
    switch (int ret = db->put(NULL, &key_accessor, &value_accessor, flags))
    {
        case 0:
            //q_log("berkeley put: [%s] key stored", key.c_str());
            break;
        case DB_KEYEXIST:
            q_log("berkeley put: [%s] key already exists", key.c_str());
            break;
        default:
            //db->err(ret, "Put failed because key %s already exists", key);
            q_error("berkeley put: unknown error");
    }   
}

void BerkleyQ::delete_record(const string& key, const uint flags)
{
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
        
    switch (int ret = db->del(NULL, &key_accessor, flags))
    {
        case 0:
            q_log("berkeley del: [%s] key deleted", key.c_str());
            break;
        case DB_NOTFOUND:
            q_log("berkeley del: [%s] key already exists", key.c_str());
            break;
        default:
            //db->err(ret, "Put failed because key %s already exists", key);
            q_error("berkeley del: unknown error");
    }
}

string encode_queue(const vector<string> data)
{
    stringstream stream;
    copy(data.begin(), data.end(), ostream_iterator<string>(stream, " "));
    return stream.str();
}

vector<string> decode_queue(const string& data)
{
    if (0 == data.size()) return vector<string>();
    stringstream stream(data);
    istream_iterator<std::string> begin(stream);
    istream_iterator<std::string> end;
    vector<string> vstrings(begin, end);
    return vstrings;
}

string build_queue_key(const string& queue_name)
{
    return build_key("q:%s", queue_name.c_str());
}

string build_queue_size_key(const string& queue_name)
{
    return build_key("q:%s:s", queue_name.c_str());
}

string build_job_key(const string& job_uid)
{
    return build_key("j:%s", job_uid.c_str());
}

const char* build_key(const char* format, ...)
{
    va_list args;
    va_start(args, format);    
    int max = 1024;
    char* buffer = new char[max];
    int written = vsnprintf(buffer, max, format, args);
    char* key = new char[written];
    strncpy(key, buffer, written);
    key[written] = '\0';
    va_end(args);
    return key;
}
