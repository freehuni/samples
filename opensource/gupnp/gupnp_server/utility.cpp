#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

#include "utility.h"

using namespace std;

utility::utility()
{

}

bool utility::getNetworkInterfaces(INTERFACES* interfaces)
{
	struct ifaddrs *ifaddr, *ifa;
	int n;

	if (interfaces == NULL)
	{
		printf("invalid args!\n");
		return false;
	}

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		return false;
	}

	interfaces->clear();
	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++)
	{
		if (ifa->ifa_addr == NULL) continue;
		if (ifa->ifa_addr->sa_family != AF_INET) continue;

		interfaces->push_back(ifa->ifa_name);
	}

	freeifaddrs(ifaddr);

	return true;
}

