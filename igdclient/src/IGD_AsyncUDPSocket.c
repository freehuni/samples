/*   
Copyright 2006 - 2011 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifdef MEMORY_CHECK
	#include <assert.h>
	#define MEMCHECK(x) x
#else
	#define MEMCHECK(x)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#if defined(WINSOCK2)
	#include <winsock2.h>
	#include <ws2tcpip.h>
#elif defined(WINSOCK1)
	#include <winsock.h>
	#include <wininet.h>
#endif

#include "IGD_Parsers.h"
#include "IGD_AsyncUDPSocket.h"
#include "IGD_AsyncSocket.h"

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))
#define INET_SOCKADDR_PORT(x) (x->sa_family==AF_INET6?(unsigned short)(((struct sockaddr_in6*)x)->sin6_port):(unsigned short)(((struct sockaddr_in*)x)->sin_port))
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

struct IGD_AsyncUDPSocket_Data
{
	void *user1;
	void *user2;

	IGD_AsyncSocket_SocketModule UDPSocket;
	unsigned short BoundPortNumber;

	IGD_AsyncUDPSocket_OnData OnData;
	IGD_AsyncUDPSocket_OnSendOK OnSendOK;
};

void IGD_AsyncUDPSocket_OnDataSink(IGD_AsyncSocket_SocketModule socketModule, char* buffer, int *p_beginPointer, int endPointer,IGD_AsyncSocket_OnInterrupt* OnInterrupt, void **user, int *PAUSE)
{
	struct IGD_AsyncUDPSocket_Data *data = (struct IGD_AsyncUDPSocket_Data*)*user;

	struct sockaddr_in6 RemoteAddress;
	int RemoteAddressSize;

	UNREFERENCED_PARAMETER( OnInterrupt );

	RemoteAddressSize = IGD_AsyncSocket_GetRemoteInterface(socketModule, (struct sockaddr*)&RemoteAddress);

	if (data->OnData!=NULL)
	{
		data->OnData(
			socketModule,
			buffer, 
			endPointer,
			&RemoteAddress,
			data->user1, 
			data->user2, 
			PAUSE);
	}
	*p_beginPointer = endPointer;
}
void IGD_AsyncUDPSocket_OnSendOKSink(IGD_AsyncSocket_SocketModule socketModule, void *user)
{
	struct IGD_AsyncUDPSocket_Data *data = (struct IGD_AsyncUDPSocket_Data*)user;
	if (data->OnSendOK!=NULL)
	{
		data->OnSendOK(socketModule, data->user1, data->user2);
	}
}

void IGD_AsyncUDPSocket_OnDisconnect(IGD_AsyncSocket_SocketModule socketModule, void *user)
{
	UNREFERENCED_PARAMETER( socketModule );
	free(user);
}
/*! \fn IGD_AsyncUDPSocket_SocketModule IGD_AsyncUDPSocket_CreateEx(void *Chain, int BufferSize, int localInterface, unsigned short localPortStartRange, unsigned short localPortEndRange, enum IGD_AsyncUDPSocket_Reuse reuse, IGD_AsyncUDPSocket_OnData OnData, IGD_AsyncUDPSocket_OnSendOK OnSendOK, void *user)
	\brief Creates a new instance of an IGD_AsyncUDPSocket module, using a random port number between \a localPortStartRange and \a localPortEndRange inclusive.
	\param Chain The chain to add this object to. (Chain must <B>not</B> not be running)
	\param BufferSize The size of the buffer to use
	\param localInterface The IP address to bind this socket to, in network order
	\param localPortStartRange The begin range to select a port number from (host order)
	\param localPortEndRange The end range to select a port number from (host order)
	\param reuse Reuse type
	\param OnData The handler to receive data
	\param OnSendOK The handler to receive notification that pending sends have completed
	\param user User object to associate with this object
	\returns The IGD_AsyncUDPSocket_SocketModule handle that was created
*/
IGD_AsyncUDPSocket_SocketModule IGD_AsyncUDPSocket_CreateEx(void *Chain, int BufferSize, struct sockaddr *localInterface, enum IGD_AsyncUDPSocket_Reuse reuse, IGD_AsyncUDPSocket_OnData OnData, IGD_AsyncUDPSocket_OnSendOK OnSendOK, void *user)
{
	int off = 0;
	SOCKET sock;
	int ra = (int)reuse;
	void *RetVal = NULL;
	struct IGD_AsyncUDPSocket_Data *data;
	#ifdef WINSOCK2
	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	#endif

	// Initialize the UDP socket data structure
	data = (struct IGD_AsyncUDPSocket_Data*)malloc(sizeof(struct IGD_AsyncUDPSocket_Data));
	if (data == NULL) return NULL;
	memset(data, 0, sizeof(struct IGD_AsyncUDPSocket_Data));
	data->OnData = OnData;
	data->OnSendOK = OnSendOK;
	data->user1 = user;

	// Create a new socket & set REUSE if needed. If it's IPv6, use the same socket for both IPv4 and IPv6.
	if ((sock = socket(localInterface->sa_family, SOCK_DGRAM, IPPROTO_UDP)) == -1) { free(data); return 0; }
	if (reuse == IGD_AsyncUDPSocket_Reuse_SHARED) if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&ra, sizeof(ra)) != 0) ILIBCRITICALERREXIT(253);
