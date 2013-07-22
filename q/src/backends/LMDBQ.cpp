//
//  LMDBQ.cpp
//  q
//
//  Created by Tomer Doron on 7/18/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "LMDBQ.h"

#include <sstream>
#include <algorithm>
#include <cstdarg>

#include "../core/JobCodec.h"
#include "../core/Logger.h"

namespace Q
{
    static LMDBConfig parse_config(const Json::Value& configuration);
    
    static string encode_queue(const vector<string> data);
    static vector<string> decode_queue(const string& data);
    
    static string build_queue_key(const string& queue_name);
    static string build_queue_size_key(const string& queue_name);
    static string build_job_key(const string& job_uid);
    static string build_key(const char* format, ...);
    
    
    static unsigned long load_queue_size(MDB_txn* txn, MDB_dbi dbi, const string& queue_name);
    static void save_queue_size(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, unsigned long size);
    
    static string load_queue_raw(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, size_t size_hint);
    static void save_queue_raw(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, const string& data);
    
    static vector<string> load_queue_vector(MDB_txn* txn, MDB_dbi dbi, const string& queue_name);
    static void save_queue_vector(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, const vector<string> data);
    
    static JobOption load_job_record(MDB_txn* txn, MDB_dbi dbi, const string& uid);
    static void save_job_record(MDB_txn* txn, MDB_dbi dbi, const Job& job);
    static void delete_job_record(MDB_txn* txn, MDB_dbi dbi, const string& uid);
    
    static string load_record(MDB_txn* txn, MDB_dbi dbi, const string& key, size_t size_hint=0);
    static void save_record(MDB_txn* txn,MDB_dbi dbi,  const string& key, const string& value, const uint flags);
    static void delete_record(MDB_txn* txn, MDB_dbi dbi, const string& key);
    
    LMDBQ::LMDBQ(const Json::Value& configuration) : Q::Q(configuration), config(parse_config(configuration))
    {
        env = NULL;
    }
    
    LMDBQ::~LMDBQ()
    {
    }
    
    bool LMDBQ::connect()
    {
        if (NULL != env) return true;
        
        MDB_txn* txn = NULL;
        
        try
        {
            int retval = mdb_env_create(&env);
            if (0 != retval) throw QException("mdb_env_create failed " + string(mdb_strerror(retval)));
            
            retval = mdb_env_set_mapsize(env, 1024*1048576);
            if (0 != retval) throw QException("mdb_env_set_mapsize failed " + string(mdb_strerror(retval)));
            
            int flags = MDB_NOSUBDIR;
            if (config.transient) flags |= MDB_NOSYNC | MDB_NOMETASYNC;
            
            retval = mdb_env_open(env, config.path.c_str(), flags, 0664);
            if (0 != retval) throw QException("mdb_env_open failed " + string(mdb_strerror(retval)));
            
            retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            retval = mdb_open(txn, NULL, 0, &db);
            if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
            
            if (config.transient) drop();
            
            start();
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            mdb_close(env, db);
            mdb_env_close(env);
            env = NULL;
            q_error("LMDBQ::connect() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            mdb_close(env, db);
            mdb_env_close(env);
            env = NULL;
            q_error("LMDBQ::connect() failed. unknown error");
        }
        
        return (NULL != env);
    }
    
    void LMDBQ::disconnect()
    {
        stop();
        
        if (NULL == env) return;
        
        try
        {
            mdb_close(env, db);
            mdb_env_close(env);
            env = NULL;
        }
        catch(std::exception &e)
        {
            q_error("LMDBQ::disconnect() failed. %s", e.what());
        }
        catch (...)
        {
            q_error("LMDBQ::disconnect() failed. unknown error");
        }
    }
    
    // FIXME: implement this
    void LMDBQ::clear(const string& queue)
    {
    }
    
    void LMDBQ::drop()
    {
        if (!this->active) return;
        
        MDB_txn* txn = NULL;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            mdb_drop(txn, db, 1);
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::size() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::size() failed. unknown error");
        }
    }

#pragma mark - protected

