#include "fileoutput.h"
#include <syscall.h>
#include <unistd.h>

namespace  Freehuni
{
bool FileOutput::SetLogFile(const std::string path, const std::string name, bool isWeekChanged)
{
	struct timespec tspec;
	struct tm now;
	char buf[2000];

	clock_gettime(CLOCK_REALTIME, &tspec);
	localtime_r(&tspec.tv_sec, &now);
	mWeek.SetWeek((Week::eWEEK)now.tm_wday);

	if (*path.rbegin() =='/')
	{
		mPath = path.substr(0, path.size() - 1);
	}
	else
	{
		mPath = path;
	}

	mName = name;

	snprintf(buf, 2000, "%s/%s-%s.log", path.c_str(), name.c_str(), mWeek().c_str());
	mLogFullPath = buf;

	closeLogger();

	if (isWeekChanged == true)
	{
		// backup file 만들기
		char backup[1000];
		snprintf(backup, 1000, "%s.bak", mLogFullPath.c_str());
		rename(mLogFullPath.c_str(), backup);
	}

	mFile = fopen(buf, "a");
	if (mFile != nullptr)
	{
		fprintf(mFile,"[LogStart]==============================================================\n");
		return true;
	}

	return false;
}

const std::string FileOutput::GetLogFile()
{
	return mLogFullPath;
}

bool FileOutput::Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message)
{
	struct timespec tspec;
	struct tm now;

	clock_gettime(CLOCK_REALTIME, &tspec);
	localtime_r(&tspec.tv_sec, &now);

	if (mWeek.GetWeek() != now.tm_wday)
	{
		SetLogFile(mPath, mName, true);
		mWeek.SetWeek((Week::eWEEK)now.tm_wday);
	}

	if (mFile != nullptr)
	{
		fprintf(mFile, "[%s][%02d:%02d:%02d.%03d][%ld][%s:%d] %s\n", mLevelString[eLevel].c_str(),
				now.tm_hour, now.tm_min, now.tm_sec, (int)tspec.tv_nsec/1000000,
				syscall(SYS_gettid),
				funcName, codeLine, message);
	}

	return true;
}

void FileOutput::closeLogger()
{
	if (mFile != nullptr)
	{
		fprintf(mFile,"[LogEnd]================================================================\n");
		fclose(mFile);
	}
}
}
