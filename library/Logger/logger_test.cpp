#include <gtest/gtest.h>
#include "logger.h"
#include "consoleoutput.h"


using namespace Freehuni;
using namespace std;

TEST(LogLevelTest, DebugFatal)
{
	shared_ptr<Logger> logger(new Logger);

	logger->SetLevel(eDebug | eFatal);
	logger->AddOutput(new ConsoleOutput);

	EXPECT_FALSE(LOG_PRINT(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_FALSE(LOG_INFO(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_FALSE(LOG_WARN(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_DEBUG(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_FALSE(LOG_ERROR(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_FATAL(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_FALSE(LOG_APP(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_FALSE(LOG_PRINT(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
}


TEST(LogLevelTest, All)
{
	shared_ptr<Logger> logger(new Logger);

	logger->SetLevel(eAll);
	logger->AddOutput(new ConsoleOutput);

	EXPECT_TRUE(LOG_PRINT(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_INFO(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_WARN(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_DEBUG(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_ERROR(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_FATAL(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_APP(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
	EXPECT_TRUE(LOG_PRINT(logger, "frehuni: sizeof(%d)",  sizeof(std::thread::id)));
}


TEST(LogFileTest, Simple)
{
	Logger* logger=new Logger;

	logger->SetLevel(eAll);

}

