#include <gtest/gtest.h>
#include "week.h"

using namespace Freehuni;

TEST(WeekTest, SetWeek)
{
	Week week;
	week.SetWeek(Week::eWeekMon);
	EXPECT_EQ(week.GetWeek(), Week::eWeekMon);
	week.SetWeek(Week::eWeekFri);
	EXPECT_EQ(week.GetWeek(), Week::eWeekFri);
}

TEST(WeekTest, GetWeek)
{
	Week week;

	EXPECT_STREQ(week.GetWeek(Week::eWeekTue), "tue");


}
