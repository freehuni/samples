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

TEST(ParsePathTest,  Title)
{
	const string Filename="hello";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  TitleExt)
{
	const string Filename="hello.txt";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  TitleExtExt)
{
	const string Filename="hello.txt.org";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  PathTitleExt)
{
	const string Filename="./mypath/hello.txt";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  PathTitleExtExt)
{
	const string Filename="./mypath/hello.txt.org";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  PathTitle)
{
	const string Filename="./mypath/hello";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  RootPathPathTitleExt)
{
	const string Filename="/root/mypath/hello.txt";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}

TEST(ParsePathTest,  RelativePathPathTitleExt)
{
	const string Filename="./current/mypath/hello.txt.org";
	string path, title, ext;
	Utility::ParsePath(Filename, path, title, ext);

	printf("Filename:%s\n", Filename.c_str());
	printf("  path :%s\n", path.c_str());
	printf("  title:%s\n", title.c_str());
	printf("  ext  :%s\n", ext.c_str());
}
