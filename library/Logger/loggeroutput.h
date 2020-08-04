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
		std::map<eLOG_LEVEL, std::string> mLevelString ={
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


	class ConsoleOutput : public LoggerOutput
	{
	public:
		ConsoleOutput()
		{}
		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;

	};

	class ColorConsoleOutput : public ConsoleOutput
	{
	public:
		ColorConsoleOutput()
		{}
		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;
	};

	class FileOutput : public LoggerOutput
	{
	public:
		FileOutput() : mFile(nullptr), mWeekDay(-1)
		{}
		virtual ~FileOutput()
		{
			closeLogger();
		}

		bool SetLogFile(const std::string path, const std::string name, bool isWeekChanged=false);
		const std::string GetLogFile();

		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;

	private:
		void closeLogger();
		std::map<int, const char*> mWeek={{0,"sun"},{1,"mon"},{2,"tue"},{3,"wed"},{4,"thr"},{5,"fri"},{6,"sat"}};

		FILE*	mFile;
		std::string mPath;
		std::string mName;
		std::string mLogFullPath;
		int		mWeekDay;

	};

	class UdpOutput : public LoggerOutput
	{
	public:
		UdpOutput(){}
		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;
	};
}

#endif // LOGGEROUTPUT_H
