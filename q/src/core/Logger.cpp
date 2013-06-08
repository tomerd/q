//
//  Logger.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <mutex>
#include <cstdarg>
#include <cstring>

#include "Logger.h"

namespace Q
{
    static std::mutex* log_mutex = new std::mutex;

    static char* replace_placeholders(const char* format, va_list args);

    void q_log(const char* format, ...)
    {        
        va_list args;
        va_start(args, format);
        char* message = replace_placeholders(format, args);
        va_end(args);
        log_mutex->lock();
        std::clog << "q: " << message << "\n";
        log_mutex->unlock();
        delete message;
    }

    void q_error(const char* format, ...)
    {    
        va_list args;
        va_start(args, format);
        char* message = replace_placeholders(format, args);
        va_end(args);
        log_mutex->lock();
        std::cerr << "q: " << message << "\n";
        log_mutex->unlock();
        delete message;    
    }

    void q_log(const std::string& message)
    {
        q_log(message.c_str());
    }

    void q_error(const std::string& message)
    {    
        q_error(message.c_str());
    }

    char* replace_placeholders(const char* format, va_list args)
    {
        unsigned long max = strlen(format)*5;
        char* message = new char[max];
        int written = vsnprintf(message, max, format, args);
        message[written] = '\0';
        return message;
    }
}