#include <gtest/gtest.h>
#include "loggeroutput.h"

using namespace Freehuni;

TEST(ConsoleOutputTest, Simple)
{
	ConsoleOutput output;

	output.Print("namne: %s", "freehuni");
}


TEST(ColoeConsoleOutputTest, Simple)
{
	ColorConsoleOutput output;

	output.Print(
				ColorConsoleOutput::eRed,
				ColorConsoleOutput::eBackYellow ,
				"Color Red/Yellow namne: %s", "freehuni");
}
