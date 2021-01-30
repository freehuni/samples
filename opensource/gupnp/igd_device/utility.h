#ifndef UTILITY_H
#define UTILITY_H

#include <vector>
#include <string>

using INTERFACES = std::vector<std::string>;
using INTERFACE_IT = INTERFACES::iterator;

class Utility
{
public:
	Utility();

	static const INTERFACES getNetworkInterfaces();
};

#endif // UTILITY_H
