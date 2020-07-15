#ifndef BASETHREAD_H
#define BASETHREAD_H

#include <thread>
#include <condition_variable>

namespace Freehuni
{
	class BaseThread
	{
	public:
		BaseThread();
		virtual ~BaseThread();

		bool Start();
		bool Stop();
		bool IsRunning();

	protected:
		virtual void threadBody() = 0;

	private:
		void threadproc();

	private:
		std::thread mThread;
		std::condition_variable mCond;
		std::mutex mLock;
		bool mIsReady;
		bool mIsRunning;
	};
}

#endif // BASETHREAD_H
