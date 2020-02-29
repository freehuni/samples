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

/*! \file IGD_WebServer.h 
	\brief MicroStack APIs for HTTP Server functionality
*/

#ifndef __IGD_WebServer__
#define __IGD_WebServer__
#include "IGD_Parsers.h"
#include "IGD_AsyncServerSocket.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \defgroup IGD_WebServer IGD_WebServer Module
	\{
*/

enum IGD_WebServer_Status
{
	IGD_WebServer_ALL_DATA_SENT						= 0,	/*!< All of the data has already been sent */
	IGD_WebServer_NOT_ALL_DATA_SENT_YET				= 1,	/*!< Not all of the data could be sent, but is queued to be sent as soon as possible */
	IGD_WebServer_SEND_RESULTED_IN_DISCONNECT		= -2,	/*!< A send operation resulted in the socket being closed */
	IGD_WebServer_INVALID_SESSION					= -3,	/*!< The specified IGD_WebServer_Session was invalid */
	IGD_WebServer_TRIED_TO_SEND_ON_CLOSED_SOCKET	= -4,	/*!< A send operation was attmepted on a closed socket */
};
/*! \typedef IGD_WebServer_ServerToken
	\brief The handle for an IGD_WebServer module
*/
typedef void* IGD_WebServer_ServerToken;

struct IGD_WebServer_Session;

/*! \typedef IGD_WebServer_Session_OnReceive
	\brief OnReceive Handler
	\param sender The associate \a IGD_WebServer_Session object
	\param InterruptFlag boolean indicating if the underlying chain/thread is disposing
	\param header The HTTP headers that were received
	\param bodyBuffer Pointer to the HTTP body
	\param[in,out] beginPointer Starting offset of the body buffer. Advance this pointer when the data is consumed.
	\param endPointer Length of available data pointed to by \a bodyBuffer
	\param done boolean indicating if the entire packet has been received
*/
typedef void (*IGD_WebServer_Session_OnReceive)(struct IGD_WebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, int done);
/*! \typedef IGD_WebServer_Session_OnDisconnect
	\brief Handler for when an IGD_WebServer_Session object is disconnected
	\param sender The \a IGD_WebServer_Session object that was disconnected
*/
typedef void (*IGD_WebServer_Session_OnDisconnect)(struct IGD_WebServer_Session *sender);
/*! \typedef IGD_WebServer_Session_OnSendOK
	\brief Handler for when pending send operations have completed
	\par
	<B>Note:</B> This handler will only be called after all pending data from any call(s) to 
	\a IGD_WebServer_Send, \a IGD_WebServer_Send_Raw, \a IGD_WebServer_StreamBody,
	\a IGD_WebServer_StreamHeader, and/or \a IGD_WebServer_StreamHeader_Raw, have completed. You will need to look at the return values
	of those methods, to determine if there is any pending data that still needs to be sent. That will determine if this handler will get called.
	\param sender The \a IGD_WebServer_Session that has completed sending all of the pending data
*/
typedef	void (*IGD_WebServer_Session_OnSendOK)(struct IGD_WebServer_Session *sender);

/*! \struct IGD_WebServer_Session
	\brief A structure representing the state of an HTTP Session
*/
struct IGD_WebServer_Session
{
	/*! \var OnReceive
		\brief A Function Pointer that is triggered whenever data is received
	*/
	IGD_WebServer_Session_OnReceive OnReceive;
	/*! \var OnDisconnect
		\brief A Function Pointer that is triggered when the session is disconnected
	*/
	IGD_WebServer_Session_OnDisconnect OnDisconnect;
	/*! \var OnSendOK
		\brief A Function Pointer that is triggered when the send buffer is emptied
	*/
	IGD_WebServer_Session_OnSendOK OnSendOK;
	void *Parent;

	/*! \var User
		\brief A reserved pointer that you can use for your own use
	*/
	void *User;
	/*! \var User2
		\brief A reserved pointer that you can use for your own use
	*/
	void *User2;
	/*! \var User3
		\brief A reserved pointer that you can use for your own use
	*/
	void *User3;
	/*! \var User4
		\brief A reserved pointer that you can use for your own use
	*/
	unsigned int User4;
	unsigned int User5;
	char* CertificateHashPtr; // Points to the certificate hash (next field) if set
	char CertificateHash[32]; // Used by the Mesh to store NodeID of this session

	void *Reserved1;	// AsyncServerSocket
	void *Reserved2;	// ConnectionToken
	void *Reserved3;	// WebClientDataObject
	void *Reserved7;	// VirtualDirectory
	int   Reserved4;	// Request Answered Flag (set by send)
	int   Reserved8;	// RequestAnswered Method Called
	int   Reserved5;	// Request Made Flag
	int   Reserved6;	// Close Override Flag
	int   Reserved9;	// Reserved for future use
	void *Reserved10;	// DisconnectFlagPointer

	sem_t Reserved11;	// Session Lock
	int   Reserved12;	// Reference Counter;
	int   Reserved13;	// Override VirDir Struct

	char *buffer;
	int bufferLength;
	int done;
	int SessionInterrupted;
};


void IGD_WebServer_AddRef(struct IGD_WebServer_Session *session);
void IGD_WebServer_Release(struct IGD_WebServer_Session *session);

/*! \typedef IGD_WebServer_Session_OnSession
	\brief New Session Handler
	\param SessionToken The new Session
	\param User The \a User object specified in \a IGD_WebServer_Create
*/
typedef void (*IGD_WebServer_Session_OnSession)(struct IGD_WebServer_Session *SessionToken, void *User);
/*! \typedef IGD_WebServer_VirtualDirectory
	\brief Request Handler for a registered Virtual Directory
	\param session The session that received the request
	\param header The HTTP headers
	\param bodyBuffer Pointer to the HTTP body
	\param[in,out] beginPointer Starting index of \a bodyBuffer. Advance this pointer as data is consumed
	\param endPointer Length of available data in \a bodyBuffer
	\param done boolean indicating that the entire packet has been read
	\param user The \a user specified in \a IGD_WebServer_Create
*/
typedef void (*IGD_WebServer_VirtualDirectory)(struct IGD_WebServer_Session *session, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, int done, void *user);

#ifndef MICROSTACK_NOTLS
void IGD_WebServer_SetTLS(IGD_WebServer_ServerToken object, void *ssl_ctx);
#endif

void IGD_WebServer_SetTag(IGD_WebServer_ServerToken WebServerToken, void *Tag);
void *IGD_WebServer_GetTag(IGD_WebServer_ServerToken WebServerToken);

IGD_WebServer_ServerToken IGD_WebServer_CreateEx(void *Chain, int MaxConnections, unsigned short PortNumber, int loopbackFlag, IGD_WebServer_Session_OnSession OnSession, void *User);
#define IGD_WebServer_Create(Chain, MaxConnections, PortNumber, OnSession, User) IGD_WebServer_CreateEx(Chain, MaxConnections, PortNumber, INADDR_ANY, OnSession, User)

int IGD_WebServer_RegisterVirtualDirectory(IGD_WebServer_ServerToken WebServerToken, char *vd, int vdLength, IGD_WebServer_VirtualDirectory OnVirtualDirectory, void *user);
int IGD_WebServer_UnRegisterVirtualDirectory(IGD_WebServer_ServerToken WebServerToken, char *vd, int vdLength);

enum IGD_WebServer_Status IGD_WebServer_Send(struct IGD_WebServer_Session *session, struct packetheader *packet);
enum IGD_WebServer_Status IGD_WebServer_Send_Raw(struct IGD_WebServer_Session *session, char *buffer, int bufferSize, int userFree, int done);

/*! \def IGD_WebServer_Session_GetPendingBytesToSend
	\brief Returns the number of outstanding bytes to be sent
	\param session The IGD_WebServer_Session object to query
*/
#define IGD_WebServer_Session_GetPendingBytesToSend(session) IGD_AsyncServerSocket_GetPendingBytesToSend(session->Reserved1,session->Reserved2)
/*! \def IGD_WebServer_Session_GetTotalBytesSent
	\brief Returns the total number of bytes sent
	\param session The IGD_WebServer_Session object to query
*/
#define IGD_WebServer_Session_GetTotalBytesSent(session) IGD_AsyncServerSocket_GetTotalBytesSent(session->Reserved1,session->Reserved2)
/*! \def IGD_WebServer_Session_ResetTotalBytesSent
	\brief Resets the total bytes set counter
	\param session The IGD_WebServer_Session object to query

*/
#define IGD_WebServer_Session_ResetTotalBytesSent(session) IGD_AsyncServerSocket_ResetTotalBytesSent(session->Reserved1,session->Reserved2)

unsigned short IGD_WebServer_GetPortNumber(IGD_WebServer_ServerToken WebServerToken);
int IGD_WebServer_GetLocalInterface(struct IGD_WebServer_Session *session, struct sockaddr *localAddress);
int IGD_WebServer_GetRemoteInterface(struct IGD_WebServer_Session *session, struct sockaddr *remoteAddress);

enum IGD_WebServer_Status IGD_WebServer_StreamHeader(struct IGD_WebServer_Session *session, struct packetheader *header);
enum IGD_WebServer_Status IGD_WebServer_StreamBody(struct IGD_WebServer_Session *session, char *buffer, int bufferSize, int userFree, int done);

enum IGD_WebServer_Status IGD_WebServer_StreamHeader_Raw(struct IGD_WebServer_Session *session, int StatusCode,char *StatusData,char *ResponseHeaders, int ResponseHeaders_FREE);
void IGD_WebServer_DisconnectSession(struct IGD_WebServer_Session *session);

void IGD_WebServer_Pause(struct IGD_WebServer_Session *session);
void IGD_WebServer_Resume(struct IGD_WebServer_Session *session);

void IGD_WebServer_OverrideReceiveHandler(struct IGD_WebServer_Session *session, IGD_WebServer_Session_OnReceive OnReceive);

void IGD_WebServer_StreamFile(struct IGD_WebServer_Session *session, FILE* pfile);

#ifdef __cplusplus
}
#endif

/* \} */
#endif

