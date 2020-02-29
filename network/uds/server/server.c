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
    int clilen;

    struct sockaddr_un clientaddr, serveraddr;

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0); 
    if (sockfd < 0)
    {
        perror("socket error : ");
        exit(0);
    }    
    unlink(UDS_NAME);

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sun_family = AF_UNIX;
    strcpy(serveraddr.sun_path, UDS_NAME);

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
    {
        perror("bind error : ");
        exit(0);
    }
    clilen  = sizeof(clientaddr); 

    while(1)
    {
        char buffer[1000]={0};

        recvfrom(sockfd, (void *)buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &clilen); 
        printf("recv data: %s\n", buffer);    

    }
    close(sockfd);
    exit(0);
}
