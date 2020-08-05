#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include "loggeroutput.h"

namespace Freehuni
{
class ConsoleOutput : public LoggerOutput
{
public:
	ConsoleOutput()
	{}

	bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;
};

}

#endif // CONSOLEOUTPUT_H
