#include "logger.h"
#include <map>
#include <string>

namespace Freehuni
{
	class WeekManager
	{
	typedef enum
		{
			eSun,
			eMon,
			eTue,
			eWed,
			eThr,
			eFri,
			eSat
		} eWEEK;

	std::map<eWEEK, std::string> mWeekString=
	{
		{eSun,"sun"},
		{eMon,"mon"},
		{eTue,"tue"},
		{eWed,"wed"},
		{eThr,"thr"},
		{eFri,"fri"},
		{eSat,"sat"}
	};

	public:
		WeekManager();

		eWEEK GetWeek()
		{
#ifdef UNIT_TEST
			return mWeekDay;
#else
			time_t t=time(0);
			struct tm* now = localtime(&t);
			return (eWEEK)now->tm_wday;
#endif
		}

		const char* GetWeekString(eWEEK week)
		{
			return mWeekString[week].c_str();
		}

		void SetCurrentWeek(eWEEK week)
		{
			mWeekDay = week;
		}

	private:
		eWEEK mWeekDay;
	};

	Logger::Logger()
	{
		mLogLevel = 0;
	}

	void Logger::SetLevel(int logLevel, std::string logFile)
	{
		mLogLevel = logLevel;
		mLogFile = logFile;

		// path 구분
		// 파일 이름
		// 확장자
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
