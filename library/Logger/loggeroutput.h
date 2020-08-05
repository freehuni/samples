#ifndef LOGGEROUTPUT_H
#define LOGGEROUTPUT_H

#include "logger.h"

namespace Freehuni
{
	class LoggerOutput
	{
	public:
		LoggerOutput() : mLogLevel(eNone)
		{}
		virtual ~LoggerOutput(){}

		virtual bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) = 0;
		void SetLogLevel(int eLevel)
		{
			mLogLevel = eLevel;
		}

	protected:
		std::map<eLOG_LEVEL, std::string> mLevelString =
		{
			{eCmd, "CMD"},
			{eInfo, "INF"},
			{eWarn, "WRN"},
			{eDebug, "DBG"},
			{eError, "ERR"},
			{eFatal, "FAT"},
			{eApp, "APP"},
			{ePrint, "PRT"},
			};
		int	mLogLevel;
	};
}

#endif // LOGGEROUTPUT_H
