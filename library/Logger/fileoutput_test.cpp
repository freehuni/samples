#include <gtest/gtest.h>
#include <vector>
#include "fileoutput.h"

using namespace Freehuni;

TEST(FileOutputTest, All)
{
	FileOutput output;

	output.SetLogLevel(eAll);

	output.SetLogFile(".","All");

	output.Print(eCmd  , __FUNCTION__, __LINE__, "### eCmd ###");
	output.Print(ePrint, __FUNCTION__, __LINE__, "### ePrint ###");
	output.Print(eInfo , __FUNCTION__, __LINE__, "### eInfo ###");
	output.Print(eWarn , __FUNCTION__, __LINE__, "### eWarn ###");
	output.Print(eDebug, __FUNCTION__, __LINE__, "### eDebug ###");
	output.Print(eError, __FUNCTION__, __LINE__, "### eError ###");
	output.Print(eFatal, __FUNCTION__, __LINE__, "### eFatal ###");
	output.Print(eApp  , __FUNCTION__, __LINE__, "### eApp ###");

	EXPECT_EQ(access(output.GetLogFile().c_str(), 0), 0);
	remove(output.GetLogFile().c_str());
}
