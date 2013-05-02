//
//  QFactory.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "QFactory.h"

#include "TransientQ.h"
#include "BerkeleyQ.h"

Q* QFactory::createQ(const string& config)
{
    //return new TransientQ();
    return new BerkeleyQ();
}