    unsigned long LMDBQ::size(const string& queue_name)
    {
        if (!this->active) return -1;
        
        MDB_txn* txn = NULL;
        unsigned long size = 0;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            //DbLock lock = acquire_lock(queue_name, DB_LOCK_READ);
            size = load_queue_size(txn, db, queue_name);
            //release_lock(&lock);
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::size() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::size() failed. unknown error");
        }
        
        return size;
    }
    
    JobOption LMDBQ::peek(const string& queue_name)
    {
        if (!this->active) return JobOption();
        
        MDB_txn* txn = NULL;
        JobOption result;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            //DbLock lock = acquire_lock(queue_name, DB_LOCK_READ);
            vector<string> queue = load_queue_vector(txn, db, queue_name);
            //release_lock(&lock);
            if (!queue.empty()) result = load_job_record(txn, db, *queue.begin());
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::peek() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::peek() failed. unknown error");
        }
        
        return result;
    }
    
    JobOption LMDBQ::pop_front(const string& queue_name)
    {
        if (!active) return JobOption();
        
        MDB_txn* txn = NULL;
        JobOption result;
        
        try
        {
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));

            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            //DbLock lock = acquire_lock(queue_name, DB_LOCK_WRITE);
            
            vector<string> queue = load_queue_vector(txn, db, queue_name);
            if (!queue.empty())
            {
                vector<string>::iterator head = queue.begin();
                string uid = *head;
                queue.erase(head);
                save_queue_vector(txn, db, queue_name, queue);
                save_queue_size(txn, db, queue_name, queue.size());

                result = load_job_record(txn, db, uid);
            }
            
            //release_lock(&lock);
            
            //mdb_close(env, dbi);
                        
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::pop_front() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::pop_front() failed. unknown error");
        }
        
        return result;
    }
    
    void LMDBQ::push_back(const string& queue_name, const Job& job)
    {
        if (!this->active) return;
        
        MDB_txn* txn = NULL;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            save_job_record(txn, db, job);
            
            //DbLock lock = acquire_lock(queue_name, DB_LOCK_WRITE);
            
            unsigned long queue_size = load_queue_size(txn, db, queue_name);
            
            string raw_queue = load_queue_raw(txn, db, queue_name, queue_size*40);
            if (!raw_queue.empty()) raw_queue.append(" ");
            raw_queue.append(job.uid());
            
            /*
            std::stringstream stream;
            string raw_queue = load_queue_raw(txn, dbi, queue_name, queue_size*40);
            if (!raw_queue.empty()) stream << raw_queue << " ";
            stream << job.uid();
            */
            
            save_queue_raw(txn, db, queue_name, raw_queue);
            
            save_queue_size(txn, db, queue_name, queue_size+1);
            
            //release_lock(&lock);
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::push_back() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::push_back() failed. unknown error");
        }
    }
    
    JobOption LMDBQ::find_job(const string& uid)
    {
        if (!this->active) return JobOption();
        
        MDB_txn* txn = NULL;
        JobOption result;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            result = load_job_record(txn, db, uid);
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::find_job() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::find_job() failed. unknown error");
        }
        
        return result;
    }
    
    JobOption LMDBQ::update_job_status(const string& uid, const JobStatus status, const string& status_description)
    {
        if (!this->active) return JobOption();
        
        MDB_txn* txn = NULL;
        JobOption result;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            JobOption job = load_job_record(txn, db, uid);
            if (!job.empty())
            {            
                Job updated_job = job.get().withStatus(status, status_description);
                save_job_record(txn, db, updated_job);
                result = JobOption(updated_job);
            }
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::update_job_status() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::update_job_status() failed. unknown error");
        }
        
        return result;
    }
    
    JobOption LMDBQ::update_job_run_at(const string& uid, const long run_at)
    {
        if (!this->active) return JobOption();
        
        MDB_txn* txn = NULL;
        JobOption result;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            JobOption job = load_job_record(txn, db, uid);
            if (!job.empty())
            {
                Job updated_job = job.get().withRunAt(run_at);
                save_job_record(txn, db, updated_job);
                result = JobOption(updated_job);
            }
            
            //mdb_close(env, dbi);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::update_job_run_at() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::update_job_run_at() failed. unknown error");
        }
        
        return result;
    }
    
    void LMDBQ::delete_job(const string& uid)
    {
        if (!this->active) return;
        
        MDB_txn* txn = NULL;
        
        try
        {            
            int retval = mdb_txn_begin(env, NULL, 0, &txn);
            if (0 != retval) throw QException("mdb_txn_begin failed " + string(mdb_strerror(retval)));
            
            //MDB_dbi dbi;
            //retval = mdb_open(txn, NULL, 0, &dbi);
            //if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
            
            delete_job_record(txn, db, uid);
            
            vector<string> all_queues = this->queues();
            for (vector<string>::iterator queue_it = all_queues.begin(); queue_it != all_queues.end(); queue_it++)
            {
                string queue_name = *queue_it;
                //DbLock lock = acquire_lock(queue_name, DB_LOCK_WRITE);
                vector<string> queue = load_queue_vector(txn, db, queue_name);
                vector<string>::iterator it = std::find(queue.begin(), queue.end(), uid);
                if (it != queue.end())
                {
                    queue.erase(it);
                    save_queue_vector(txn, db, queue_name, queue);
                    save_queue_size(txn, db, queue_name, queue.size());
                }
            }
            
            //release_lock(&lock);
            
            //mdb_close(env, db);
            
            retval = mdb_txn_commit(txn);
            if (0 != retval) throw QException("mdb_txn_commit failed " + string(mdb_strerror(retval)));
        }
        catch(std::exception &e)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::delete_job() failed. %s", e.what());
        }
        catch (...)
        {
            if (NULL != txn) mdb_txn_abort(txn);
            q_error("LMDBQ::delete_job() failed. unknown error");
        }
            
    }
    
