#if 1
#include <gtest/gtest.h>


int sum(int a, int b)
{
	return a+b;
}

TEST(MyTestCase1, ExpectTest1)
{
	ASSERT_EQ(100, sum(10, 90)) << "first";
	EXPECT_EQ(100, sum(10, 90)) << "second";
	EXPECT_EQ(100, sum(10, 90));
	EXPECT_EQ(100, sum(10, 90));
}

TEST(MyTestCase1, ExpectTest2)
{
	EXPECT_EQ(100, sum(10, 90));
	EXPECT_EQ(101, sum(10, 90)) << "not equaled!";
	EXPECT_EQ(100, sum(10, 90));
}

void TestDeath()
{
	printf("Hello");
	assert(0);
}

void TestDeath3()
{
	fprintf(stderr,"Exit");
	abort();
}

TEST(DeathSuite, test1)
{
	//std::set_terminate([](){std::cout << "Goodbye cruel world\n"; /*std::abort();*/ });

	EXPECT_DEATH(std::terminate(), ".*");

}
TEST(DeathSuite, test2)
{
	EXPECT_DEATH(TestDeath(), "\\d");
}

TEST(DeathSuite, test3)
{
	EXPECT_DEATH(TestDeath3(), "Exit");
}

void testThrow()
{
	throw 1;
}

void testNoThrow()
{

}

TEST(CatchSuite, test1)
{
	EXPECT_ANY_THROW(testThrow());
}

TEST(CatchSuite, test2)
{
	EXPECT_ANY_THROW(testNoThrow());
}

class MyFixture : public ::testing::Test
{
public:
	MyFixture()
	{}
	virtual ~MyFixture()
	{}

	void SetUp()
	{
		printf("setup\n");
	}

	void TearDown()
	{
		printf("teardown\n");
	}

	bool DoTest(int value)
	{
		if (value < 0) return false;

		return true;
	}
};

TEST_F(MyFixture, FixtureTest)
{
	int value = 1000;

	EXPECT_TRUE(DoTest(value)) << "DoTest failed!";

}

#endif
