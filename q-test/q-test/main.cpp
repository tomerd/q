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

static void test1(void* q);
static void test2(void* q);
static void test3(void* q, uint total, uint wait);
static void test4(void* q, uint total);
static void test5(void* q);
static void test6(void* q);

struct receiver
{
    static vector<string> recieved_buffer;
    static vector<string> observed_beffer;
    
    static void worker1(const char** data)
    {
        cout << "worker 1 completed " << *data << "\n";
        recieved_buffer.push_back(*data);
    }
    
    static void worker2(const char** data)
    {
        cout << "worker 2 completed " << *data << "\n";
        recieved_buffer.push_back(*data);
    }
    
    static void observer1(const char** data)
    {
        cout << "observer 1 observed " << *data << "\n";
        observed_beffer.push_back(*data);
    }
    
    static void observer2(const char** data)
    {
        cout << "observer 2 observed " << *data << "\n";
        observed_beffer.push_back(*data);
    }
    
    static bool received(string data)
    {
        vector<string>::iterator it = std::find(recieved_buffer.begin(), recieved_buffer.end(), data);
        return recieved_buffer.end() != it;
    }
    
    static bool observed(string data)
    {
        vector<string>::iterator it = std::find(observed_beffer.begin(), observed_beffer.end(), data);
        return observed_beffer.end() != it;
    }
    
    static void assert_recieved(uint number)
    {
        assert(number == recieved_buffer.size());
    }
    
    static void assert_recieved(string data)
    {
        assert(true == received(data));
    }
    
    static void assert_not_recieved(string data)
    {
        assert(false == received(data));
    }
    
    static void assert_observed(string data)
    {
        assert(true == observed(data));
    }
    
    static void assert_not_observed(string data)
    {
        assert(false == observed(data));
    }
};

vector<string> receiver::recieved_buffer = vector<string>();
vector<string> receiver::observed_beffer = vector<string>();

void clear(void* pq)
{
    q_flush(pq);
    receiver::recieved_buffer.clear();
    receiver::observed_beffer.clear();        
}

int main(int argc, const char * argv[])
{
    cout << "testing q version " << q_version() << "\n";

    string transient = "";
    string berkeley = "{ \"driver\": \"berkeley\" }";
    string redis = "{ \"driver\": \"redis\", \"host\": \"127.0.0.1\" }";
    
    void* pq = NULL;
    q_connect(&pq, transient.c_str());
    if (NULL == pq) return 1;
    
    
    /***************/
    //clear(pq);
    //test1(pq);
    
    //clear(pq);
    //test2(pq);
    
    //clear(pq);
    //test3(pq, 100000, 30);
    
    //clear(pq);
    //test4(pq, 50000);
    
    clear(pq);
    test5(pq);
    
    clear(pq);
    test6(pq);
    /***************/    
    
    q_disconnect(pq);
        
    cout << "done\n";    
    
    return 0;
}

void test1(void* q)
{    
    q_worker(q, "channel1", &receiver::worker1);
    
    q_post(q, "channel1", NULL, "test 1");
    q_post(q, "channel1", NULL, "test 2");
    q_post(q, "channel1", NULL, "test 3");
    q_post(q, "channel1", NULL, "test 4");
    q_post(q, "channel1", NULL, "test 5");
    q_post(q, "channel1", NULL, "test 6");
    q_post(q, "channel1", NULL, "test 7");
    q_post(q, "channel1", NULL, "test 8");
    q_post(q, "channel1", NULL, "test 9");
    q_post(q, "channel1", NULL, "test 10");
    
    sleep(2);
    receiver::assert_recieved("test 1");
    receiver::assert_recieved("test 2");
    receiver::assert_recieved("test 3");
    receiver::assert_recieved("test 4");
    receiver::assert_recieved("test 5");
    receiver::assert_recieved("test 6");
    receiver::assert_recieved("test 7");
    receiver::assert_recieved("test 8");
    receiver::assert_recieved("test 9");
    receiver::assert_recieved("test 10");
}

