#include <iostream>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <unistd.h>
#include <atomic>

using namespace std;

condition_variable g_event;
mutex g_lock;
atomic<bool>&& bRunning = false;

void thread_producer()
{        
    printf("After 1000 msec, send a signal to consumer.\n");
    usleep(1000000);

    // If you call notify_one before calling g_event.wait, the signal sent by notify_one is ignored.
    g_event.notify_one();
    printf("send signal\n");
}

void thread_consumer()
{
    unique_lock<mutex> lock(g_lock);
    std::cv_status retCond;
    bool ret;;

    printf("[%s:%d] wait...\n", __FUNCTION__, __LINE__);

    g_event.wait(lock);
    //g_event.wait(lock, []()->bool{ return bRunning;});
    //ret = g_event.wait_for(lock, chrono::milliseconds(5000), []()->bool{return bRunning;});
    //retCond = g_event.wait_for(lock, chrono::milliseconds(1000));

    printf(" Do something...\n");
}

int main()
{    
    thread producer(thread_producer);
    thread consumer(thread_consumer);

    consumer.join();
    producer.join();
    cout << "Hello World!" << endl;
    return 0;
}
