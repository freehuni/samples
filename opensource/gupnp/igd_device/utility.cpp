#include "utility.h"
#include <linux/if_link.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

Utility::Utility()
{

}


const INTERFACES Utility::getNetworkInterfaces()
{
	INTERFACES interfaces;
	struct ifaddrs *ifaddr, *ifa;
	int n;

	if (getifaddrs(&ifaddr) == -1)
	{
		return interfaces;
	}

	for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == nullptr) continue;
		if (ifa->ifa_addr->sa_family != AF_INET) continue;

		interfaces.push_back(ifa->ifa_name);
	}

	freeifaddrs(ifaddr);

	return interfaces;
}
