#include "logger.h"
#include <map>
#include <string>
#include <iostream>
#include <utility.h>

using namespace std;

namespace Freehuni
{

	Logger::Logger()
	{
		mLogLevel = 0;

	}

	void Logger::SetLevel(int logLevel)
	{
		mLogLevel = logLevel;
	}

	void Logger::AddOutput(LoggerOutput* output)
	{
		mOutput.insert(output);
	}

	bool Logger::WriteLog(eLEVEL eLevel, const char*funcName, const int codeLine, const char* fmt, ...)
	{
		char logMessage[4096]={0};

		if (!(eLevel & mLogLevel)) return false;
		if (mOutput.empty()) return false;

		std::va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);

		struct timespec tspec;
		struct tm now;

		clock_gettime(CLOCK_REALTIME, &tspec);
		localtime_r(&tspec.tv_sec, &now);

		for(LoggerOutput* output : mOutput)
		{
			output->Print("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n",
						 mLevelString[eLevel].c_str(),
						 now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
						 std::this_thread::get_id(),
						 funcName, codeLine,
						 logMessage);
		}

		return true;
	}
}
