//
//  KyotoCabinetQ.cpp
//  q
//
//  Created by Tomer Doron on 7/17/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <mutex>
#include <cstdarg>

#include "KyotoCabinetQ.h"

#include "../3rd-party/json/json.h"
#include "../core/JobCodec.h"
#include "../core/Logger.h"

namespace Q
{
    struct not_found_exception : public exception
    {
        not_found_exception() {}
    };
    
    static KyotoCabinetConfig parse_config(const Json::Value& configuration);
    
    static string encode_queue(const vector<string> data);
    static vector<string> decode_queue(const string& data);

    static string build_queue_key(const string& queue_name);
    static string build_queue_size_key(const string& queue_name);
    static string build_job_key(const string& job_uid);
    static string build_key(const char* format, ...);
    
    //mutex* KyotoCabinetQ::kc_mutex = new mutex();
    
    KyotoCabinetQ::KyotoCabinetQ(const Json::Value& configuration) : Q::Q(configuration), config(parse_config(configuration))
    {
    }
    
    KyotoCabinetQ::~KyotoCabinetQ()
    {
    }
    
    bool KyotoCabinetQ::connect()
    {
        bool success = false;
        
        try
        {
            string db_name = "/Users/tomer/q.kch";
            if (!db.open(db_name, PolyDB::OWRITER | PolyDB::OCREATE)) throw q_exception(db.error().message());
            
            start();
            
            success = true;
        }
        catch (exception& e)
        {
            q_error("failed connecting to kyoto cabinet. %s", e.what());
        }
        catch (...)
        {
            q_error("failed connecting to kyoto cabinet. unknown error");
        }
        
        return success;
    }
    
    void KyotoCabinetQ::disconnect()
    {
        stop();
        
        try
        {
            if (!db.close()) throw q_exception(db.error().message());
        }
        catch (exception& e)
        {
            q_error("failed disconnecting from kyoto cabinet. %s", e.what());
        }
        catch (...)
        {
            q_error("failed disconnecting from kyoto cabinet. unknown error");
        }
    }
    
    void KyotoCabinetQ::flush()
    {
        if (!this->active) return;
        
        db.clear();
    }
    
#pragma mark - protected
    
    unsigned long KyotoCabinetQ::size(const string& queue_name)
    {
        if (!this->active) return -1;
                        
        try
        {
            string value;
            string key = build_queue_size_key(queue_name);
            if (!db.get(key, &value))
            {
                if (BasicDB::Error::Code::NOREC == db.error().code()) throw not_found_exception();
                throw q_exception("kyoto cabinet GET failed on [" + key + "] " + db.error().message());
            }
            
            char* end;
            unsigned long size = 0;
            size = strtol(value.c_str(), &end, 10);
            if (end == value.c_str() || *end != '\0' || errno == ERANGE) throw q_exception("queue counter [" + key + "] returned invalid value (" + value + ")");
            return size;
        }
        catch (not_found_exception& e)
        {
            // ignore
        }
        catch (exception& e)
        {
            q_error("size() failed. %s", e.what());
        }
        
        return 0;
    }
    
    JobOption KyotoCabinetQ::peek(const string& queue_name)
    {
        if (!this->active) return JobOption();
                
        try
        {
            string value;
            string key = build_queue_key(queue_name);
            if (!db.get(key, &value))
            {
                if (BasicDB::Error::Code::NOREC == db.error().code()) throw not_found_exception();
                throw q_exception("kyoto cabinet GET failed on [" + key + "] " + db.error().message());
            }
            vector<string> queue = decode_queue(value);
            if (!queue.empty()) return find_job(*queue.begin());
        }
        catch (not_found_exception& e)
        {
            // ignore
        }
        catch (exception& e)
        {
            q_error("peek() failed. %s", e.what());
        }
        
        return JobOption();
    }
    
