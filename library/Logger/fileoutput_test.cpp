#include <gtest/gtest.h>
#include <vector>
#include "fileoutput.h"

using namespace Freehuni;

TEST(FileOutputTest, Simple)
{
	FileOutput output;

	output.SetLogLevel(eAll);

	output.SetLogFile(".","All");

	output.Print(eCmd  , __FUNCTION__, __LINE__, "### eCmd ###");

	EXPECT_EQ(access(output.GetLogFile().c_str(), 0), 0);
	remove(output.GetLogFile().c_str());
}


TEST(FileOutputTest, Backup)
{
	FileOutput output;
	char backup[1000]={0};

	output.SetLogLevel(eAll);

	output.SetLogFile(".","Backup");

	output.Print(eCmd  , __FUNCTION__, __LINE__, "### eCmd ###");
	EXPECT_EQ(access(output.GetLogFile().c_str(), 0), 0);

	output.SetLogFile(".", "Backup", true);
	output.Print(eCmd  , __FUNCTION__, __LINE__, "New Backup");
	sprintf(backup, "%s.bak", output.GetLogFile().c_str());

	EXPECT_EQ(access(backup, 0), 0);

	remove(output.GetLogFile().c_str());
	remove(backup);
}


TEST(FileOutputTest, InvalidPath)
{
	FileOutput output;

	output.SetLogLevel(eAll);

	EXPECT_FALSE(output.SetLogFile("./invalid_path/test","InvalidPath"));
}
