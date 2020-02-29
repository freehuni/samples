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

#ifndef __IGD_WebClient__
#define __IGD_WebClient__

#include "IGD_Parsers.h"
#include "IGD_AsyncSocket.h"

#ifndef MICROSTACK_NOTLS
#include <openssl/x509v3.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \file IGD_WebClient.h 
	\brief MicroStack APIs for HTTP Client functionality
*/

#define IGD_WebClientIsStatusOk(statusCode) (statusCode>=200 && statusCode<=299)

/*! \defgroup IGD_WebClient IGD_WebClient Module
	\{
*/

/*! \def WEBCLIENT_DESTROYED
	\brief The IGD_WebClient object was destroyed
*/
#define WEBCLIENT_DESTROYED 5
/*! \def WEBCLIENT_DELETED
	\brief The IGD_WebClient object was deleted with a call to IGD_WebClient_DeleteRequests
*/
#define WEBCLIENT_DELETED 6

#if defined(WIN32) || defined(_WIN32_WCE)
#include <STDDEF.h>
#elif defined(_POSIX)
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#endif

enum IGD_WebClient_DoneCode
{
	IGD_WebClient_DoneCode_NoErrors = 1,
	IGD_WebClient_DoneCode_StillReading = 0,
	IGD_WebClient_DoneCode_HeaderTooBig = -2,
	IGD_WebClient_DoneCode_BodyTooBig = -3,
};

enum IGD_WebClient_Range_Result
{
	IGD_WebClient_Range_Result_OK = 0,
	IGD_WebClient_Range_Result_INVALID_RANGE = 1,
	IGD_WebClient_Range_Result_BAD_REQUEST = 2,
};
/*! \typedef IGD_WebClient_RequestToken
	\brief The handle for a request, obtained from a call to \a IGD_WebClient_PipelineRequest
*/
typedef void* IGD_WebClient_RequestToken;

/*! \typedef IGD_WebClient_RequestManager
	\brief An object that manages HTTP Client requests. Obtained from \a IGD_CreateWebClient
*/
typedef void* IGD_WebClient_RequestManager;

/*! \typedef IGD_WebClient_StateObject
	\brief Handle for an HTTP Client Connection.
*/
typedef void* IGD_WebClient_StateObject;

/*! \typedef IGD_WebClient_OnResponse
	\brief Function Callback Pointer, dispatched to process received data
	\param WebStateObject The IGD_WebClient that received a response
	\param InterruptFlag A flag indicating if this request was interrupted with a call to IGD_StopChain
	\param header The packetheader object containing the HTTP headers.
	\param bodyBuffer The buffer containing the body
	\param[in,out] beginPointer The start index of the buffer referenced by \a bodyBuffer
	\param endPointer The end index of the buffer referenced by \a bodyBuffer
	\param done Flag indicating if all of the response has been received. (0 = More data to be received, NonZero = Done)
	\param user1 
	\param user2
	\param[out] PAUSE Set to nonzero value if you do not want the system to read any more data from the network
*/
typedef void(*IGD_WebClient_OnResponse)(IGD_WebClient_StateObject WebStateObject,int InterruptFlag,struct packetheader *header,char *bodyBuffer,int *beginPointer,int endPointer,int done,void *user1,void *user2,int *PAUSE);
/*! \typedef IGD_WebClient_OnSendOK
	\brief Handler for when pending send operations have completed
	\par
	<B>Note:</B> This handler will only be called after all data from any call(s) to \a IGD_WebClient_StreamRequestBody
	have completed.
	\param sender The \a IGD_WebClient_StateObject whos pending sends have completed
	\param user1 User1 object that was associated with this connection
	\param user2 User2 object that was associated with this connection
*/
typedef void(*IGD_WebClient_OnSendOK)(IGD_WebClient_StateObject sender, void *user1, void *user2);
/*! \typedef IGD_WebClient_OnDisconnect
	\brief Handler for when the session has disconnected
	\param sender The \a IGD_WebClient_StateObject that has been disconnected
	\param request The IGD_WebClient_RequestToken that represents the request that was in progress when the disconnect happened.
*/
typedef void(*IGD_WebClient_OnDisconnect)(IGD_WebClient_StateObject sender, IGD_WebClient_RequestToken request);

#ifndef MICROSTACK_NOTLS
typedef int(*IGD_WebClient_OnSslConnection)(IGD_WebClient_StateObject sender, STACK_OF(X509) *certs, struct sockaddr_in6 *address, void *user);
#endif

//
// This is the number of seconds that a connection must be idle for, before it will
// be automatically closed. Idle means there are no pending requests
//
/*! \def HTTP_SESSION_IDLE_TIMEOUT
	\brief This is the number of seconds that a connection must be idle for, before it will be automatically closed.
	\par
	Idle means there are no pending requests
*/
#define HTTP_SESSION_IDLE_TIMEOUT 10

/*! \def HTTP_CONNECT_RETRY_COUNT
	\brief This is the number of times, an HTTP connection will be attempted, before it fails.
	\par
	This module utilizes an exponential backoff algorithm. That is, it will retry immediately, then it will retry after 1 second, then 2, then 4, etc.
*/
#define HTTP_CONNECT_RETRY_COUNT 2

/*! \def INITIAL_BUFFER_SIZE
	\brief This initial size of the receive buffer
*/
#define INITIAL_BUFFER_SIZE 65535


