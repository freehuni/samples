#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>

bool check_wireless(const char* ifname, char* protocol)
{
	int sockfd = -1;
	struct iwreq pwrq;
	memset(&pwrq, 0, sizeof(pwrq));
	strncpy(pwrq.ifr_name, ifname, IFNAMSIZ);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		goto ErrBlock;
	}

	if (ioctl(sockfd, SIOCGIWNAME, &pwrq) == -1)
	{
		goto ErrBlock;
	}

	if (protocol) strncpy(protocol, pwrq.u.name, IFNAMSIZ);
	close(sockfd);

	return true;

ErrBlock:

	if (sockfd != -1 ) close(sockfd);

	return false;
}

bool get_ssid_of_wifi(char* name_wlan, 	char  ssid[IW_ESSID_MAX_SIZE+1])
{
	int sockfd = -1;

	struct iwreq wreq;
	memset(&wreq, 0, sizeof(struct iwreq));
	wreq.u.essid.length = IW_ESSID_MAX_SIZE+1;
	sprintf(wreq.ifr_name, name_wlan);

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		goto ErrBlock;
	}

	wreq.u.essid.pointer = ssid;
	if (ioctl(sockfd,SIOCGIWESSID, &wreq) == -1)
	{
		goto ErrBlock;
	}

	close(sockfd);

	return (strlen(ssid) > 0);

ErrBlock:
	if (sockfd != -1)
	{
		close(sockfd);
	}

	return false;
}

int main(int argc, char const *argv[])
{
	char  ssid[IW_ESSID_MAX_SIZE+1]={0};
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		return -1;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		char protocol[IFNAMSIZ]  = {0};

		if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET) continue;

		if (check_wireless(ifa->ifa_name, protocol))
		{
			printf("interface %s is wireless: %s\n", ifa->ifa_name, protocol);
			if (ifa->ifa_flags & IFF_UP)
			{
				if (get_ssid_of_wifi(ifa->ifa_name, ssid) == true)
				{
					printf(" - %s is connected to %s!\n", ifa->ifa_name, ssid);
				}
				else
				{
					printf(" - Though %s is up, it is NOT connected to wifi\n", ifa->ifa_name);
				}
			}
			else
			{
				printf(" - %s is down!\n", ifa->ifa_name);
			}
		}
		else
		{
			printf("interface %s is not wireless\n", ifa->ifa_name);
		}
	}

	freeifaddrs(ifaddr);
	return 0;
}