    JobOption KyotoCabinetQ::pop_front(const string& queue_name)
    {
        if (!active) return JobOption();
                
        class PopVisitor : public DB::Visitor
        {
            
        public:
            string job_uid;
            
        protected:
            const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t *sp)
            {
                const string value = string(vbuf, 0, vsiz);
                if (value.empty()) return NOP;
                vector<string> queue = decode_queue(value);
                vector<string>::iterator head = queue.begin();
                job_uid = *head;
                queue.erase(head);
                const string updated_queue = encode_queue(queue);
                
                char* result = new char[updated_queue.size()];
                updated_queue.copy(result, updated_queue.size());                
                *sp = updated_queue.size();
                return result;
                
                //const string result = encode_queue(queue);
                //*sp = result.size();
                //return result.c_str()
            }
            
            const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp)
            {
                return NOP;
            }
        } pop_visitor;
        
        class DecVisitor : public DB::Visitor
        {   
            const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t *sp)
            {
                char* end;
                const string value = string(vbuf, 0, vsiz);
                unsigned long count = strtol(value.c_str(), &end, 10);
                if (end == vbuf || *end != '\0' || errno == ERANGE)
                {
                    q_error("queue counter [%s] returned invalid value (%s)", kbuf, vbuf);
                    return NOP;
                }
                std::stringstream stream;
                stream << (count-1);
                const string string_count = stream.str();
                
                char* result = new char[string_count.size()];
                string_count.copy(result, string_count.size());
                *sp = string_count.size();
                return result;
                
                //*sp = result.size();
                //return result.c_str();
            }
            
            const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp)
            {
                q_error("queue counter [%s] unexpectedly empty", kbuf);
                return NOP;
            }
        } dec_visitor;
                
        JobOption result = JobOption();
        
        db.begin_transaction();
        
        try
        {
            string queue_key = build_queue_key(queue_name);
            if (!db.accept(queue_key.c_str(), queue_key.size(), &pop_visitor, true)) throw q_exception("kyoto cabinet ACCEPT failed on [" + queue_key + "] " + db.error().message());
            
            if (!pop_visitor.job_uid.empty())
            {
                string queue_size_key = build_queue_size_key(queue_name);
                if (!db.accept(queue_size_key.c_str(), queue_size_key.size(), &dec_visitor, true)) throw q_exception("kyoto cabinet ACCEPT failed on [" + queue_size_key + "] " + db.error().message());
                
                result = find_job(pop_visitor.job_uid);
            }
        }
        catch (exception e)
        {
            q_error("pop_front() failed. %s", e.what());
        }
        
        db.end_transaction();
        
        return result;
    }
    
    void KyotoCabinetQ::push_back(const string& queue_name, const Job& job)
    {
        if (!this->active) return;
                
        class PushVisitor : public DB::Visitor
        {

        public:
            PushVisitor(const string& job_uid) : job_uid(job_uid)
            {}
        
        protected:
            const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t *sp)
            {
                if (0 == vsiz)
                {
                    //*sp = job_uid.size();
                    //return job_uid.c_str();
                    
                    char* result = new char[job_uid.size()];
                    job_uid.copy(result, job_uid.size());
                    *sp = job_uid.size();
                    return result;
                }
                                
                unsigned long size = vsiz + job_uid.size() + 1;
                char* result = new char[size];
                strncpy(result, vbuf, vsiz);
                strcpy(result + vsiz, " ");
                strcpy(result + vsiz + 1, job_uid.c_str());
                result[size] = '\0';
                *sp = size;
                return result;
                
                //const string result = value + " " + job_uid;
                //*sp = result.size();
                //return result.c_str();
            }
            
            const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp)
            {
                //*sp = job_uid.size();
                //return job_uid.c_str();
                
                char* result = new char[job_uid.size()];
                job_uid.copy(result, job_uid.size());
                *sp = job_uid.size();
                return result;
            }
            
        private:
            const string job_uid;
            
        } push_visitor(job.uid());
        
        class IncVisitor : public DB::Visitor
        {
            const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t *sp)
            {
                char* end;
                const string value = string(vbuf, 0, vsiz);
                unsigned long count = strtol(value.c_str(), &end, 10);
                if (end == vbuf || *end != '\0' || errno == ERANGE)
                {
                    q_error("queue counter [%s] returned invalid value (%s)", kbuf, vbuf);
                    return NOP;
                }
                std::stringstream stream;
                stream << (count+1);
                const string string_count = stream.str();
                //*sp = result.size();
                //return result.c_str();
                
                char* result = new char[string_count.size()];
                string_count.copy(result, string_count.size());
                *sp = string_count.size();
                return result;
            }
            
            const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp)
            {
                *sp = 1;
                return "1";
            }
        } inc_visitor;
        
        
        db.begin_transaction();
        
        try
        {
            string job_key = build_job_key(job.uid());
            if (!db.set(job_key, JobCodec::encode(job))) throw q_exception("kyoto cabinet SET failed on [" + job_key + "] " + db.error().message());
            
            string queue_key = build_queue_key(queue_name);
            if (!db.accept(queue_key.c_str(), queue_key.size(), &push_visitor, true)) throw q_exception("kyoto cabinet ACCEPT failed on [" + queue_key + "] " + db.error().message());
            
            string queue_size_key = build_queue_size_key(queue_name);
            if (!db.accept(queue_size_key.c_str(), queue_size_key.size(), &inc_visitor, true)) throw q_exception("kyoto cabinet ACCEPT failed on [" + queue_size_key + "] " + db.error().message());
        }
        catch (exception e)
        {
            q_error("push_back() failed. %s", e.what());
        }

        db.end_transaction();
    }
    
    JobOption KyotoCabinetQ::find_job(const string& uid)
    {
        if (!this->active) return JobOption();
        
        try
        {
            string value;
            string key = build_job_key(uid);
            if (!db.get(key, &value))
            {
                if (BasicDB::Error::Code::NOREC == db.error().code()) throw not_found_exception();
                throw q_exception("kyoto cabinet GET failed on [" + key + "] " + db.error().message());
            }
            return JobCodec::decode(value);
        }
        catch (not_found_exception& e)
        {
            // ignore
        }
        catch (exception& e)
        {
            q_error("find_job() failed. %s", e.what());
        }
        
        return JobOption();
    }
    
    JobOption KyotoCabinetQ::update_job_status(const string& uid, const JobStatus status, const string& status_description)
    {
        if (!this->active) return JobOption();
        
        class Visitor : public DB::Visitor
        {
            
        public:
            
            Visitor(const JobStatus status, const string& status_description) : status(status), status_description(status_description)
            {}
            
            JobOption updated_job = JobOption();
            
        protected:
            const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t *sp)
            {
                const string value = string(vbuf, 0, vsiz);
                JobOption job = JobCodec::decode(value);
                if (job.empty()) return NOP;
                updated_job = JobOption(job.get().withStatus(status, status_description));
                const string updated_job_string = JobCodec::encode(updated_job.get());
                
                char* result = new char[updated_job_string.size()];
                updated_job_string.copy(result, updated_job_string.size());
                *sp = updated_job_string.size();
                return result;
                
                //*sp = result.size();
                //return result.c_str();
            }
            
            const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp)
            {
                return NOP;
            }
            
        private:
            const JobStatus status;
            const string status_description;
            
        } visitor(status, status_description);
        
        try
        {
            string key = build_job_key(uid);
            if (!db.accept(key.c_str(), key.size(), &visitor, true)) throw q_exception("kyoto cabinet ACCEPT failed on [" + key + "] " + db.error().message());
            return visitor.updated_job;
        }
        catch (exception& e)
        {
            q_error("update_job_status() failed. %s", e.what());
        }
        
        return JobOption();
    }
    
    JobOption KyotoCabinetQ::update_job_run_at(const string& uid, const long run_at)
    {
        if (!this->active) return JobOption();
        
        class Visitor : public DB::Visitor
        {
            
        public:
            
            Visitor(const long run_at) : run_at(run_at)
            {}
            
            JobOption updated_job = JobOption();
            
        protected:
            const char* visit_full(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz, size_t *sp)
            {
                const string value = string(vbuf, 0, vsiz);
                JobOption job = JobCodec::decode(value);
                if (job.empty()) return NOP;
                updated_job = JobOption(job.get().withRunAt(run_at));
                const string updated_job_string = JobCodec::encode(updated_job.get());
                
                char* result = new char[updated_job_string.size()];
                updated_job_string.copy(result, updated_job_string.size());
                *sp = updated_job_string.size();
                return result;
                
                //*sp = result.size();
                //return result.c_str();
            }
            
            const char* visit_empty(const char* kbuf, size_t ksiz, size_t *sp)
            {
                return NOP;
            }
            
        private:
            const long run_at;
            
        } visitor(run_at);
        
        try
        {
            string key = build_job_key(uid);
            if (!db.accept(key.c_str(), key.size(), &visitor, true)) throw q_exception("kyoto cabinet ACCEPT failed on [" + key + "] " + db.error().message());
            return visitor.updated_job;
        }
        catch (exception& e)
        {
            q_error("update_job_run_at() failed. %s", e.what());
        }
        
        return JobOption();
    }
    
    void KyotoCabinetQ::delete_job(const string& uid)
    {
        if (!this->active) return;
        
        try
        {
            string value;
            string key = build_job_key(uid);
            if (!db.remove(key)) throw q_exception("kyoto cabinet REMOVE failed on [" + key + "] " + db.error().message());
        }
        catch (exception& e)
        {
            q_error("delete_job() failed. %s", e.what());
        }
    }
    
#pragma mark - private
        
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
    
    KyotoCabinetConfig parse_config(const Json::Value& configuration)
    {
        return KyotoCabinetConfig();
    }
}