#include <gtest/gtest.h>
#include <vector>
#include "colorconsoleoutput.h"
#include "fileoutput.h"

using namespace Freehuni;
using namespace std;

TEST(ConsoleOutputTest, Simple)
{
	ConsoleOutput output;

	output.Print(Freehuni::eInfo, __FUNCTION__, __LINE__,	"ConsoleOutputTest.Simple");
}


TEST(ColoeConsoleOutputTest, Simple)
{
	ColorConsoleOutput output;

	output.Print(Freehuni::eInfo, __FUNCTION__, __LINE__,	"ColoeConsoleOutputTest.Simple");
}


TEST(ColoeConsoleOutputTest, All)
{
	ColorConsoleOutput output;

	output.SetLogLevel(eAll);

	output.Print(eCmd  , __FUNCTION__, __LINE__, "### eCmd ###");
	output.Print(ePrint, __FUNCTION__, __LINE__, "### ePrint ###");
	output.Print(eInfo , __FUNCTION__, __LINE__, "### eInfo ###");
	output.Print(eWarn , __FUNCTION__, __LINE__, "### eWarn ###");
	output.Print(eDebug, __FUNCTION__, __LINE__, "### eDebug ###");
	output.Print(eError, __FUNCTION__, __LINE__, "### eError ###");
	output.Print(eFatal, __FUNCTION__, __LINE__, "### eFatal ###");
	output.Print(eApp  , __FUNCTION__, __LINE__, "### eApp ###");
}

