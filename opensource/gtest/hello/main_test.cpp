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
