#include "udpoutput.h"

using namespace std;

namespace Freehuni
{
	UdpOutput::UdpOutput()
	{
		BaseThread::Start();
	}

	UdpOutput::~UdpOutput()
	{
		BaseThread::Stop();
	}

	bool UdpOutput::Open(std::string targetIP, int targetPort)
	{
		return true;
	}

	bool UdpOutput::Close()
	{
		return true;
	}

	bool UdpOutput::Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message)
	{
		if ((eLevel & mLogLevel) == 0)
		{
			return false;
		}

		char logMessage[5000]={0};
		struct timespec tspec;
		struct tm now;

		clock_gettime(CLOCK_REALTIME, &tspec);
		localtime_r(&tspec.tv_sec, &now);

		if (eLevel == ePrint)
		{
			snprintf(logMessage, 5000, "%s\n",  message);
		}
		else
		{
			snprintf(logMessage, 5000, "[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n", mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					std::this_thread::get_id(),
					funcName, codeLine, message);
		}

		unique_lock<mutex> lock(mLock);
		mLogQueue.push(logMessage);
		mCond.notify_one();

		return true;
	}

	void UdpOutput::threadBody()
	{
		unique_lock<mutex> lock(mLock);

		mCond.wait_for(lock, chrono::milliseconds(1000));

		if (mLogQueue.empty())
		{
			return;
		}

		string message = mLogQueue.front();
		mLogQueue.pop();

		printf("%s", message.c_str());
	}
}