#pragma mark - private
    
    /*
    DbLock LMDBQ::acquire_lock(const string& name, const db_lockmode_t lock_mode)
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
    
    void LMDBQ::release_lock(DbLock* lock)
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
    */
    
    unsigned long load_queue_size(MDB_txn* txn, MDB_dbi dbi, const string& queue_name)
    {
        string key = build_queue_size_key(queue_name);
        const string value = load_record(txn, dbi, key, 7);
        if (value.empty()) return 0;
        
        char* end;
        unsigned long result = strtol(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != '\0' || errno == ERANGE) throw QException("queue size [" + key+ "] returned invalid value (" + value + ")");
        return result;
        
        /*
        unsigned long result = 0;
        
        MDB_dbi dbi;
        MDB_val key_accessor, value_accessor;
        
        int retval = mdb_open(txn, NULL, 0, &dbi);
        if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
        
        string key = build_queue_size_key(queue_name);
        key_accessor.mv_size = key.size(); //+1;
        key_accessor.mv_data = (void*)key.c_str();
                    
        value_accessor.mv_size = sizeof(result);
        value_accessor.mv_data = &result;
        
        retval = mdb_get(txn, dbi, &key_accessor, &value_accessor);
        if (0 != retval && MDB_NOTFOUND != retval) throw QException("mdb_get failed " + string(mdb_strerror(retval)));
        
        return result;
        */
         
        /*
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
        */
    }
    
    void save_queue_size(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, unsigned long size)
    {
        std::stringstream stream;
        stream << size;
        const string value = stream.str();
        
        string key = build_queue_size_key(queue_name);
        save_record(txn, dbi, key, value, 0);
        
        /*
        MDB_dbi dbi;
        MDB_val key_accessor, value_accessor;
        
        int retval = mdb_open(txn, NULL, 0, &dbi);
        if (0 != retval) throw QException("mdb_open failed " + string(mdb_strerror(retval)));
        
        string key = build_queue_size_key(queue_name);
        key_accessor.mv_size = key.size(); //+1;
        key_accessor.mv_data = (void*)key.c_str();
        
        value_accessor.mv_size = sizeof(size);
        value_accessor.mv_data = &size;
        
        retval = mdb_put(txn, dbi, &key_accessor, &value_accessor, 0);
        if (0 != retval) throw QException("mdb_put failed " + string(mdb_strerror(retval)));
        */
        
        /*
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
        */
    }
    
    string load_queue_raw(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, size_t size_hint)
    {
        string key = build_queue_key(queue_name);
        return load_record(txn, dbi, key, size_hint);
    }
    
    void save_queue_raw(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, const string& data)
    {
        string key = build_queue_key(queue_name);
        save_record(txn, dbi, key, data, 0);
    }
    
    vector<string> load_queue_vector(MDB_txn* txn, MDB_dbi dbi, const string& queue_name)
    {
        string key = build_queue_key(queue_name);
        string result = load_record(txn, dbi, key);
        return decode_queue(result);
    }
    
    void save_queue_vector(MDB_txn* txn, MDB_dbi dbi, const string& queue_name, vector<string> data)
    {
        string key = build_queue_key(queue_name);
        string value = encode_queue(data);
        save_record(txn, dbi, key, value, 0);
    }
    
    JobOption load_job_record(MDB_txn* txn, MDB_dbi dbi, const string& uid)
    {
        string key = build_job_key(uid);
        string result = load_record(txn, dbi, key);
        return result.empty() ? JobOption() : JobCodec::decode(result);
    }
    
    void save_job_record(MDB_txn* txn, MDB_dbi dbi, const Job& job)
    {
        string key = build_job_key(job.uid());
        string value = JobCodec::encode(job);
        save_record(txn, dbi, key, value, 0);
    }
    
    void delete_job_record(MDB_txn* txn, MDB_dbi dbi, const string& uid)
    {
        string key = build_job_key(uid);
        delete_record(txn, dbi, key);
    }
    
    string load_record(MDB_txn* txn, MDB_dbi dbi, const string& key, size_t size_hint)
    {
        MDB_val key_accessor;
        key_accessor.mv_size = key.size();
        key_accessor.mv_data = (void*)key.c_str();
        
        char value[size_hint];
        MDB_val value_accessor;
        value_accessor.mv_size = sizeof(value);
        value_accessor.mv_data = value;
        
        int retval = mdb_get(txn, dbi, &key_accessor, &value_accessor);
        
        if (MDB_NOTFOUND == retval) return "";
        if (0 != retval) throw QException("mdb_get failed " + string(mdb_strerror(retval)));
        if (0 == value_accessor.mv_size) return "";
        
        string result;
        result.assign((char*)value_accessor.mv_data, value_accessor.mv_size);
        return result;
    }
    
    void save_record(MDB_txn* txn, MDB_dbi dbi, const string& key, const string& value, const uint flags)
    {
        MDB_val key_accessor;
        key_accessor.mv_size = key.size();
        key_accessor.mv_data = (void*)key.c_str();
        
        MDB_val value_accessor;
        value_accessor.mv_size = value.size();
        value_accessor.mv_data =(void*)value.c_str();
        
        int retval = mdb_put(txn, dbi, &key_accessor, &value_accessor, flags);
        if (0 != retval) throw QException("mdb_put failed " + string(mdb_strerror(retval)));
    }
    
    void delete_record(MDB_txn* txn, MDB_dbi dbi, const string& key)
    {
        MDB_val key_accessor;        
        key_accessor.mv_size = key.size();
        key_accessor.mv_data = (void*)key.c_str();
                
        int retval = mdb_del(txn, dbi, &key_accessor, NULL);
        if (MDB_NOTFOUND == retval) return;
        if (0 != retval) throw QException("mdb_del failed " + string(mdb_strerror(retval)));
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
    
    LMDBConfig parse_config(const Json::Value& configuration)
    {
        bool transient = 0 == strcmp("true", configuration.get("transient", "true").asString().c_str());
        string path = configuration.get("path", "").asString();
        if (path.empty())
        {
            const char* dir = getenv("TMPDIR");
            path = 0 != dir ? dir : "/tmp";
        }
        path += "/qlibdb";
        
        return LMDBConfig(transient, path);
    }
}