void test2(void* q)
{
    long now = 0;
    
    q_observer(q, "channel1", &receiver::observer1);
    q_observer(q, "channel1", &receiver::observer2);
    q_observer(q, "channel2", &receiver::observer1);
    q_observer(q, "channel2", &receiver::observer2);
    
    q_worker(q, "channel1", &receiver::worker1);
    q_worker(q, "channel1", &receiver::worker2);
 
    
    time(&now);    
    q_post(q, "channel1", NULL, "test 11");
    q_post(q, "channel1", NULL, "test 12", now + 4);
    q_post(q, "channel1", NULL, "test 13");
    q_post(q, "channel1", NULL, "test 14", now + 6);
    q_post(q, "channel1", NULL, "test 15");
    q_post(q, "channel1", NULL, "test 16", now + 4);
    
    q_post(q, "channel2", NULL, "test 21");
    q_post(q, "channel2", NULL, "test 22", now + 11);
    q_post(q, "channel2", NULL, "test 23");
    
    sleep(2);
    receiver::assert_recieved("test 11");
    receiver::assert_recieved("test 13");
    receiver::assert_recieved("test 15");
    receiver::assert_not_recieved("test 12");
    receiver::assert_not_recieved("test 16");
    receiver::assert_not_recieved("test 14");
    
    sleep(3);
    receiver::assert_recieved("test 12");
    receiver::assert_recieved("test 16");
    receiver::assert_not_recieved("test 14");    
    
    sleep(3);
    receiver::assert_recieved("test 14");
    
    time(&now);
    q_post(q, "channel2", NULL, "test 24");
    q_post(q, "channel2", NULL, "test 25", now + 3);
    q_post(q, "channel2", NULL, "test 26");
    
    q_worker(q, "channel2", &receiver::worker1);

    sleep(2);
    receiver::assert_recieved("test 21");
    receiver::assert_recieved("test 23");
    receiver::assert_recieved("test 24");
    receiver::assert_recieved("test 26");
    receiver::assert_not_recieved("test 25");
    receiver::assert_not_recieved("test 22");
    
    sleep(2);
    receiver::assert_recieved("test 25");
    receiver::assert_recieved("test 22");
}

void test3(void* q, uint total, uint wait)
{
    q_worker(q, "channel1", &receiver::worker1);
    
    for (uint index=0; index < total; index++)
    {
        char* data = new char[50];
        int w = sprintf(data, "test %d", index);
        data[w] = '\0';
        
        char* uid = NULL;
        q_post(q, "channel1", NULL, data, 0, &uid);
        delete uid;
        
        delete data;
        
        if (index % 5000 == 0) q_worker(q, "channel1", &receiver::worker1);
    }
    
    sleep(wait);
    receiver::assert_recieved(total);
}

void test4(void* q, uint total)
{
    for (uint index=0; index < total; index++)
    {
        char* data = new char[50];
        int w = sprintf(data, "test %d", index);
        data[w] = '\0';
        
        q_post(q, "channel1", NULL, data);
        
        delete data;        
    }
}


void test5(void* q)
{
    long now = 0;
    
    q_worker(q, "channel1", &receiver::worker1);
    
    time(&now);
    q_post(q, "channel1", "test1", "test 1", now + 2);
    q_post(q, "channel1", "test2", "test 2", now + 4);
    
    sleep(3);
    receiver::assert_recieved("test 1");
    receiver::assert_not_recieved("test 2");
    
    time(&now);
    q_reschedule(q, "test2", now + 4);
    
    sleep(2);
    receiver::assert_not_recieved("test 2");

    sleep(3);
    receiver::assert_recieved("test 2");
}

void test6(void* q)
{
    long now = 0;
    
    q_worker(q, "channel1", &receiver::worker1);
    
    time(&now);
    q_post(q, "channel1", "test1", "test 1", now + 2);
    q_post(q, "channel1", "test2", "test 2", now + 4);
    
    sleep(3);
    receiver::assert_recieved("test 1");
    receiver::assert_not_recieved("test 2");
    
    time(&now);
    q_cancel(q, "test2");
    
    sleep(3);
    receiver::assert_not_recieved("test 2");
}

