//
//  JobError.cpp
//  q
//
//  Created by Tomer Doron on 4/27/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "JobError.h"

namespace Q
{
    JobError::JobError(const string& description)
    {
        _description = description;
    }

    JobError::~JobError()
    {
    }

    string const& JobError::description() const
    {
        return _description;
    }
}