#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <string>

typedef std::vector<std::string>	INTERFACES;
typedef INTERFACES::iterator		INTERFACE_IT;

class utility
{
public:
	utility();

	static bool getNetworkInterfaces(INTERFACES* interfaces);
};

#endif // UTILITY_H
