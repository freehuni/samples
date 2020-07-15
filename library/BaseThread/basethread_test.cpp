#include <gtest/gtest.h>
#include "basethread.h"

using namespace Freehuni;

class Worker : public BaseThread
{
public:
	Worker()
	{}

	void threadBody()
	{
		for(int i = 0 ; i < 10 ; i++)
		{
			printf("[IDX:%d] Hello\n", i);
		}
	}
};

// --gtest_repeat=100000 --gtest_break_on_failure


TEST(ThreadTest, StartStop)
{
	Worker man;

	EXPECT_TRUE(man.Start());
	EXPECT_TRUE(man.IsRunning());
	EXPECT_TRUE(man.Stop());
}
