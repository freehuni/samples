#ifndef LOGGER_H
#define LOGGER_H

#include <cstdarg>
#include <stdio.h>
#include <map>
#include <string>
#include <thread>
#include <fstream>

#define LOG_COMMAND(logger, fmt,...)	logger->WriteLog(eCmd, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(logger, fmt,...)		logger->WriteLog(eInfo, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(logger, fmt,...)		logger->WriteLog(eWarn, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(logger, fmt,...)		logger->WriteLog(eDebug, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(logger, fmt,...)		logger->WriteLog(eError, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(logger, fmt,...)		logger->WriteLog(eFatal, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_APP(logger, fmt,...)		logger->WriteLog(eApp, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_PRINT(logger, fmt,...)		logger->WriteLog(ePrint, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

/*
일주일단위로 생성되는 로그파일이 Rotation된다.
*/

namespace Freehuni
{
	typedef enum
	{
		eCmd =1,
		eInfo = 2,
		eWarn = 4,
		eDebug = 8,
		eError = 16,
		eFatal = 32,
		eApp = 64,
		ePrint=128,
		eAll=0xff
	} eLEVEL;

	class Logger
	{
		std::map<eLEVEL, std::string> mLevelString ={
			{eCmd, "CMD"},
			{eInfo, "INF"},
			{eWarn, "WRN"},
			{eDebug, "DBG"},
			{eError, "ERR"},
			{eFatal, "FAT"},
			{eApp, "APP"},
			{ePrint, "PRT"},
			};
		int mLogLevel;
		std::string mLogFile;
	public:
		Logger();

		void SetLevel(int logLevel, std::string logFile="");
		bool WriteLog(eLEVEL elevel, const char*funcName, const int codeLine, const char* fmt, ...);

	private:
		std::string mPath;
		std::string mName;
		std::string mFullLogFile;
		std::ofstream mOutFile;
	};
}
#endif // LOGGER_H
