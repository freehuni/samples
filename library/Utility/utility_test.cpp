#include <gtest/gtest.h>
#include "utility.h"
#include <regex>

using namespace std;

TEST(GetTokenTest, Simple)
{
	vector<string> tokens;

	tokens = Utility::GetTokens("00:aa:11:bb:22:cc:33", ':');
	for(string token: tokens)
	{
		printf("token:%s\n", token.c_str());
	}

	EXPECT_EQ(tokens.size(), 6);


}


TEST(RegexTest, Simple)
{
	string str="aaa:bbb:ccc:ddd::111";
	regex reg(":+");

	sregex_token_iterator it(str.begin(), str.end(), reg, -1);
	sregex_token_iterator end;

	vector<string> vec(it, end);

	for(string text : vec)
	{
		cout << "TOKENS:" <<text << endl;
	}
}
