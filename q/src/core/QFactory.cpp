//
//  QFactory.cpp
//  q
//
//  Created by Tomer Doron on 4/30/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include "QFactory.h"

#include "TransientQ.h"
#include "BerkleyQ.h"

Q* QFactory::createQ()
{
    //return new TransientQ();
    return new BerkleyQ();
}