#include "colorconsoleoutput.h"
#include <syscall.h>
#include <unistd.h>

#define COLOR_COMMAND(message)  ("\033[2;30;43m" message "\033[0m")
#define COLOR_PRINT(message)  ("\033[1;36m" message "\033[0m")
#define COLOR_INFO(message)  ("\033[1;37m" message "\033[0m")
#define COLOR_WARN(message)  ("\033[1;33m" message "\033[0m")
#define COLOR_DEBUG(message)  ("\033[2;37m" message "\033[0m")
#define COLOR_ERROR(message)  ("\033[1;31m" message "\033[0m")
#define COLOR_FATAL(message)  ("\033[1;37;41m" message "\033[0m")
#define COLOR_APP(message)  ("\033[1;34m" message "\033[0m")

namespace Freehuni
{


bool ColorConsoleOutput::Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message)
{
	if ((eLevel & mLogLevel) == 0)
	{
		return false;
	}

	char logMessage[4096]={0};
	struct timespec tspec;
	struct tm now;

	clock_gettime(CLOCK_REALTIME, &tspec);
	localtime_r(&tspec.tv_sec, &now);

	switch(eLevel)
	{
		case eCmd:
			fprintf(stderr, COLOR_COMMAND("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case eInfo:
			fprintf(stderr, COLOR_INFO("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case eWarn:
			fprintf(stderr, COLOR_WARN("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case eDebug:
			fprintf(stderr, COLOR_DEBUG("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case eError:
			fprintf(stderr, COLOR_ERROR("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case eFatal:
			fprintf(stderr, COLOR_FATAL("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case eApp:
			fprintf(stderr, COLOR_APP("[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n"), mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					syscall(SYS_gettid),
					funcName, codeLine, message);
			break;

		case ePrint:
			fprintf(stderr, COLOR_PRINT("%s\n"),  message);
			break;
	}

	fflush(stderr);
	return true;
}

}
