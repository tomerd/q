//
//  main.cpp
//  q-test
//
//  Created by Tomer Doron on 4/28/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <iostream>
#include <unistd.h>

#include "Qlib.h"

void test1();
void test2();

void worker1(const char** data);
void worker2(const char** data);
void observer1(const char** data);
void observer2(const char** data);

int main(int argc, const char * argv[])
{
    std::cout << q_version() << "\n";
    
    test2();
        
    return 0;
}

void test1()
{
    long now = 0;
    time(&now);
    
    void* q = NULL;
    q_connect(&q);
    
    q_observer(q, "channel1", &observer1);
    q_observer(q, "channel1", &observer2);
    q_observer(q, "channel2", &observer1);
    q_observer(q, "channel2", &observer2);
    
    q_worker(q, "channel1", &worker1);
    q_worker(q, "channel1", &worker2);
    
    q_post(q, "channel1", "test 11", 0);
    q_post(q, "channel1", "test 12", now + 4);
    q_post(q, "channel1", "test 13", 0);
    q_post(q, "channel1", "test 14", now + 6);
    q_post(q, "channel1", "test 15", 0);
    q_post(q, "channel1", "test 16", now + 4);
    
    q_post(q, "channel2", "test 21", 0);
    q_post(q, "channel2", "test 22", now + 2);
    q_post(q, "channel2", "test 23", 0);
    
    sleep(8);
    
    time(&now);
    
    q_post(q, "channel2", "test 24", 0);
    q_post(q, "channel2", "test 25", now + 2);
    q_post(q, "channel2", "test 26", 0);
    
    q_worker(q, "channel2", &worker1);
    
    sleep(4);
    
    q_disconnect(q);
    
    std::cout << "done\n";
}

void test2()
{
    long now = 0;
    time(&now);
    
    void* q = NULL;
    q_connect(&q);
    
    q_worker(q, "channel1", &worker1);

    
    for (uint index=0; index < 256; index++)
    {
        char* data = new char[1024];
        int w = sprintf(data, "test %d", index);
        data[w] = '\0';
        q_post(q, "channel1", data, 0);
        delete data;
    }
            
    sleep(10);
    
    q_disconnect(q);
    
    std::cout << "done\n";
}

void worker1(const char** data)
{
    std::cout << "worker 1 completed " << *data << "\n";
}

void worker2(const char** data)
{
    std::cout << "worker 2 completed " << *data << "\n";
}

void observer1(const char** data)
{
    std::cout << "observer 1 observed " << *data << "\n";
}

void observer2(const char** data)
{
    std::cout << "observer 2 observed " << *data << "\n";
}


