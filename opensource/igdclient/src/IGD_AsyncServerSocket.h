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

#ifndef ___IGD_AsyncServerSocket___
#define ___IGD_AsyncServerSocket___

/*! \file IGD_AsyncServerSocket.h 
	\brief MicroStack APIs for TCP Server Functionality
*/

/*! \defgroup IGD_AsyncServerSocket IGD_AsyncServerSocket Module
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

/*! \typedef IGD_AsyncServerSocket_ServerModule
	\brief The handle for an IGD_AsyncServerSocket module
*/
typedef void* IGD_AsyncServerSocket_ServerModule;

/*! \typedef IGD_AsyncServerSocket_ConnectionToken
	\brief Connection state, for a connected session
*/
typedef IGD_AsyncSocket_SocketModule IGD_AsyncServerSocket_ConnectionToken;

/*! \typedef IGD_AsyncServerSocket_BufferReAllocated
	\brief BufferReAllocation Handler
	\param AsyncServerSocketToken The IGD_AsyncServerSocket token
	\param ConnectionToken The IGD_AsyncServerSocket_Connection token
	\param user The User object
	\param newOffset The buffer has shifted by this offset
*/
typedef void (*IGD_AsyncServerSocket_BufferReAllocated)(IGD_AsyncServerSocket_ServerModule AsyncServerSocketToken, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, void *user, ptrdiff_t newOffset);
void IGD_AsyncServerSocket_SetReAllocateNotificationCallback(IGD_AsyncServerSocket_ServerModule AsyncServerSocketToken, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, IGD_AsyncServerSocket_BufferReAllocated Callback);

/*! \typedef IGD_AsyncServerSocket_OnInterrupt
	\brief Handler for when a session was interrupted by a call to IGD_StopChain
	\param AsyncServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user
*/
typedef void (*IGD_AsyncServerSocket_OnInterrupt)(IGD_AsyncServerSocket_ServerModule AsyncServerSocketModule, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, void *user);
/*! \typedef IGD_AsyncServerSocket_OnReceive
	\brief Handler for when data is received
	\par
	<B>Note on memory handling:</B>
	When you process the received buffer, you must advance \a p_beginPointer the number of bytes that you 
	have processed. If \a p_beginPointer does not equal \a endPointer when this method completes,
	the system will continue to reclaim any memory that has already been processed, and call this method again
	until no more memory has been processed. If no memory has been processed, and more data has been received
	on the network, the buffer will be automatically grown (according to a specific alogrythm), to accomodate any new data.
	\param AsyncServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param buffer The data that was received
	\param[in,out] p_beginPointer The start index of the data that was received
	\param endPointer The end index of the data that was received
	\param[in,out] OnInterrupt Set this pointer to receive notification if this session is interrupted
	\param[in,out] user Set a custom user object
	\param[out] PAUSE Flag to indicate if the system should continue reading data off the network
*/
typedef void (*IGD_AsyncServerSocket_OnReceive)(IGD_AsyncServerSocket_ServerModule AsyncServerSocketModule, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, char* buffer, int *p_beginPointer, int endPointer, IGD_AsyncServerSocket_OnInterrupt *OnInterrupt, void **user, int *PAUSE);
/*! \typedef IGD_AsyncServerSocket_OnConnect
	\brief Handler for when a connection is made
	\param AsyncServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param[in,out] user Set a user object to associate with this connection
*/
typedef void (*IGD_AsyncServerSocket_OnConnect)(IGD_AsyncServerSocket_ServerModule AsyncServerSocketModule, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, void **user);
/*! \typedef IGD_AsyncServerSocket_OnDisconnect
	\brief Handler for when a connection is terminated normally
	\param AsyncServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user User object that was associated with this connection
*/
typedef void (*IGD_AsyncServerSocket_OnDisconnect)(IGD_AsyncServerSocket_ServerModule AsyncServerSocketModule, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, void *user);
/*! \typedef IGD_AsyncServerSocket_OnSendOK
	\brief Handler for when pending send operations have completed
	\par
	This handler will only be called if a call to \a IGD_AsyncServerSocket_Send returned a value greater
	than 0, which indicates that not all of the data could be sent.
	\param AsyncServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param user User object that was associated with this connection
*/
typedef void (*IGD_AsyncServerSocket_OnSendOK)(IGD_AsyncServerSocket_ServerModule AsyncServerSocketModule, IGD_AsyncServerSocket_ConnectionToken ConnectionToken, void *user);

