#include "loggeroutput.h"
#include <stdio.h>
#include <stdarg.h>
#include <iostream>

using namespace std;

namespace Freehuni
{
	void ConsoleOutput::Print(const char* fmt, ...)
	{
		char logMessage[4096]={0};

		va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);

		cout << logMessage;
	}

	void ColorConsoleOutput::Print(const char* fmt, ...)
	{
		char logMessage[4096]={0};

		va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);

		cout << logMessage;
	}

	void ColorConsoleOutput::Print( eForeColor foreColor, eBackColor backColor, const char* fmt, ...)
	{
		char logMessage[4096]={0};

		va_list arg;
		va_start(arg, fmt);
		vsnprintf(logMessage, 4096, fmt, arg);
		va_end(arg);

		printf("\033[1;%d;%dm%s\033[0m\n",foreColor,backColor, logMessage);

	}
}
