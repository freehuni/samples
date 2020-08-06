#include <gtest/gtest.h>
#include <vector>
#include "udpoutput.h"

using namespace Freehuni;

TEST(UdpOutputTest, Simple)
{
	UdpOutput output;

	output.SetLogLevel(eAll);

	output.Print(eCmd  , __FUNCTION__, __LINE__, "### eCmd ###");
}
