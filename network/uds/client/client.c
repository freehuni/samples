#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define UDS_NAME "/tmp/udpconnreq.tmp"

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_un serveraddr;
    int  clilen;
    char buffer[1000]={0};

    if (argc != 2)
    {
        printf("Usage: udsclient <message>\n");
        return -1;
    }

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0); 
    if (sockfd < 0)
    {
        perror("exit : ");
        exit(0);
    }
 
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, UDS_NAME);
    clilen = sizeof(serveraddr);

    if (sendto(sockfd, (void *)argv[1], strlen(argv[1]), 0, (struct sockaddr *)&serveraddr, clilen) < 0)
    {
        perror("send error : ");
        exit(0);
    }

    close(sockfd);
    exit(0);
}