// loopbackFlag: 0 to bind to ANY, 1 to bind to IPv6 loopback first, 2 to bind to IPv4 loopback first.
IGD_AsyncServerSocket_ServerModule IGD_CreateAsyncServerSocketModule(void *Chain, int MaxConnections, unsigned short PortNumber, int initialBufferSize, int loopbackFlag, IGD_AsyncServerSocket_OnConnect OnConnect,IGD_AsyncServerSocket_OnDisconnect OnDisconnect,IGD_AsyncServerSocket_OnReceive OnReceive,IGD_AsyncServerSocket_OnInterrupt OnInterrupt,IGD_AsyncServerSocket_OnSendOK OnSendOK);

void *IGD_AsyncServerSocket_GetTag(IGD_AsyncServerSocket_ServerModule IGD_AsyncSocketModule);
void IGD_AsyncServerSocket_SetTag(IGD_AsyncServerSocket_ServerModule IGD_AsyncSocketModule, void *user);
int IGD_AsyncServerSocket_GetTag2(IGD_AsyncServerSocket_ServerModule IGD_AsyncSocketModule);
void IGD_AsyncServerSocket_SetTag2(IGD_AsyncServerSocket_ServerModule IGD_AsyncSocketModule, int user);
#ifndef MICROSTACK_NOTLS
void IGD_AsyncServerSocket_SetSSL_CTX(IGD_AsyncServerSocket_ServerModule IGD_AsyncSocketModule, void *ssl_ctx);
#endif

unsigned short IGD_AsyncServerSocket_GetPortNumber(IGD_AsyncServerSocket_ServerModule ServerSocketModule);

/*! \def IGD_AsyncServerSocket_Send
	\brief Sends data onto the TCP stream
	\param ServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
	\param buffer The data to be sent
	\param bufferLength The length of \a buffer
	\param UserFreeBuffer The \a IGD_AsyncSocket_MemoryOwnership enumeration, that identifies how the memory pointer to by \a buffer is to be handled
	\returns \a IGD_AsyncSocket_SendStatus indicating the send status
*/
#define IGD_AsyncServerSocket_Send(ServerSocketModule, ConnectionToken, buffer, bufferLength, UserFreeBuffer) IGD_AsyncSocket_Send(ConnectionToken,buffer,bufferLength,UserFreeBuffer)

/*! \def IGD_AsyncServerSocket_Disconnect
	\brief Disconnects a TCP stream
	\param ServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define IGD_AsyncServerSocket_Disconnect(ServerSocketModule, ConnectionToken) IGD_AsyncSocket_Disconnect(ConnectionToken)
/*! \def IGD_AsyncServerSocket_GetPendingBytesToSend
	\brief Gets the outstanding number of bytes to be sent
*/
#define IGD_AsyncServerSocket_GetPendingBytesToSend(ServerSocketModule, ConnectionToken) IGD_AsyncSocket_GetPendingBytesToSend(ConnectionToken)
/*! \def IGD_AsyncServerSocket_GetTotalBytesSent
	\brief Gets the total number of bytes that have been sent
	\param ServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define IGD_AsyncServerSocket_GetTotalBytesSent(ServerSocketModule, ConnectionToken) IGD_AsyncSocket_GetTotalBytesSent(ConnectionToken)
/*! \def IGD_AsyncServerSocket_ResetTotalBytesSent
	\brief Resets the total bytes sent counter
	\param ServerSocketModule The parent IGD_AsyncServerSocket_ServerModule
	\param ConnectionToken The connection state for this session
*/
#define IGD_AsyncServerSocket_ResetTotalBytesSent(ServerSocketModule, ConnectionToken) IGD_AsyncSocket_ResetTotalBytesSent(ConnectionToken)

#ifdef __cplusplus
}
#endif

#endif

