#include "logger.h"
#include <map>
#include <string>
#include <iostream>
#include <utility.h>

using namespace std;

namespace Freehuni
{
	class LoggerOutput
	{
	public:
		LoggerOutput() : mNextOutput(nullptr)
		{}
		virtual void Print(const char* fmt, ...) = 0;

	protected:
		LoggerOutput* mNextOutput;
	};

	class ConsoleOutput : public LoggerOutput
	{
	public:
		ConsoleOutput()
		{}
		void Print(const char* fmt, ...) override;

	protected:


	};

	void ConsoleOutput::Print(const char* fmt, ...)
	{
		char logMessage[4096]={0};

		std::va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);

		cout << logMessage;

		if (mNextOutput != nullptr)
		{
			mNextOutput->Print(logMessage);
		}
	}


	class ColorConsoleOutput : public ConsoleOutput
	{
	public:
		ColorConsoleOutput(){}

	protected:
		void Print(const char* fmt, ...) override;
	};

	class UdpOutput : public LoggerOutput
	{
	public:
		UdpOutput(){}

	protected:
		void Print(const char* fmt, ...) override;
	};

	Logger::Logger()
	{
		mLogLevel = 0;
	}

	void Logger::SetLevel(int logLevel, std::string logFile)
	{
		mLogLevel = logLevel;
		mLogFile = logFile;

		mLogPath=".";
		Utility::ParsePath(logFile, mLogPath, mLogTitle, mLogExt);
	}

	bool Logger::WriteLog(eLEVEL elevel, const char*funcName, const int codeLine, const char* fmt, ...)
	{
		char logMessage[4096]={0};

		if (!(elevel & mLogLevel)) return false;

		std::va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);

		struct timespec tspec;
		struct tm now;

		clock_gettime(CLOCK_REALTIME, &tspec);
		localtime_r(&tspec.tv_sec, &now);

		fprintf(stderr, "[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n",
				mLevelString[elevel].c_str(),
				now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
				std::this_thread::get_id(),
				funcName, codeLine,
				logMessage);
		fflush(stderr);

		return true;
	}
}
