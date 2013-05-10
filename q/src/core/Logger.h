//
//  Logger.h
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__Logger__
#define __q__Logger__

#include <iostream>

namespace Q
{
    void q_log(const char* format, ...);
    void q_error(const char* format, ...);

    void q_log(const std::string& message);
    void q_error(const std::string& message);
}

#endif /* defined(__q__Logger__) */
