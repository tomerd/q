//
//  BerkeleyQ.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "BerkeleyQ.h"

#include <sstream>
#include <algorithm>

#include "JobCodec.h"
#include "Logger.h"

static string encode_queue(const vector<string> data);
static vector<string> decode_queue(const string& data);

static string build_queue_key(const string& queue_name);
static string build_queue_size_key(const string& queue_name);
static string build_job_key(const string& job_uid);
static string build_key(const char* format, ...);

BerkeleyQ::BerkeleyQ(const Json::Value& configuration) : Q::Q(configuration)
{
    db = NULL;
    locker_id = 0;
}

BerkeleyQ::~BerkeleyQ()
{
}

bool BerkeleyQ::connect()
{
    if (NULL != db) return true;
        
    try
    {
        int ret = 0;        
        uint env_flags =    DB_CREATE     |  /* Create the environment if it does not exist */
                            DB_INIT_LOCK  |  /* Initialize the locking subsystem */
                            DB_INIT_MPOOL |  /* Initialize the memory pool (in-memory cache) */
                            DB_PRIVATE    |  /* Region files are not backed by the
                                              * filesystem. Instead, they are backed by
                                              * heap memory.  */
                            DB_THREAD;
        
        DbEnv* env = new DbEnv(0);
        
        ret = env->set_cachesize(0,                  /* 0 gigabytes */
                                 50 * 1024 * 1024,   /* 50 megabytes */
                                 1);                 /* Create 1 cache. All memory will
                                                      * be allocated contiguously. */
        env->open(NULL, env_flags, 0);
        
        db = new Db(env, 0); // Instantiate the Db object
                
        db->open(NULL,          // Transaction pointer
                 NULL,          // Database file name
                 NULL,          // Optional logical database name
                 DB_BTREE,      // Database access method
                 DB_CREATE | DB_THREAD,     // Open flags
                 0);            // File mode (using defaults)
        
        DbMpoolFile* mp = db->get_mpf();
        ret = mp->set_flags(DB_MPOOL_NOFILE, 1);
        
        db->get_env()->lock_id(&locker_id);
        
        start();
    }
    catch(DbException &e)
    {
        db = NULL;        
        q_error("failed connecting to berkeley. %s", e.what());
    }
    catch(std::exception &e)
    {
        db = NULL;
        q_error("failed connecting to berkeley. %s", e.what());
    }
    catch (...)
    {
        db = NULL;
        q_error("failed connecting to berkeley. unknown error");
    }
    
    return (NULL != db);
}

void BerkeleyQ::disconnect()
{
    stop(); 
    
    if (NULL == db) return;
    
    try
    {
        if (locker_id != 0)
        {
            db->get_env()->lock_id_free(locker_id);
            locker_id = 0;
        }
        
        db->get_env()->close(0);
        //db->close(0);
        db = NULL;        
    }
    catch(DbException &e)
    {
        q_error("failed disconnecting from berkeley. %s", e.what());
    }
    catch(std::exception &e)
    {
        q_error("failed disconnecting from berkeley. %s", e.what());
    }
    catch (...)
    {
        q_error("failed disconnecting from berkeley. unknown error");
    }
}

void BerkeleyQ::flush()
{
    if (!this->active) return;
    
    // FIXME....    
}

#pragma mark - protected

unsigned long BerkeleyQ::size(const string& queue_name)
{
    if (!this->active) return -1;
    
    DbLock lock = acquire_lock(queue_name, DB_LOCK_READ);
    unsigned long queue_size = load_queue_size(queue_name);
    release_lock(&lock);
    return queue_size;
}

JobOption BerkeleyQ::peek(const string& queue_name)
{
    if (!this->active) return JobOption();
    
    DbLock lock = acquire_lock(queue_name, DB_LOCK_READ);
    vector<string> queue = load_queue_vector(queue_name);
    release_lock(&lock);
    return !queue.empty() ? find_job(*queue.begin()) : JobOption();
}

JobOption BerkeleyQ::pop_front(const string& queue_name)
{
    if (!active) return JobOption();
    
    JobOption job;
    DbLock lock = acquire_lock(queue_name, DB_LOCK_WRITE);
    vector<string> queue = load_queue_vector(queue_name);
    if (!queue.empty())
    {
        vector<string>::iterator head = queue.begin();
        string uid = *head;
        queue.erase(head);
        save_queue_vector(queue_name, queue);
        save_queue_size(queue_name, queue.size());
        
        job = find_job(uid);
    }
    release_lock(&lock);
    return job;
}

void BerkeleyQ::push_back(const string& queue_name, const Job& job)
{
    if (!this->active) return;
    
    save_job_record(job);
    
    DbLock lock = acquire_lock(queue_name, DB_LOCK_WRITE);
    string raw_queue = load_queue_raw(queue_name);
    raw_queue.append(" ");
    raw_queue.append(job.uid());
    save_queue_raw(queue_name, raw_queue);
    unsigned long queue_size = load_queue_size(queue_name);
    save_queue_size(queue_name, queue_size+1);
    //vector<string> queue = load_queue(queue_name);
    //queue.insert(queue.end(), job->uid());
    //save_queue(queue_name, queue);
    //save_queue_size(queue_name, queue.size());    
    release_lock(&lock);
}

JobOption BerkeleyQ::find_job(const string& uid)
{
    if (!this->active) return JobOption();
    return load_job_record(uid);
}