#ifdef __APPLE__
	if (reuse == IGD_AsyncUDPSocket_Reuse_SHARED) if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (char*)&ra, sizeof(ra)) != 0) ILIBCRITICALERREXIT(253);
#endif
	if (localInterface->sa_family == AF_INET6) if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off)) != 0) ILIBCRITICALERREXIT(253);

	// Attempt to bind the UDP socket
	#ifdef WIN32
	if ( bind(sock, localInterface, INET_SOCKADDR_LENGTH(localInterface->sa_family)) != 0 ) { closesocket(sock); free(data); return NULL; }
	#else
	if ( bind(sock, localInterface, INET_SOCKADDR_LENGTH(localInterface->sa_family)) != 0 ) { close(sock); free(data); return NULL; }
	#endif

	#ifdef WINSOCK2
	WSAIoctl(sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
	#endif

	// Set the BoundPortNumber
	if (localInterface->sa_family == AF_INET6) { data->BoundPortNumber = ntohs(((struct sockaddr_in6*)localInterface)->sin6_port); } else { data->BoundPortNumber = ntohs(((struct sockaddr_in*)localInterface)->sin_port); }

	// Create an Async Socket to handle the data
	RetVal = IGD_CreateAsyncSocketModule(Chain, BufferSize, &IGD_AsyncUDPSocket_OnDataSink, NULL, &IGD_AsyncUDPSocket_OnDisconnect, &IGD_AsyncUDPSocket_OnSendOKSink);
	if (RetVal == NULL)
	{
		#if defined(WIN32) || defined(_WIN32_WCE)
		closesocket(sock);
		#else
		close(sock);
		#endif
		free(data);
		return NULL;
	}
	IGD_AsyncSocket_UseThisSocket(RetVal, &sock, &IGD_AsyncUDPSocket_OnDisconnect, data);
	return RetVal;
}

SOCKET IGD_AsyncUDPSocket_GetSocket(IGD_AsyncUDPSocket_SocketModule module)
{
	return *((SOCKET*)IGD_AsyncSocket_GetSocket(module));
}

/*! \fn int IGD_AsyncUDPSocket_JoinMulticastGroup(IGD_AsyncUDPSocket_SocketModule module, int localInterface, int remoteInterface)
	\brief Joins a multicast group
	\param module The IGD_AsyncUDPSocket_SocketModule to join the multicast group
	\param localInterface The local IP address in network order, to join the multicast group
	\param remoteInterface The multicast ip address in network order, to join
	\returns 0 = Success, Nonzero = Failure
*/
void IGD_AsyncUDPSocket_JoinMulticastGroupV4(IGD_AsyncUDPSocket_SocketModule module, struct sockaddr_in *multicastAddr, struct sockaddr *localAddr)
{
	struct ip_mreq mreq;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)IGD_AsyncSocket_GetSocket(module));
#else
	int s = *((int*)IGD_AsyncSocket_GetSocket(module));
#endif

	// We start with the multicast structure
	memcpy(&mreq.imr_multiaddr, &(((struct sockaddr_in*)multicastAddr)->sin_addr), sizeof(mreq.imr_multiaddr));
#ifdef WIN32
	mreq.imr_interface.s_addr = ((struct sockaddr_in*)localAddr)->sin_addr.S_un.S_addr;
