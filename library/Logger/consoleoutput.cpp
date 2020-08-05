#include "consoleoutput.h"
#include <unistd.h>


namespace Freehuni
{

	bool ConsoleOutput::Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message)
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

		if (eLevel == ePrint)
		{
			fprintf(stderr, "%s\n",  message);
		}
		else
		{
			fprintf(stderr, "[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n", mLevelString[eLevel].c_str(),
					now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
					std::this_thread::get_id(),
					funcName, codeLine, message);
		}

		fflush(stderr);
		return true;
	}
}
