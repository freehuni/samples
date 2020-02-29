#include <stdio.h>
#include<sys/socket.h>
#include<errno.h>
#include<netdb.h>
#include<arpa/inet.h>

int hostname_to_ip(char *hostname , char* service, sa_family_t family,  char *ip)
{
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *sin;
    struct sockaddr_in6 *sin6;
    int rv;

    printf("[%s:%d] hostname:%s service:%s family:%d\n", __FUNCTION__, __LINE__, hostname, service, family);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;

    if ( (rv = getaddrinfo( hostname , service , &hints , &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        snprintf(ip, sizeof(ip), hostname);
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if (p->ai_addr->sa_family == AF_INET)
        {
            sin = (struct sockaddr_in*) p->ai_addr;
            inet_ntop(sin->sin_family, &sin->sin_addr, ip, INET6_ADDRSTRLEN);
        }
        else if (p->ai_addr->sa_family == AF_INET6)
        {
            sin6 = (struct sockaddr_in6*) p->ai_addr;
            inet_ntop(sin6->sin6_family, &sin6->sin6_addr, ip, INET6_ADDRSTRLEN);
        }
        else
        {
            printf("[ERROR] Unknown family type:%d\n", p->ai_addr->sa_family);
        }
    }

    freeaddrinfo(servinfo);
    return 0;
}


int main()
{
    char *host="stun.acs.zaq.ad.jp.dfd";
   // char *host="dfkljdf.com.jp";
    char ip[1000]={0};
    int i;

    //for (i = 0 ; i < 10 ; i++)
    {
        hostname_to_ip(host, "http", AF_INET, ip);
    }

    printf("gai str:%s\n", gai_strerror(EAI_AGAIN));

    printf("host:%s\n", host);
    printf("ip:%s\n", ip);

    return 0;
}