IGD_WebClient_RequestManager IGD_CreateWebClient(int PoolSize, void *Chain);
IGD_WebClient_StateObject IGD_CreateWebClientEx(IGD_WebClient_OnResponse OnResponse, IGD_AsyncSocket_SocketModule socketModule, void *user1, void *user2);

void IGD_WebClient_OnBufferReAllocate(IGD_AsyncSocket_SocketModule token, void *user, ptrdiff_t offSet);
void IGD_WebClient_OnData(IGD_AsyncSocket_SocketModule socketModule,char* buffer,int *p_beginPointer, int endPointer,IGD_AsyncSocket_OnInterrupt *InterruptPtr, void **user, int *PAUSE);
void IGD_DestroyWebClient(void *object);

void IGD_WebClient_DestroyWebClientDataObject(IGD_WebClient_StateObject token);
struct packetheader *IGD_WebClient_GetHeaderFromDataObject(IGD_WebClient_StateObject token);

IGD_WebClient_RequestToken IGD_WebClient_PipelineRequestEx(
	IGD_WebClient_RequestManager WebClient, 
	struct sockaddr *RemoteEndpoint, 
	char *headerBuffer,
	int headerBufferLength,
	int headerBuffer_FREE,
	char *bodyBuffer,
	int bodyBufferLength,
	int bodyBuffer_FREE,
	IGD_WebClient_OnResponse OnResponse,
	void *user1,
	void *user2);

IGD_WebClient_RequestToken IGD_WebClient_PipelineRequest(
	IGD_WebClient_RequestManager WebClient, 
	struct sockaddr *RemoteEndpoint,
	struct packetheader *packet,
	IGD_WebClient_OnResponse OnResponse,
	void *user1,
	void *user2);

int IGD_WebClient_StreamRequestBody(
									 IGD_WebClient_RequestToken token, 
									 char *body,
									 int bodyLength, 
									 enum IGD_AsyncSocket_MemoryOwnership MemoryOwnership,
									 int done
									 );
IGD_WebClient_RequestToken IGD_WebClient_PipelineStreamedRequest(
									IGD_WebClient_RequestManager WebClient,
									struct sockaddr *RemoteEndpoint,
									struct packetheader *packet,
									IGD_WebClient_OnResponse OnResponse,
									IGD_WebClient_OnSendOK OnSendOK,
									void *user1,
									void *user2);


void IGD_WebClient_FinishedResponse_Server(IGD_WebClient_StateObject wcdo);
void IGD_WebClient_DeleteRequests(IGD_WebClient_RequestManager WebClientToken, struct sockaddr* addr);
void IGD_WebClient_Resume(IGD_WebClient_StateObject wcdo);
void IGD_WebClient_Pause(IGD_WebClient_StateObject wcdo);
void IGD_WebClient_Disconnect(IGD_WebClient_StateObject wcdo);
void IGD_WebClient_CancelRequest(IGD_WebClient_RequestToken RequestToken);
void IGD_WebClient_ResetUserObjects(IGD_WebClient_StateObject webstate, void *user1, void *user2);
IGD_WebClient_RequestToken IGD_WebClient_GetRequestToken_FromStateObject(IGD_WebClient_StateObject WebStateObject);
IGD_WebClient_StateObject IGD_WebClient_GetStateObjectFromRequestToken(IGD_WebClient_RequestToken token);

void IGD_WebClient_Parse_ContentRange(char *contentRange, int *Start, int *End, int *TotalLength);
enum IGD_WebClient_Range_Result IGD_WebClient_Parse_Range(char *Range, long *Start, long *Length, long TotalLength);

void IGD_WebClient_SetMaxConcurrentSessionsToServer(IGD_WebClient_RequestManager WebClient, int maxConnections);
void IGD_WebClient_SetUser(IGD_WebClient_RequestManager manager, void *user);
void* IGD_WebClient_GetUser(IGD_WebClient_RequestManager manager);
void* IGD_WebClient_GetChain(IGD_WebClient_RequestManager manager);

#ifndef MICROSTACK_NOTLS
void IGD_WebClient_SetTLS(IGD_WebClient_RequestManager manager, void *ssl_ctx, IGD_WebClient_OnSslConnection OnSslConnection);
#endif

// Added methods
int IGD_WebClient_GetLocalInterface(void* socketModule, struct sockaddr *localAddress);
int IGD_WebClient_GetRemoteInterface(void* socketModule, struct sockaddr *remoteAddress);

// OpenSSL supporting code
#ifndef MICROSTACK_NOTLS
X509*			IGD_WebClient_SslGetCert(void* socketModule);
STACK_OF(X509)*	IGD_WebClient_SslGetCerts(void* socketModule);
#endif

// Mesh specific code, used to store the NodeID in the web client
char* IGD_WebClient_GetCertificateHash(void* socketModule);
char* IGD_WebClient_SetCertificateHash(void* socketModule, char* ptr);
char* IGD_WebClient_GetCertificateHashEx(void* socketModule);

int IGD_WebClient_HasAllocatedClient(IGD_WebClient_RequestManager WebClient, struct sockaddr *remoteAddress);
int IGD_WebClient_GetActiveClientCount(IGD_WebClient_RequestManager WebClient);

#ifdef UPNP_DEBUG
char *IGD_WebClient_QueryWCDO(IGD_WebClient_RequestManager wcm, char *query);
#endif

#ifdef __cplusplus
}
#endif

#endif

