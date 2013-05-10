//
//  QFactory.h
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__QFactory__
#define __q__QFactory__

#include <iostream>

#include "Q.h"

namespace Q
{
    class QFactory
    {
        
    public:
        
        static bool createQ(Q** pq, const string& configuration);
        
    };
}

#endif /* defined(__q__QFractory__) */
