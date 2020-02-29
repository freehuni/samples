#include <iostream>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/if_arp.h>

using namespace std;

//#define IFACE_NAME "eno1"
#define IFACE_NAME "docker0"

static char *ethernet_mactoa(struct sockaddr *addr)
{
    static char buff[256];
    unsigned char *ptr = (unsigned char *) addr->sa_data;

    sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X",
        (ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
        (ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));

    return (buff);

}

int mxDlnaMscp_IPtoMAC( char *pDestIpStr, char **ppMacAddr )
{
    int                 s;
    struct arpreq       areq;
    struct sockaddr_in *sin;
    unsigned int DestIp = 0;
    char * mac = NULL;

    if(pDestIpStr == NULL)
    {
        printf(  "[mxDlnaMscp_IPtoMAC] ipAddress is NULL..\n");
        return -1;
    }

    DestIp = inet_addr(pDestIpStr);

    /* Get an internet domain socket. */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf( "[PCGetIPtoMAC] getSourceMacAddress socket\n");
        return -1;
    }
    printf( "[mxDlnaMscp_IPtoMAC] socket(%d)\n", s);

    /* Make the ARP request. */
    memset(&areq, 0, sizeof(areq));
    sin = (struct sockaddr_in *) &areq.arp_pa;
    sin->sin_family = AF_INET;

    sin->sin_addr.s_addr = DestIp;


    sin = (struct sockaddr_in *) &areq.arp_ha;
    sin->sin_family = ARPHRD_ETHER;

    strncpy(areq.arp_dev, IFACE_NAME, 15);

    if (ioctl(s, SIOCGARP, (caddr_t) &areq) == -1)
    {
        printf( "-- Error: unable to make ARP request, error(errno:%d)\n", errno);
        close(s);
        return -1;
    }

    mac = ethernet_mactoa(&areq.arp_ha);
    printf( "[PCGetIPtoMAC] %d:::%s\n", DestIp, 	mac);

    if(ppMacAddr)
        *ppMacAddr = strdup(mac);

    close(s);
    return 1;
}

int main()
{
    char* mac = NULL;

    mxDlnaMscp_IPtoMAC("192.168.1.1", &mac);
    printf("mac:%s\n", mac);

    return 0;
}
