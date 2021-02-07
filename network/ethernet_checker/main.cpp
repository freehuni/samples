#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#define ETHTOOL_GLINK                   0x0000000a

typedef __uint32_t __u32;
struct ethtool_value
{
  __u32 cmd;
  __u32 data;
};

int main (int argc, char **argv)
{
	// 이더넷 데이터 구조체
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	struct sockaddr_in6 *sin6;
	struct sockaddr *sa;
	// 이더넷 설정 구조체
	struct ifconf ifcfg;
	struct ethtool_value edata;
	int fd;
	int n;
	int numreqs = 30;
	fd = socket (AF_INET, SOCK_DGRAM, 0);
	int link;

	memset (&ifcfg, 0, sizeof (ifcfg));
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof (struct ifreq) * numreqs;
	ifcfg.ifc_buf = (__caddr_t)malloc (ifcfg.ifc_len);
	for (;;)
	{
		ifcfg.ifc_len = sizeof (struct ifreq) * numreqs;
		ifcfg.ifc_buf = (__caddr_t)realloc (ifcfg.ifc_buf, ifcfg.ifc_len);
		if (ioctl (fd, SIOCGIFCONF, (char *) &ifcfg) < 0)
		{
			perror ("SIOCGIFCONF ");
			exit(0);
		}
		break;
	}

	// 네트워크 장치의 정보를 얻어온다.
	ifr = ifcfg.ifc_req;
	for (n = 0; n < ifcfg.ifc_len; n += sizeof (struct ifreq))
	{
		// 주소값을 출력하고 링크 상태 확인
		sin = (struct sockaddr_in *) &ifr->ifr_addr;
		printf ("[%s]\n", ifr->ifr_name);
		printf ("IP    %s\n", inet_ntoa (sin->sin_addr));
		edata.cmd = ETHTOOL_GLINK;
		ifr->ifr_data = (caddr_t) & edata;
		link = ioctl (fd, SIOCETHTOOL, (char *) ifr);
		printf ("Link : %s\n\n", edata.data ? "connected" : "not connect");
		ifr++;
	}
}
