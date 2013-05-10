//
//  JobCodec.h
//  q
//
//  Created by Tomer Doron on 5/1/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__JobCodec__
#define __q__JobCodec__

#include <iostream>

#include "Job.h"

using namespace std;

namespace Q
{
    class JobCodec
    {
        
    public:
        static string encode(const Job& job);
        static JobOption decode(const string& data);
    };
}

#endif /* defined(__q__JobCodec__) */

