#include <iostream>
#include <gtest/gtest.h>

using namespace std;


class GlobalEnv : public ::testing::Environment
{
public:
	GlobalEnv(){}
	~GlobalEnv(){}

	void SetUp()
	{
		printf("GlobalEnv::SetUp\n");
	}

	void TearDown()
	{
		printf("GlobalEnv::TearDown\n");
	}
};

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::AddGlobalTestEnvironment(new GlobalEnv);

	return RUN_ALL_TESTS();
}
