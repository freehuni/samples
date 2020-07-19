#include <gtest/gtest.h>
#include "logger.h"

using namespace Freehuni;


TEST(WeekTest, SetWeek)
{
	WeekManager week;
	week.SetCurrentWeek(WeekManager::eMon);
	EXPECT_EQ(week.GetWeek(), WeekManager::eMon);
	week.SetCurrentWeek(WeekManager::eFri);
	EXPECT_EQ(week.GetWeek(), WeekManager::eFri);
}

TEST(LogLevelTest, DebugFatal)
{
	Logger* logger=new Logger;

	logger->SetLevel(eDebug | eFatal);

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
	Logger* logger=new Logger;

	logger->SetLevel(eAll);

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

	logger->SetLevel(eAll, "Simple.log");

}
