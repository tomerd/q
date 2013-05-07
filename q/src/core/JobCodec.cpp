//
//  JobCodec.cpp
//  q
//
//  Created by Tomer Doron on 5/1/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "JobCodec.h"

#include "Logger.h"
#include "json.h"

string JobCodec::encode(const Job& job)
{
    Json::Value root;
    Json::StyledWriter writer;
    
    root["uid"] = job.uid();
    root["data"] = job.data();
    root["status"] = job.status();
    root["status_description"] = job.status_description();
    root["run_at"] = (int)job.run_at();
    root["timestamp"] = (int)job.timestamp();
    
    return writer.write(root);
}

JobOption JobCodec::decode(const string& buffer)
{
    Json::Value root;
    Json::Reader reader;
    bool result = reader.parse(buffer, root);
    if (!result || Json::objectValue != root.type())
    {
        q_error(string("invalid job json ").append(reader.getFormatedErrorMessages()));
        return JobOption();
    }
        
    string uid = root.get("uid", "").asString();
    string data = root.get("data", "").asString();
    JobStatus status = (JobStatus)root.get("status", JSUnknown).asInt();
    string status_description = root.get("status_description", "").asString();
    long run_at = root.get("run_at", 0).asInt();
    long timestamp = root.get("timestamp", 0).asInt();
                                              
    return JobOption(Job(uid, data, status, status_description, run_at, timestamp));
}