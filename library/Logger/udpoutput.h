#ifndef UDPOUTPUT_H
#define UDPOUTPUT_H

#include "loggeroutput.h"

namespace Freehuni
{
	class UdpOutput : public LoggerOutput
	{
	public:
		UdpOutput(){}

		bool Print(eLOG_LEVEL eLevel, const char* funcName, const int codeLine, char* message) override;
	};
}

#endif // UDPOUTPUT_H
