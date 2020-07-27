#include <gtest/gtest.h>
#include "week.h"

using namespace Freehuni;

TEST(WeekTest, SetWeek)
{
	Week week;
	week.SetWeek(Week::eMon);
	EXPECT_EQ(week.GetWeek(), Week::eMon);
	week.SetWeek(Week::eFri);
	EXPECT_EQ(week.GetWeek(), Week::eFri);
}

TEST(WeekTest, GetWeek)
{
	Week week;

	EXPECT_STREQ(week.GetWeek(Week::eTue), "tue");


}
