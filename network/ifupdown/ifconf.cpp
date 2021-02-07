#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <map>
#include <string>

using namespace std;

typedef map<string, int>	FLAG_MAP;
typedef FLAG_MAP::iterator	FLAG_MAP_IT;

static int getflags(char**flags, int count);

int main(int argc, char *argv[])
{
	int sockfd = -1;
	struct ifreq ifr;
	int flag;

	if (argc < 2)
	{
		printf("Usage: ifconf <interface> <flag1> <flag2> ...\n");
		printf("  - flags:\n");
		printf("      0, IFF_UP,IFF_BROADCAST, IFF_RUNNING, IFF_PROMISC, IFF_MULTICAST,...\n");
		printf("  - sample: ifconf wlan0 IFF_UP IFF_RUNNING IFF_MULTICAST ...\n");
		return -1;
	}

	// check interface
	if (if_nametoindex(argv[1]) == 0)
	{
		printf("unknown interface(%s)\n", argv[1]);
		return -2;
	}

	flag = getflags(&argv[2], argc - 2);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	sprintf(ifr.ifr_name, argv[1]);

	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) != 0)
	{
		printf("ioctl(SIOCGIFFLAGS) failed(errno:%d)\n", errno);
	}

	printf("before ifr_flags:%x\n", ifr.ifr_flags);

	ifr.ifr_flags =  flag;
	if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) != 0)
	{
		printf("ioctl(SIOCSIFFLAGS) failed(errno:%d)\n", errno);
	}

	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) != 0)
	{
		printf("ioctl(SIOCGIFFLAGS) failed(errno:%d)\n", errno);
	}

	printf("after  ifr_flags:%x\n", ifr.ifr_flags);

	close(sockfd);

	return 0;
}

int getflags(char**flags, int count)
{
	FLAG_MAP maps;
	FLAG_MAP_IT it;
	int i;
	int value= 0;

	maps.insert(make_pair("0"			, 0));
	maps.insert(make_pair("IFF_UP"			, (int)IFF_UP));
	maps.insert(make_pair("IFF_BROADCAST"	, (int)IFF_BROADCAST));
	maps.insert(make_pair("IFF_DEBUG"		, (int)IFF_DEBUG));
	maps.insert(make_pair("IFF_LOOPBACK"	, (int)IFF_LOOPBACK));
	maps.insert(make_pair("IFF_POINTOPOINT"	, (int)IFF_POINTOPOINT));
	maps.insert(make_pair("IFF_NOTRAILERS"	, (int)IFF_NOTRAILERS));
	maps.insert(make_pair("IFF_RUNNING"		, (int)IFF_RUNNING));
	maps.insert(make_pair("IFF_NOARP"		, (int)IFF_NOARP));
	maps.insert(make_pair("IFF_PROMISC"		, (int)IFF_PROMISC));
	maps.insert(make_pair("IFF_ALLMULTI"	, (int)IFF_ALLMULTI));
	maps.insert(make_pair("IFF_MASTER"		, (int)IFF_MASTER));
	maps.insert(make_pair("IFF_SLAVE"		, (int)IFF_SLAVE));
	maps.insert(make_pair("IFF_MULTICAST"	, (int)IFF_MULTICAST));
	maps.insert(make_pair("IFF_PORTSEL"		, (int)IFF_PORTSEL));
	maps.insert(make_pair("IFF_AUTOMEDIA"	, (int)IFF_AUTOMEDIA));
	maps.insert(make_pair("IFF_DYNAMIC"		, (int)IFF_DYNAMIC));


	for (i = 0 ; i < count ; i++)
	{
		it = maps.find(flags[i]);
		if (it != maps.end())
		{
			value += it->second;
		}
	}

	maps.clear();
	return value;;
}

