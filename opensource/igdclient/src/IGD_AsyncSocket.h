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

#ifndef ___IGD_AsyncSocket___
#define ___IGD_AsyncSocket___

/*! \file IGD_AsyncSocket.h 
\brief MicroStack APIs for TCP Client Functionality
*/

/*! \defgroup IGD_AsyncSocket IGD_AsyncSocket Module
\{
*/

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.H>
#elif defined(_POSIX)
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#endif

//#include "ssl.h"
#ifndef MICROSTACK_NOTLS
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32_WCE)
#ifndef ptrdiff_t
#define ptrdiff_t long
#endif
#endif

/*! \def MEMORYCHUNKSIZE
\brief Incrementally grow the buffer by this amount of bytes
*/
#define MEMORYCHUNKSIZE 4096

enum IGD_AsyncSocket_SendStatus
{
	IGD_AsyncSocket_ALL_DATA_SENT = 0, /*!< All of the data has already been sent */
	IGD_AsyncSocket_NOT_ALL_DATA_SENT_YET = 1, /*!< Not all of the data could be sent, but is queued to be sent as soon as possible */
	IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR	= -4 /*!< A send operation was attmepted on a closed socket */
};

/*! \enum IGD_AsyncSocket_MemoryOwnership
\brief Enumeration values for Memory Ownership of variables
*/
enum IGD_AsyncSocket_MemoryOwnership
{
	IGD_AsyncSocket_MemoryOwnership_CHAIN = 0, /*!< The Microstack will own this memory, and free it when it is done with it */
	IGD_AsyncSocket_MemoryOwnership_STATIC = 1, /*!< This memory is static, so the Microstack will not free it, and assume it will not go away, so it won't copy it either */
	IGD_AsyncSocket_MemoryOwnership_USER = 2 /*!< The Microstack doesn't own this memory, so if necessary the memory will be copied */
};

/*! \typedef IGD_AsyncSocket_SocketModule
\brief The handle for an IGD_AsyncSocket module
*/
typedef void* IGD_AsyncSocket_SocketModule;
/*! \typedef IGD_AsyncSocket_OnInterrupt
\brief Handler for when a session was interrupted by a call to IGD_StopChain
\param socketModule The \a IGD_AsyncSocket_SocketModule that was interrupted
\param user The user object that was associated with this connection
*/

//typedef void(*IGD_AsyncSocket_OnReplaceSocket)(IGD_AsyncSocket_SocketModule socketModule, void *user);
typedef void(*IGD_AsyncSocket_OnBufferSizeExceeded)(IGD_AsyncSocket_SocketModule socketModule, void *user);

