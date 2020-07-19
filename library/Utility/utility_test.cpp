#include <gtest/gtest.h>
#include "utility.h"
#include <regex>

using namespace std;

TEST(GetTokenTest, Simple)
{
	vector<string> tokens;

	tokens = Utility::GetTokens("00:aa:11:bb:22:cc", ':');
	for(string token: tokens)
	{
		printf("token:%s\n", token.c_str());
	}

	EXPECT_EQ(tokens.size(), 6);
}

TEST(GetTokenTest, DoubleColon)
{
	vector<string> tokens;

	tokens = Utility::GetTokens("00:aa:11:bb:22::cc", ':');
	for(string token: tokens)
	{
		printf("token:%s\n", token.c_str());
	}

	EXPECT_EQ(tokens.size(), 7);
}
