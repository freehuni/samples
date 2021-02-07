/*  Copyright (C) 2011-2015  P.D. Buchan (pdbuchan@yahoo.com)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Send an IPv6 ICMP router solicitation packet.
// Change hoplimit and specify interface using ancillary
// data method.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>           // close()
#include <string.h>           // strcpy, memset(), and memcpy()

#include <netinet/icmp6.h>    // struct nd_router_solicit, which contains icmp6_hdr, ND_ROUTER_SOLICIT
#include <netinet/in.h>       // IPPROTO_IPV6, IPPROTO_ICMPV6, INET6_ADDRSTRLEN
#include <netinet/ip.h>       // IP_MAXPACKET (65535)
#include <arpa/inet.h>        // inet_ntop()
#include <netdb.h>            // struct addrinfo
#include <sys/ioctl.h>        // macro ioctl is defined
#include <bits/ioctls.h>      // defines values for argument "request" of ioctl. Here, we need SIOCGIFHWADDR
#include <bits/socket.h>      // structs msghdr and cmsghdr
#include <net/if.h>           // struct ifreq

#include <errno.h>            // errno, perror()


int main (int argc, char **argv)
{
	int  sd, status;
	struct addrinfo hints;
	struct addrinfo *res;
	struct nd_router_solicit rs;
	char target[INET6_ADDRSTRLEN]={0};
	int TTL=255;

	if ((sd = socket (AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
	{
		perror ("Failed to get socket descriptor ");
		exit (EXIT_FAILURE);
	}

	// Populate icmp6_hdr portion of router solicit struct.
	rs.nd_rs_hdr.icmp6_type		= ND_ROUTER_SOLICIT;	// 133 (RFC 4861)
	rs.nd_rs_hdr.icmp6_code		= 0;					// zero for router solicitation (RFC 4861)
	rs.nd_rs_hdr.icmp6_cksum	= htons(0);				// zero when calculating checksum
	rs.nd_rs_reserved			= htonl (0);            // Reserved - must be set to zero (RFC 4861)

	strcpy (target, "ff02::2");	// to all routers

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family		= AF_INET6;
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_flags		= hints.ai_flags | AI_CANONNAME;

	if ((status = getaddrinfo (target, NULL, &hints, &res)) != 0)
	{
		fprintf (stderr, "getaddrinfo() failed: %s\n", gai_strerror (status));
		return (EXIT_FAILURE);
	}

	if (setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS , (char*)&TTL, sizeof(TTL)) < 0 )
	{
		printf("setsockopt error:%d\n", errno);
	}

	if (sendto(sd, &rs, sizeof(struct nd_router_solicit), 0, res->ai_addr, res->ai_addrlen) < 0)
	{
		printf("sendto error:%d\n", errno);
	}

	freeaddrinfo(res);

	close (sd);

	return (EXIT_SUCCESS);
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum does not preclude collisions.
uint16_t checksum (uint16_t *addr, int len)
{
  int count = len;
  register uint32_t sum = 0;
  uint16_t answer = 0;

  // Sum up 2-byte values until none or only one byte left.
  while (count > 1) {
	sum += *(addr++);
	count -= 2;
  }

  // Add left-over byte, if any.
  if (count > 0) {
	sum += *(uint8_t *) addr;
  }

  // Fold 32-bit sum into 16 bits; we lose information by doing this,
  // increasing the chances of a collision.
  // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
  while (sum >> 16) {
	sum = (sum & 0xffff) + (sum >> 16);
  }

  // Checksum is one's compliment of sum.
  answer = ~sum;

  return (answer);
}

