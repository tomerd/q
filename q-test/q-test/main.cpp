//
//  main.cpp
//  q-test
//
//  Created by Tomer Doron on 4/28/13.
//  Copyright (c) 2013 Tomer Doron. All rights reserved.
//

#include <iostream>
#include <vector>

#include <assert.h>
#include <unistd.h>

#include "Qlib.h"

using namespace std;

static void test1(string config);
static void test2(string config);
static void test3(string config);

static bool received(string data);
static void assert_recieved(string data);
static void assert_not_recieved(string data);

static void worker1(const char** data);
static void worker2(const char** data);
static void observer1(const char** data);
static void observer2(const char** data);

vector<string> recieved_buffer;

int main(int argc, const char * argv[])
{
    cout << q_version() << "\n";

    string berkeley_config = "{ \"driver\": \"berkeley\" }";
    string redis_config = "{ \"driver\": \"redis\", \"host\": \"127.0.0.1\" }";
    
    test3(redis_config);
        
    return 0;
}

void test1(string config)
{
    void* q = NULL;
    q_connect(&q, config.c_str());
    if (NULL == q) return;
    
    q_worker(q, "channel1", &worker1);
    
    q_post(q, "channel1", "test 1", 0);
    q_post(q, "channel1", "test 2", 0);
    q_post(q, "channel1", "test 3", 0);
    
    sleep(2);
    assert_recieved("test 1");
    assert_recieved("test 2");
    assert_recieved("test 3");
    
    q_disconnect(q);
    
    cout << "done\n";
}

void test2(string config)
{
    long now = 0;
    time(&now);
    
    void* q = NULL;
    q_connect(&q, config.c_str());
    if (NULL == q) return;
    
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
    q_post(q, "channel2", "test 22", now + 11);
    q_post(q, "channel2", "test 23", 0);
    
    sleep(2);
    assert_recieved("test 11");
    assert_recieved("test 13");
    assert_recieved("test 15");
    assert_not_recieved("test 12");
    assert_not_recieved("test 16");
    assert_not_recieved("test 14");
    
    sleep(3);
    assert_recieved("test 12");
    assert_recieved("test 16");
    assert_not_recieved("test 14");    
    
    sleep(3);
    assert_recieved("test 14");
    
    time(&now);
    
    q_post(q, "channel2", "test 24", 0);
    q_post(q, "channel2", "test 25", now + 3);
    q_post(q, "channel2", "test 26", 0);
    
    q_worker(q, "channel2", &worker1);
    sleep(2);
    assert_recieved("test 21");
    assert_recieved("test 23");
    assert_recieved("test 24");
    assert_recieved("test 26");
    assert_not_recieved("test 25");
    assert_not_recieved("test 22");
    
    sleep(2);
    assert_recieved("test 25");
    assert_recieved("test 22");
    
    q_disconnect(q);
    
    cout << "done\n";
}

void test3(string config)
{
    void* q = NULL;
    q_connect(&q, config.c_str());
    if (NULL == q) return;
 
    //long now = 0;
    //time(&now);
    
    q_worker(q, "channel1", &worker1);    
    for (uint index=0; index < 100000; index++)
    {
        char* data = new char[50];
        int w = sprintf(data, "test %d", index);
        data[w] = '\0';
        q_post(q, "channel1", data, 0);
        delete data;
        
        if (index % 1000 == 0) q_worker(q, "channel1", &worker1);
    }
    
    sleep(30);
    
    q_disconnect(q);
    
    cout << "done\n";
}

void assert_recieved(string data)
{
    assert (true==received(data));
}

void assert_not_recieved(string data)
{    
    assert (false==received(data));
}

bool received(string data)
{
    vector<string>::iterator it = std::find(recieved_buffer.begin(), recieved_buffer.end(), data);
    return recieved_buffer.end() != it;
}

void worker1(const char** data)
{
    cout << "worker 1 completed " << *data << "\n";
    recieved_buffer.push_back(*data);
}

void worker2(const char** data)
{
    cout << "worker 2 completed " << *data << "\n";
    recieved_buffer.push_back(*data);    
}

void observer1(const char** data)
{
    cout << "observer 1 observed " << *data << "\n";
    recieved_buffer.push_back(*data);    
}

void observer2(const char** data)
{
    cout << "observer 2 observed " << *data << "\n";
    recieved_buffer.push_back(*data); 
}


