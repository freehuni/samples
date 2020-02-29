#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int main(int argc, char* argv[])
{
    struct sockaddr_in6 listeningAddr, remoteAddr;
    int clientSock = -1;
    int sd = -1;
    int ret;
    int on = 0;
    int ra = 1;
    socklen_t socklen;
    char address[1000]={0};
    char message[1000]={0};

    if (argc != 2)
    {
        printf("Usage: tcpserver <listening port>\n");
        return -1;
    }

    sd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (sd == -1)
    {
        printf("cannot create socket (err:%d)\n", errno);
        goto END_LOOP;
    }

    ret = setsockopt(sd, IPPROTO_IPV6, IPV6_V6ONLY, (void*)&on, sizeof(on));
    if (ret == -1)
    {
        printf("cannot set socket IPV6_V6ONLY option(err:%d)\n", errno);
        goto END_LOOP;
    }

    ret =  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(ra));
    if (ret == -1)
    {
        printf("cannot set socket SO_REUSEADDR option(err:%d)\n", errno);
        goto END_LOOP;
    }

    listeningAddr.sin6_family = AF_INET6;
    listeningAddr.sin6_addr = in6addr_any;
    listeningAddr.sin6_port = htons(atoi(argv[1]));

    ret = bind(sd, (struct sockaddr*)&listeningAddr, sizeof(struct sockaddr_in6));
    if (ret == -1)
    {
        printf("cannot bind (err:%d)\n", errno);
        goto END_LOOP;
    }

    ret = listen(sd, 5);
    if (ret == -1)
    {
        printf("cannot listen (err:%d)\n", errno);
        goto END_LOOP;
    }

    socklen = sizeof(struct sockaddr_in6);
    clientSock = accept(sd, (struct sockaddr*)&remoteAddr, &socklen);
    if (clientSock == -1)
    {
        printf("cannot accept the connection request (err:%d)\n", errno);
        goto END_LOOP;
    }

    if (IN6_IS_ADDR_V4MAPPED(&(remoteAddr.sin6_addr)))
    {
        printf("remote address is ipv4 mapped ipv6 address\n");
    }

    inet_ntop(remoteAddr.sin6_family, (void*)&(remoteAddr.sin6_addr), address, sizeof(address));
    printf("client address: [%s]\n",  address);

    read(clientSock, message, 1000);
    printf("receive: %s\n", message);

    close(clientSock);

END_LOOP:
    if (sd != -1) close(sd);

    return 0;
}
