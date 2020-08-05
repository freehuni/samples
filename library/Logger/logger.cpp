#include "loggeroutput.h"
#include "logger.h"
#include <map>
#include <string>
#include <iostream>
#include <utility.h>

#define COLOR_RESET "\033[0m"
#define ANSI_COLOR_RED(message)  ("\033[1;33;44m" message "\033[0m")

using namespace std;

namespace Freehuni
{

	Logger::Logger() : mLogLevel(eNone)
	{
	}

	void Logger::SetLevel(int eLevel)
	{
		mLogLevel = eLevel;
	}

	void Logger::AddOutput(LoggerOutput* output)
	{
		output->SetLogLevel(mLogLevel);
		mOutput.insert(output);
	}

	bool Logger::WriteLog(eLOG_LEVEL eLevel, const char*funcName, const int codeLine, const char* fmt, ...)
	{
		char logMessage[4096]={0};
		bool ret =true;

		if (mOutput.empty()) return false;

		std::va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);


		for(LoggerOutput* output : mOutput)
		{
			if (output->Print(eLevel, funcName, codeLine, logMessage) == false)
			{
				ret = false;
			}
		}

		return ret;
	}
}