JobOption BerkeleyQ::update_job_status(const string& uid, const JobStatus status, const string& status_description)
{
    if (!this->active) return JobOption();
    
    JobOption job = find_job(uid);
    if (job.empty()) return JobOption();
        
    Job updated_job = job.get().withStatus(status, status_description);
    save_job_record(updated_job);
    return JobOption(updated_job);
}

void BerkeleyQ::delete_job(const string& uid)
{
    if (!this->active) return;
    
    delete_job_record(uid);
    
    vector<string> all_queues = this->queues();
    for (vector<string>::iterator queue_it = all_queues.begin(); queue_it != all_queues.end(); queue_it++)
    {
        string queue_name = *queue_it;
        DbLock lock = acquire_lock(queue_name, DB_LOCK_WRITE);
        vector<string> queue = load_queue_vector(queue_name);
        vector<string>::iterator it = std::find(queue.begin(), queue.end(), uid);
        if (it != queue.end())
        {
            queue.erase(it);
            save_queue_vector(queue_name, queue);
            save_queue_size(queue_name, queue.size());
        }
        release_lock(&lock);
    }
}

#pragma mark - private

DbLock BerkeleyQ::acquire_lock(const string& name, const db_lockmode_t lock_mode)
{
    DbLock lock;
    Dbt lock_info((void*)name.c_str(), (uint)name.size()+1);
    switch (int ret = db->get_env()->lock_get(locker_id, 0, &lock_info, lock_mode, &lock))
    {
        case 0:
            //q_log("berkeley [%s] lock acquired", queue_name.c_str());
            break;
        default:
            q_error("berkeley lock_get: unknown error");
    }
    return lock;
}

void BerkeleyQ::release_lock(DbLock* lock)
{
    switch (int ret = db->get_env()->lock_put(lock))
    {
        case 0:
            //q_log("berkeley lock released");
            break;
        default:
            q_error("berkeley lock_put: unknown error");
    }
}

unsigned long BerkeleyQ::load_queue_size(const string& queue_name)
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
        case DB_NOTFOUND:
            break;
        default:
            q_error("berkeley get: unknown error");
    }
    return value;
}

void BerkeleyQ::save_queue_size(const string& queue_name, unsigned long size)
{
    string key = build_queue_size_key(queue_name);
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
    Dbt value_accessor((void*)&size, sizeof(unsigned long));
    
    switch (int ret = db->put(NULL, &key_accessor, &value_accessor, DB_OVERWRITE_DUP))
    {
        case 0:
            break;
        default:
            q_error("berkeley put: unknown error");
    }
}

string BerkeleyQ::load_queue_raw(const string& queue_name)
{
    string key = build_queue_key(queue_name);
    return load_record(key);
}

void BerkeleyQ::save_queue_raw(const string& queue_name, const string& data)
{
    string key = build_queue_key(queue_name);
    save_record(key, data, 0);
}

vector<string> BerkeleyQ::load_queue_vector(const string& queue_name)
{
    string key = build_queue_key(queue_name);
    string result = load_record(key);
    return decode_queue(result);
}

void BerkeleyQ::save_queue_vector(const string& queue_name, vector<string> data)
{
    string key = build_queue_key(queue_name);
    string value = encode_queue(data);
    save_record(key, value, 0);
}

JobOption BerkeleyQ::load_job_record(const string& uid)
{
    string key = build_job_key(uid);
    string result = load_record(key);
    return JobCodec::decode(result);
}

void BerkeleyQ::save_job_record(const Job& job)
{
    string key = build_job_key(job.uid());
    string value = JobCodec::encode(job);
    save_record(key, value, DB_OVERWRITE_DUP);
}

void BerkeleyQ::delete_job_record(const string& uid)
{
    string key = build_job_key(uid);
    delete_record(key, 0);
}

string BerkeleyQ::load_record(const string& key)
{
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
    
    Dbt value_accessor;
    /*
    char value[VALUE_SIZE+1];
    value_accessor.set_data(value);
    value_accessor.set_ulen(VALUE_SIZE+1);
    value_accessor.set_flags(DB_DBT_USERMEM);
    */
    value_accessor.set_flags(DB_DBT_MALLOC);
    
    string result;
    switch (int ret = db->get(NULL, &key_accessor, &value_accessor, 0))
    {
        case 0:
        {
            //q_log("berkeley get: [%s] key found", key.c_str());
            uint length = value_accessor.get_size();
            char* value = (char*)value_accessor.get_data();
            result.append(value, 0, length);
            delete value;
            break;
        }
        case DB_NOTFOUND:
            q_log("berkeley get: [%s] key not found", key.c_str());
            break;
        default:
            q_error("berkeley get: unknown error");
    }
    return result;
}

void BerkeleyQ::save_record(const string& key, const string& value, const uint flags)
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
            q_error("berkeley put: unknown error");
    }   
}

void BerkeleyQ::delete_record(const string& key, const uint flags)
{
    Dbt key_accessor((void*)key.c_str(), (uint)key.size()+1);
        
    switch (int ret = db->del(NULL, &key_accessor, flags))
    {
        case 0:
            //q_log("berkeley del: [%s] key deleted", key.c_str());
            break;
        case DB_NOTFOUND:
            q_log("berkeley del: [%s] key not found", key.c_str());
            break;
        default:
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

string build_key(const char* format, ...)
{
    char key[1024];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(key, 1024, format, args);
    key[written] = '\0';
    va_end(args);
    return key;
}
