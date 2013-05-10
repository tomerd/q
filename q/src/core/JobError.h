//
//  JobError.h
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#ifndef __q__JobError__
#define __q__JobError__

#include <iostream>

using namespace std;

namespace Q
{
    class JobError
    {
        
    private:
        string _description;
        
    public:
        
        JobError(const string& description);
        ~JobError();
        
        string const& description() const;
    };
}


#endif /* defined(__q__JobError__) */
