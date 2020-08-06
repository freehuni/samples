#ifndef UDPOUTPUT_H
#define UDPOUTPUT_H

#include "loggeroutput.h"
#include "basethread.h"

#include <string>
#include <thread>
#include <mutex>
#include <queue>

namespace Freehuni
{
	class UdpOutput : public LoggerOutput, public BaseThread
	{
	public:
		UdpOutput();
		~UdpOutput();

		bool Open(std::string targetIP, int targetPort);
		bool Close();

		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;

	private:
		virtual void threadBody();

		long long mSeq;
		int	mLogSock;
		int mTargetIP;
		int mTargetPort;

		std::mutex mLock;
		std::condition_variable mCond;
		std::queue<std::string> mLogQueue;
	};
}

#endif // UDPOUTPUT_H