typedef void(*IGD_AsyncSocket_OnInterrupt)(IGD_AsyncSocket_SocketModule socketModule, void *user);
/*! \typedef IGD_AsyncSocket_OnData
\brief Handler for when data is received
\par
<B>Note on memory handling:</B>
When you process the received buffer, you must advance \a p_beginPointer the number of bytes that you 
have processed. If \a p_beginPointer does not equal \a endPointer when this method completes,
the system will continue to reclaim any memory that has already been processed, and call this method again
until no more memory has been processed. If no memory has been processed, and more data has been received
on the network, the buffer will be automatically grown (according to a specific alogrythm), to accomodate any new data.
\param socketModule The \a IGD_AsyncSocket_SocketModule that received data
\param buffer The data that was received
\param[in,out] p_beginPointer The start index of the data that was received
\param endPointer The end index of the data that was received
\param[in,out] OnInterrupt Set this pointer to receive notification if this session is interrupted
\param[in,out] user Set a custom user object
\param[out] PAUSE Flag to indicate if the system should continue reading data off the network
*/
typedef void(*IGD_AsyncSocket_OnData)(IGD_AsyncSocket_SocketModule socketModule, char* buffer, int *p_beginPointer, int endPointer,IGD_AsyncSocket_OnInterrupt* OnInterrupt, void **user, int *PAUSE);
/*! \typedef IGD_AsyncSocket_OnConnect
\brief Handler for when a connection is made
\param socketModule The \a IGD_AsyncSocket_SocketModule that was connected
\param user The user object that was associated with this object
*/
typedef void(*IGD_AsyncSocket_OnConnect)(IGD_AsyncSocket_SocketModule socketModule, int Connected, void *user);
/*! \typedef IGD_AsyncSocket_OnDisconnect
\brief Handler for when a connection is terminated normally
\param socketModule The \a IGD_AsyncSocket_SocketModule that was disconnected
\param user User object that was associated with this connection
*/
typedef void(*IGD_AsyncSocket_OnDisconnect)(IGD_AsyncSocket_SocketModule socketModule, void *user);
/*! \typedef IGD_AsyncSocket_OnSendOK
\brief Handler for when pending send operations have completed
\par
This handler will only be called if a call to \a IGD_AsyncSocket_Send returned a value greater
than 0, which indicates that not all of the data could be sent.
\param socketModule The \a IGD_AsyncSocket_SocketModule whos pending sends have completed
\param user User object that was associated with this connection
*/
typedef void(*IGD_AsyncSocket_OnSendOK)(IGD_AsyncSocket_SocketModule socketModule, void *user);
/*! \typedef IGD_AsyncSocket_OnBufferReAllocated
\brief Handler for when the internal data buffer has been resized.
\par
<B>Note:</B> This is only useful if you are storing pointer values into the buffer supplied in \a IGD_AsyncSocket_OnData. 
\param AsyncSocketToken The \a IGD_AsyncSocket_SocketModule whos buffer was resized
\param user The user object that was associated with this connection
\param newOffset The new offset differential. Simply add this value to your existing pointers, to obtain the correct pointer into the resized buffer.
*/
typedef void(*IGD_AsyncSocket_OnBufferReAllocated)(IGD_AsyncSocket_SocketModule AsyncSocketToken, void *user, ptrdiff_t newOffset);


void IGD_AsyncSocket_SetReAllocateNotificationCallback(IGD_AsyncSocket_SocketModule AsyncSocketToken, IGD_AsyncSocket_OnBufferReAllocated Callback);
void *IGD_AsyncSocket_GetUser(IGD_AsyncSocket_SocketModule socketModule);
void IGD_AsyncSocket_SetUser(IGD_AsyncSocket_SocketModule socketModule, void* user);
void *IGD_AsyncSocket_GetUser2(IGD_AsyncSocket_SocketModule socketModule);
void IGD_AsyncSocket_SetUser2(IGD_AsyncSocket_SocketModule socketModule, void* user);
int IGD_AsyncSocket_GetUser3(IGD_AsyncSocket_SocketModule socketModule);
void IGD_AsyncSocket_SetUser3(IGD_AsyncSocket_SocketModule socketModule, int user);

IGD_AsyncSocket_SocketModule IGD_CreateAsyncSocketModule(void *Chain, int initialBufferSize, IGD_AsyncSocket_OnData, IGD_AsyncSocket_OnConnect OnConnect, IGD_AsyncSocket_OnDisconnect OnDisconnect, IGD_AsyncSocket_OnSendOK OnSendOK);

void *IGD_AsyncSocket_GetSocket(IGD_AsyncSocket_SocketModule module);

unsigned int IGD_AsyncSocket_GetPendingBytesToSend(IGD_AsyncSocket_SocketModule socketModule);
unsigned int IGD_AsyncSocket_GetTotalBytesSent(IGD_AsyncSocket_SocketModule socketModule);
void IGD_AsyncSocket_ResetTotalBytesSent(IGD_AsyncSocket_SocketModule socketModule);

void IGD_AsyncSocket_ConnectTo(void* socketModule, struct sockaddr *localInterface, struct sockaddr *remoteAddress, IGD_AsyncSocket_OnInterrupt InterruptPtr, void *user);

#ifdef MICROSTACK_PROXY
void IGD_AsyncSocket_ConnectToProxy(void* socketModule, struct sockaddr *localInterface, struct sockaddr *remoteAddress, struct sockaddr *proxyAddress, char* proxyUser, char* proxyPass, IGD_AsyncSocket_OnInterrupt InterruptPtr, void *user);
#endif

