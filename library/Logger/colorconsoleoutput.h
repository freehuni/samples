#ifndef COLORCONSOLEOUTPUT_H
#define COLORCONSOLEOUTPUT_H
#include "consoleoutput.h"

namespace Freehuni
{
	class ColorConsoleOutput : public ConsoleOutput
	{
	public:
		ColorConsoleOutput()
		{}

		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;
	};
}

#endif // COLORCONSOLEOUTPUT_H
