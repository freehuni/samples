#include <iostream>
#include <functional>
#include <thread>
#include <vector>
#include <unistd.h>
#include <tuple>
#include <mutex>
#include <atomic>
#include <condition_variable>

using namespace std;

using WORKERS = vector<thread>;

class ThreadPool
{
public:
    ThreadPool(size_t maxThread) : mRunning(true), mMaxThread(maxThread)
    {
        mWorkers.reserve(maxThread);
        for (size_t i = 0 ; i < maxThread ; i++)
        {
            mWorkers.emplace_back([this](){this->doWork();});
        }
    }

    ~ThreadPool()
    {
        mRunning = false;
        mCV.notify_all();

        for(auto& worker : mWorkers)
        {
            worker.join();
        }

        mJobs.clear();
    }

    void doWork()
    {
        while(mRunning)
        {
            unique_lock<mutex> lock(mJobLock);
            tuple<string, function<bool(const void*)>, const void*> task;
            function<bool(const void*)> doTask;
            const void* param;
            string name;

            mCV.wait(lock, [this]()->bool{return (!mJobs.empty() || mRunning == false);});

            if (mRunning == false)
            {
                printf("exit...\n");
                return;
            }

            task = mJobs.back();
            mJobs.pop_back();
            mJobLock.unlock();

            name = get<0>(task);
            doTask = get<1>(task);
            param = get<2>(task);

            if (doTask(param) == false)
            {
                printf("the %s task failed\n", name.c_str());
            }
        }

    }
    void enqueueJob(string name, function<bool(const void*)> task, const void* param)
    {
        std::lock_guard<mutex> autolock(mJobLock);

        mJobs.push_back(make_tuple(name, task, param));

        mCV.notify_one();
    }

private:
    atomic<bool> mRunning;
    WORKERS mWorkers;
    size_t mMaxThread;
    mutex mJobLock;
    condition_variable mCV;
    vector<tuple<string, function<bool(const void*)>,const void*>> mJobs;  // name, task, param
};

int main()
{
    ThreadPool pool(10);

    for (int i = 0 ; i < 10;i++)
    {
        pool.enqueueJob("TEST",
                        [](const void* param)->bool
        {
            const char* content = static_cast<const char*>(param);
            printf("[%s:%d] content:%s\n", __FUNCTION__, __LINE__, content);
            usleep(1000000);
            return true;
        }, (const void*)"freehuni");
    }

                usleep(1000000);

    return 0;
}
