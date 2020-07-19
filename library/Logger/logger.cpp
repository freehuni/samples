#include "logger.h"
#include <map>
#include <string>

using namespace std;

namespace Freehuni
{
	Logger::Logger()
	{
		mLogLevel = 0;
	}

	void Logger::SetLevel(int logLevel, std::string logFile)
	{
		mLogLevel = logLevel;
		mLogFile = logFile;

		if (logFile.empty()) return;

		int pos = logFile.find_last_of("/");	// Check Directory
		if (pos == string::npos) // In case of No Directory
		{
			mLogPath=".";

			pos = logFile.find_last_of(".");	// Check File Extension
			if (pos != string::npos)	// In case of existing File Extension
			{
				mLogName= logFile.substr(0, pos);
				mLogExt = logFile.substr(pos + 1, logFile.size());
			}
			else
			{
				mLogName = logFile;
			}
		}
		else
		{
			mLogPath=logFile.substr(0, pos);
		}
		// path 구분
		// 파일 이름
		// 확장자
		mLogPath;
		mLogName;
		mLogExt;
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