enum IGD_AsyncSocket_SendStatus IGD_AsyncSocket_SendTo(IGD_AsyncSocket_SocketModule socketModule, char* buffer, int length, struct sockaddr *remoteAddress, enum IGD_AsyncSocket_MemoryOwnership UserFree);

/*! \def IGD_AsyncSocket_Send
\brief Sends data onto the TCP stream
\param socketModule The \a IGD_AsyncSocket_SocketModule to send data on
\param buffer The data to be sent
\param length The length of \a buffer
\param UserFree The \a IGD_AsyncSocket_MemoryOwnership enumeration, that identifies how the memory pointer to by \a buffer is to be handled
\returns \a IGD_AsyncSocket_SendStatus indicating the send status
*/
#define IGD_AsyncSocket_Send(socketModule, buffer, length, UserFree) IGD_AsyncSocket_SendTo(socketModule, buffer, length, NULL, UserFree)
void IGD_AsyncSocket_Disconnect(IGD_AsyncSocket_SocketModule socketModule);
void IGD_AsyncSocket_GetBuffer(IGD_AsyncSocket_SocketModule socketModule, char **buffer, int *BeginPointer, int *EndPointer);

void IGD_AsyncSocket_UseThisSocket(IGD_AsyncSocket_SocketModule socketModule, void* TheSocket, IGD_AsyncSocket_OnInterrupt InterruptPtr, void *user);

#ifndef MICROSTACK_NOTLS
void IGD_AsyncSocket_SetSSLContext(IGD_AsyncSocket_SocketModule socketModule, SSL_CTX *ssl_ctx, int server);
SSL_CTX *IGD_AsyncSocket_GetSSLContext(IGD_AsyncSocket_SocketModule socketModule);
#endif

void IGD_AsyncSocket_SetRemoteAddress(IGD_AsyncSocket_SocketModule socketModule, struct sockaddr *remoteAddress);
void IGD_AsyncSocket_SetLocalInterface(IGD_AsyncSocket_SocketModule module, struct sockaddr *localAddress);

int IGD_AsyncSocket_IsFree(IGD_AsyncSocket_SocketModule socketModule);
int IGD_AsyncSocket_IsConnected(IGD_AsyncSocket_SocketModule socketModule);
int IGD_AsyncSocket_GetLocalInterface(IGD_AsyncSocket_SocketModule socketModule, struct sockaddr *localAddress);
int IGD_AsyncSocket_GetRemoteInterface(IGD_AsyncSocket_SocketModule socketModule, struct sockaddr *remoteAddress);
unsigned short IGD_AsyncSocket_GetLocalPort(IGD_AsyncSocket_SocketModule socketModule);

void IGD_AsyncSocket_Resume(IGD_AsyncSocket_SocketModule socketModule);
int IGD_AsyncSocket_WasClosedBecauseBufferSizeExceeded(IGD_AsyncSocket_SocketModule socketModule);
void IGD_AsyncSocket_SetMaximumBufferSize(IGD_AsyncSocket_SocketModule module, int maxSize, IGD_AsyncSocket_OnBufferSizeExceeded OnBufferSizeExceededCallback, void *user);
void IGD_AsyncSocket_SetSendOK(IGD_AsyncSocket_SocketModule module, IGD_AsyncSocket_OnSendOK OnSendOK);
int IGD_AsyncSocket_IsIPv6LinkLocal(struct sockaddr *LocalAddress);
int IGD_AsyncSocket_IsModuleIPv6LinkLocal(IGD_AsyncSocket_SocketModule module);

#ifndef MICROSTACK_NOTLS
X509 *IGD_AsyncSocket_SslGetCert(IGD_AsyncSocket_SocketModule socketModule);
STACK_OF(X509) *IGD_AsyncSocket_SslGetCerts(IGD_AsyncSocket_SocketModule socketModule);
#endif 

#ifdef __cplusplus
}
#endif

#endif

