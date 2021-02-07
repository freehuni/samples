#include <iostream>

using namespace std;

#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

void EthernetList()
{

	struct ifaddrs * addrs, * tmp;
	struct sockaddr_in6 *sa6;
	struct sockaddr_in *sa4;
	char addr_str[40];


	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
		if (tmp->ifa_addr->sa_family == AF_INET6)
		{
			if (tmp->ifa_flags & IFF_LOOPBACK)
			{

			}
			else
			{
				sa6 = (struct sockaddr_in6*)tmp->ifa_addr;
				inet_ntop(AF_INET6, (void *)&sa6->sin6_addr, addr_str, sizeof(addr_str));
				printf(" - ifa_name:%s addr: [%s], scope_id:%d\n",  tmp->ifa_name, addr_str, sa6->sin6_scope_id);

			}
		}
		else if (tmp->ifa_addr->sa_family == AF_INET)
		{
			if (tmp->ifa_flags & IFF_LOOPBACK)
			{

			}
			else
			{
				sa4 = (struct sockaddr_in*)tmp->ifa_addr;
				inet_ntop(AF_INET, (void *)&sa4->sin_addr, addr_str, sizeof(addr_str));
				printf(" - ifa_name:%s addr: [%s]\n",  tmp->ifa_name, addr_str);

			}
		}

		tmp = tmp->ifa_next;
	}
	freeifaddrs(addrs);
}

#include <string.h>


void ILibGetIpAddress(struct sockaddr * addr, char * out, size_t size)
{
	char ipstr[INET6_ADDRSTRLEN] = {0,};
	inet_ntop(addr->sa_family,
			  (addr->sa_family == AF_INET ?
			   (void*)&((struct sockaddr_in*)addr)->sin_addr : (void*)&((struct sockaddr_in6 *)addr)->sin6_addr),
			  ipstr, sizeof(ipstr));

	snprintf(out, size, "%s", ipstr);
}

//I must get struct sockaddr_in from ScopeID of IPv4 Mapped IPv6 Address
bool GetIPv4AddressWithIndex(struct sockaddr_in* address,  int ethernetIndex)
{
	struct ifaddrs * addrs, * tmp;
	char addr_str[INET6_ADDRSTRLEN]={0};
	char ethernet[100]={0};
	bool ret = false;

	if (if_indextoname(ethernetIndex, ethernet) == NULL)
	{
		printf("[%s:%d] Not found ethernet(index:%d)\n", __FUNCTION__, __LINE__, ethernetIndex);
		return false;
	}

	getifaddrs(&addrs);
	tmp = addrs;

	while (tmp)
	{
		if (tmp->ifa_addr->sa_family == AF_INET && strcmp(tmp->ifa_name, ethernet) == 0)
		{
			inet_ntop(AF_INET, (void*)&((struct sockaddr_in*)tmp->ifa_addr)->sin_addr,  addr_str, sizeof(addr_str));

			memcpy(address, tmp->ifa_addr, sizeof(struct sockaddr_in));

			printf(" - ifa_name:%s addr: [%s]\n",  tmp->ifa_name, addr_str);
			ret = true;
			break;
		}

		tmp = tmp->ifa_next;
	}
	freeifaddrs(addrs);

	return ret;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in address;
	char ip[100];
	EthernetList();

	GetIPv4AddressWithIndex(&address, 2);
	ILibGetIpAddress((struct sockaddr*)&address, ip, 100);
	printf("ip:%s\n", ip);

	return 0;
}
