#include "basethread.h"
#include <unistd.h>

using namespace std;

BaseThread::BaseThread() : mIsReady(false), mIsRunning(false)
{
}


BaseThread::~BaseThread()
{
}

bool BaseThread::Start()
{
	unique_lock<mutex> lock(mLock);

	if (mIsRunning == true)
	{
		return true;
	}

	mThread = thread(&BaseThread::threadproc, this);

	mIsReady = true;
	cv_status ret = mCond.wait_for(lock, chrono::seconds(5));

	return (ret == cv_status::no_timeout);
}

bool BaseThread::Stop()
{
	unique_lock<mutex> lock(mLock);

	mIsRunning = false;
	if(mThread.joinable())
	{
		mThread.join();
	}

	return true;
}

bool BaseThread::IsRunning()
{
	unique_lock<mutex> lock(mLock);
	return mIsRunning;
}

void BaseThread::threadproc()
{
	while(1)
	{
		unique_lock<mutex> lock(mLock);
		if (mIsReady == true)
		{
			mIsRunning = true;
			break;
		}
		usleep(1000);
	}

	mCond.notify_one();

	while(mIsRunning)
	{
		threadBody();
		usleep(1);
	}
}
