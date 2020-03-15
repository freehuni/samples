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

#ifndef ___IGD_AsyncUDPSocket___
#define ___IGD_AsyncUDPSocket___

/*! \file IGD_AsyncUDPSocket.h 
	\brief MicroStack APIs for UDP Functionality
*/

/*! \defgroup IGD_AsyncUDPSocket IGD_AsyncUDPSocket Module
	\{
*/

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.H>
#elif defined(_POSIX)
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#endif

#include "IGD_AsyncSocket.h"

#ifdef __cplusplus
extern "C" {
#endif

enum IGD_AsyncUDPSocket_Reuse
{
	IGD_AsyncUDPSocket_Reuse_EXCLUSIVE=0,	/*!< A socket is to be bound for exclusive access */
	IGD_AsyncUDPSocket_Reuse_SHARED=1		/*!< A socket is to be bound for shared access */
};

/*! \typedef IGD_AsyncUDPSocket_SocketModule
	\brief The handle for an IGD_AsyncUDPSocket module
*/
typedef void* IGD_AsyncUDPSocket_SocketModule;
/*! \typedef IGD_AsyncUDPSocket_OnData
	\brief The handler that is called when data is received
	\param socketModule The \a IGD_AsyncUDPSocket_SocketModule handle that received data
	\param buffer The buffer that contains the read data
	\param bufferLength The amount of data that was read
	\param remoteInterface The IP address of the source, in network order
	\param remotePort The port number of the source, in host order
	\param user	User object associated with this module
	\param user2 User2 object associated with this module
	\param[out] PAUSE Set this flag to non-zero, to prevent more data from being read
*/
typedef void(*IGD_AsyncUDPSocket_OnData)(IGD_AsyncUDPSocket_SocketModule socketModule, char* buffer, int bufferLength, struct sockaddr_in6 *remoteInterface, void *user, void *user2, int *PAUSE);
/*! \typedef IGD_AsyncUDPSocket_OnSendOK
	\brief Handler for when pending send operations have completed
	\par
	This handler will only be called if a call to \a IGD_AsyncUDPSocket_SendTo returned a value greater
	than 0, which indicates that not all of the data could be sent.
	<P><B>Note:</B> On most systems, UDP data that cannot be sent will be dropped, which means that this handler
	may actually never be called.
	\param socketModule The \a IGD_AsyncUDPSocket_SocketModule whos pending sends have completed
	\param user1 User object that was associated with this connection
	\param user2 User2 object that was associated with this connection
*/
typedef void(*IGD_AsyncUDPSocket_OnSendOK)(IGD_AsyncUDPSocket_SocketModule socketModule, void *user1, void *user2);

IGD_AsyncUDPSocket_SocketModule IGD_AsyncUDPSocket_CreateEx(void *Chain, int BufferSize, struct sockaddr *localInterface, enum IGD_AsyncUDPSocket_Reuse reuse, IGD_AsyncUDPSocket_OnData OnData, IGD_AsyncUDPSocket_OnSendOK OnSendOK, void *user);

/*! \def IGD_AsyncUDPSocket_Create
	\brief Creates a new instance of an IGD_AsyncUDPSocket module.
	\param Chain The chain to add this object to. (Chain must <B>not</B> not be running)
	\param BufferSize The size of the buffer to use
	\param localInterface The IP address to bind this socket to, in network order
	\param localPort The port to bind this socket to, in host order. (0 = Random IANA specified generic port)
	\param reuse Reuse type
	\param OnData The handler to receive data
	\param OnSendOK The handler to receive notification that pending sends have completed
	\param user User object to associate with this object
	\returns The IGD_AsyncUDPSocket_SocketModule handle that was created
*/
//#define IGD_AsyncUDPSocket_Create(Chain, BufferSize, localInterface, localInterfaceSize, reuse, OnData , OnSendOK, user)localPort==0?IGD_AsyncUDPSocket_CreateEx(Chain, BufferSize, localInterface, 50000, 65500, reuse, OnData, OnSendOK, user):IGD_AsyncUDPSocket_CreateEx(Chain, BufferSize, localInterface, localPort, localPort, reuse, OnData, OnSendOK, user)

void IGD_AsyncUDPSocket_JoinMulticastGroupV4(IGD_AsyncUDPSocket_SocketModule module, struct sockaddr_in *multicastAddr, struct sockaddr *localAddr);
void IGD_AsyncUDPSocket_JoinMulticastGroupV6(IGD_AsyncUDPSocket_SocketModule module, struct sockaddr_in6 *multicastAddr, int ifIndex);
void IGD_AsyncUDPSocket_SetMulticastInterface(IGD_AsyncUDPSocket_SocketModule module, struct sockaddr *localInterface);
void IGD_AsyncUDPSocket_SetMulticastTTL(IGD_AsyncUDPSocket_SocketModule module, int TTL);
void IGD_AsyncUDPSocket_SetMulticastLoopback(IGD_AsyncUDPSocket_SocketModule module, int loopback);

/*! \def IGD_AsyncUDPSocket_GetPendingBytesToSend
	\brief Returns the number of bytes that are pending to be sent
	\param socketModule The IGD_AsyncUDPSocket_SocketModule handle to query
	\returns Number of pending bytes
*/
#define IGD_AsyncUDPSocket_GetPendingBytesToSend(socketModule) IGD_AsyncSocket_GetPendingBytesToSend(socketModule)
/*! \def IGD_AsyncUDPSocket_GetTotalBytesSent
	\brief Returns the total number of bytes that have been sent, since the last reset
	\param socketModule The IGD_AsyncUDPSocket_SocketModule handle to query
	\returns Number of bytes sent
*/
#define IGD_AsyncUDPSocket_GetTotalBytesSent(socketModule) IGD_AsyncSocket_GetTotalBytesSent(socketModule)
/*! \def IGD_AsyncUDPSocket_ResetTotalBytesSent
	\brief Resets the total bytes sent counter
	\param socketModule The IGD_AsyncUDPSocket_SocketModule handle to reset
*/
#define IGD_AsyncUDPSocket_ResetTotalBytesSent(socketModule) IGD_AsyncSocket_ResetTotalBytesSent(socketModule)


/*! \def IGD_AsyncUDPSocket_SendTo
	\brief Sends a UDP packet
	\param socketModule The IGD_AsyncUDPSocket_SocketModule handle to send a packet on
	\param remoteInterface The IP address in network order, to send the packet to
	\param remotePort The port numer in host order to send the packet to
	\param buffer The buffer to send
	\param length The length of \a buffer
	\param UserFree The IGD_AsyncSocket_MemoryOwnership flag indicating how the memory in \a buffer is to be handled
	\returns The IGD_AsyncSocket_SendStatus status of the packet that was sent
*/
#define IGD_AsyncUDPSocket_SendTo(socketModule, remoteInterface, buffer, length, UserFree) IGD_AsyncSocket_SendTo(socketModule, buffer, length, remoteInterface, UserFree)

/*! \def IGD_AsyncUDPSocket_GetLocalInterface
	\brief Get's the bounded IP address in network order
	\param socketModule The IGD_AsyncUDPSocket_SocketModule to query
	\returns The local bounded IP address in network order
*/
#define IGD_AsyncUDPSocket_GetLocalInterface(socketModule, localAddress) IGD_AsyncSocket_GetLocalInterface(socketModule, localAddress)
#define IGD_AsyncUDPSocket_SetLocalInterface(socketModule, localAddress) IGD_AsyncSocket_SetLocalInterface(socketModule, localAddress)

/*! \def IGD_AsyncUDPSocket_GetLocalPort
	\brief Get's the bounded port in host order
	\param socketModule The IGD_AsyncUDPSocket_SocketModule to query
	\returns The local bounded port in host order
*/
#define IGD_AsyncUDPSocket_GetLocalPort(socketModule) IGD_AsyncSocket_GetLocalPort(socketModule)

#define IGD_AsyncUDPSocket_Resume(socketModule) IGD_AsyncSocket_Resume(socketModule)  

SOCKET IGD_AsyncUDPSocket_GetSocket(IGD_AsyncUDPSocket_SocketModule module);

#ifdef __cplusplus
}
#endif

#endif