#else
	mreq.imr_interface.s_addr = ((struct sockaddr_in*)localAddr)->sin_addr.s_addr;
#endif
    setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));
}

void IGD_AsyncUDPSocket_JoinMulticastGroupV6(IGD_AsyncUDPSocket_SocketModule module, struct sockaddr_in6 *multicastAddr, int ifIndex)
{
	struct ipv6_mreq mreq6;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)IGD_AsyncSocket_GetSocket(module));
#else
	int s = *((int*)IGD_AsyncSocket_GetSocket(module));
#endif

	memcpy(&mreq6.ipv6mr_multiaddr, &(((struct sockaddr_in6*)multicastAddr)->sin6_addr), sizeof(mreq6.ipv6mr_multiaddr));
    mreq6.ipv6mr_interface = ifIndex;
    setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*)&mreq6, sizeof(mreq6));		// IPV6_ADD_MEMBERSHIP
}

/*! \fn int IGD_AsyncUDPSocket_SetMulticastInterface(IGD_AsyncUDPSocket_SocketModule module, int localInterface)
	\brief Sets the local interface to use, when multicasting
	\param module The IGD_AsyncUDPSocket_SocketModule handle to set the interface on
	\param localInterface The local IP address in network order, to use when multicasting
	\returns 0 = Success, Nonzero = Failure
*/
void IGD_AsyncUDPSocket_SetMulticastInterface(IGD_AsyncUDPSocket_SocketModule module, struct sockaddr *localInterface)
{
#if defined(WIN32)
	SOCKET s = *((SOCKET*)IGD_AsyncSocket_GetSocket(module));
#else
	int s = *((int*)IGD_AsyncSocket_GetSocket(module));
#endif
	if (localInterface->sa_family == AF_INET6)
	{
		// Uses the interface index of the desired outgoing interface
		if (setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_IF, (char*)&(((struct sockaddr_in6*)localInterface)->sin6_scope_id), sizeof(((struct sockaddr_in6*) localInterface)->sin6_scope_id)) != 0) ILIBCRITICALERREXIT(253);
	}
	else
	{
		if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char*)&(((struct sockaddr_in*)localInterface)->sin_addr.s_addr), sizeof(struct in_addr)) != 0) ILIBCRITICALERREXIT(253);
	}
}

/*! \fn int IGD_AsyncUDPSocket_SetMulticastTTL(IGD_AsyncUDPSocket_SocketModule module, unsigned char TTL)
	\brief Sets the Multicast TTL value
	\param module The IGD_AsyncUDPSocket_SocketModule handle to set the Multicast TTL value
	\param TTL The Multicast-TTL value to use
	\returns 0 = Success, Nonzero = Failure
*/
void IGD_AsyncUDPSocket_SetMulticastTTL(IGD_AsyncUDPSocket_SocketModule module, int TTL)
{
	struct sockaddr_in6 localAddress;
#if defined(__SYMBIAN32__)
	return(0);
#else
	#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)IGD_AsyncSocket_GetSocket(module));
#else
	int s = *((int*)IGD_AsyncSocket_GetSocket(module));
#endif
	IGD_AsyncSocket_GetLocalInterface(module, (struct sockaddr*)&localAddress);
	if (setsockopt(s, localAddress.sin6_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP, localAddress.sin6_family == PF_INET6 ? IPV6_MULTICAST_HOPS : IP_MULTICAST_TTL, (char*)&TTL, sizeof(TTL)) != 0) ILIBCRITICALERREXIT(253);
#endif
}

void IGD_AsyncUDPSocket_SetMulticastLoopback(IGD_AsyncUDPSocket_SocketModule module, int loopback)
{
	struct sockaddr_in6 localAddress;
#if defined(WIN32) || defined(_WIN32_WCE)
	SOCKET s = *((SOCKET*)IGD_AsyncSocket_GetSocket(module));
#else
	int s = *((int*)IGD_AsyncSocket_GetSocket(module));
#endif

	IGD_AsyncSocket_GetLocalInterface(module, (struct sockaddr*)&localAddress);
	if (setsockopt(s, localAddress.sin6_family == PF_INET6 ? IPPROTO_IPV6 : IPPROTO_IP, localAddress.sin6_family == PF_INET6 ? IPV6_MULTICAST_LOOP : IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback)) != 0) ILIBCRITICALERREXIT(253);
}

