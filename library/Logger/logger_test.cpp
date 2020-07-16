#include <gtest/gtest.h>
#include "logger.h"

using namespace Freehuni;


TEST(LoggerTest, WriteLog)
{
	Logger* logger=new Logger;

	logger->SetLevel(eDebug | eFatal);

	LOG_PRINT(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_INFO(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_WARN(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_DEBUG(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_ERROR(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_FATAL(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_APP(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
	LOG_PRINT(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id));
}
