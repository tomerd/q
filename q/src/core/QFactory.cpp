//
//  QFactory.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "QFactory.h"

#include "../3rd-party/json/Json.h"
#include "Logger.h"

#include "../transient/TransientQ.h"
#include "../berkeley/BerkeleyQ.h"
#include "../redis/RedisQ.h"

namespace Q
{    
    typedef enum QType
    {
        QTUndefined,
        QTTransient,
        QTBerkeley,
        QTRedis
    } QType;

    static Json::Value parse_config(const string& configuration);
    static QType parse_driver(const string& driver);
    static bool iequals(const string& a, const string& b);

    bool QFactory::createQ(Q** qp, const string& configuration)
    {
        try
        {
            Json::Value json_config = !configuration.empty() ? parse_config(configuration) : Json::Value();
            string driver = json_config.get("driver", "").asString();
            QType q_type = parse_driver(driver);
            
            switch (q_type)
            {
                case QTTransient:
                    *qp = new TransientQ(configuration);
                    break;
                case QTBerkeley:
                    *qp = new BerkeleyQ(json_config);
                    break;
                case QTRedis:
                    *qp = new RedisQ(json_config);
                    break;
                default:
                    *qp = new TransientQ(configuration);
                    break;
            }
        }
        catch (exception& e)
        {
            *qp = NULL;        
            q_error("q initialization error. %s", e.what());
        }
        catch (...)
        {
            *qp = NULL;
            q_error("unknown q initialization error");
        }
        return (NULL != *qp);
    }

    QType parse_driver(const string& driver)
    {
        if (driver.empty()) return QTUndefined;
        if (iequals(driver, "transient")) return QTTransient;
        if (iequals(driver, "berkeley")) return QTBerkeley;
        if (iequals(driver, "redis")) return QTRedis;
        return QTUndefined;
    }

    Json::Value parse_config(const string& configuration)
    {
        Json::Value root;
        Json::Reader reader;
        bool result = reader.parse(configuration, root);
        if (!result) throw q_exception(string("failed parsing configuration json. ").append(reader.getFormatedErrorMessages()));
        return root;
    }

    bool iequals(const string& a, const string& b)
    {
        unsigned long sz = a.size();
        if (b.size() != sz)
            return false;
        for (unsigned int i = 0; i < sz; ++i)
            if (tolower(a[i]) != tolower(b[i]))
                return false;
        return true;
    }
}