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

string JobCodec::encode(const Job* job)
{
    Json::Value root;
    Json::StyledWriter writer;
    
    root["uid"] = job->uid();
    root["data"] = job->data();;
    root["at"] = (int)job->at();
    root["timestamp"] = (int)job->timestamp();
    
    return writer.write(root);
}

Job* JobCodec::decode(const string& buffer)
{
    Json::Value root;
    Json::Reader reader;
    bool result = reader.parse(buffer, root);
    if (!result)
    {
        q_error("Failed to parsing json");
        return NULL;
    }
    
    string uid = root.get("uid", "").asString();
    string data = root.get("data", "").asString();
    long at = root.get("at", 0).asInt();
    long timestamp = root.get("timestamp", 0).asInt();
                                              
    return new Job(uid, data, at, timestamp);
}