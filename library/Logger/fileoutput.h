#ifndef FILEOUTPUT_H
#define FILEOUTPUT_H

#include "loggeroutput.h"
#include <week.h>

namespace Freehuni
{
class FileOutput : public LoggerOutput
{
public:
	FileOutput() : mFile(nullptr)
	{
		mWeek.SetWeek(Week::eWeekUnknown);
	}

	virtual ~FileOutput()
	{
		closeLogger();
	}

	bool SetLogFile(const std::string path, const std::string name, bool isWeekChanged=false);
	const std::string GetLogFile();

	bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;

private:
	void closeLogger();
	//std::map<int, const char*> mWeek={{0,"sun"},{1,"mon"},{2,"tue"},{3,"wed"},{4,"thr"},{5,"fri"},{6,"sat"}};

	FILE*	mFile;
	std::string mPath;
	std::string mName;
	std::string mLogFullPath;
	//int		mWeekDay;
	Week	mWeek;

};

}

#endif // FILEOUTPUT_H
