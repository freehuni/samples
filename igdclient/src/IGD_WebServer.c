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
#define HTTPVERSION "1.1"

#define HTTPVERSION "1.1"

#if defined(WIN32) && !defined(_WIN32_WCE)
#define _CRTDBG_MAP_ALLOC
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
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
#include "IGD_WebServer.h"
#include "IGD_AsyncServerSocket.h"
#include "IGD_AsyncSocket.h"
#include "IGD_WebClient.h"

//#define HTTP_SESSION_IDLE_TIMEOUT 30

#ifdef IGD_WebServer_SESSION_TRACKING
void IGD_WebServer_SessionTrack(void *Session, char *msg)
{
#if defined(WIN32) || defined(_WIN32_WCE)
	char tempMsg[4096];
	wchar_t t[4096];
	size_t len;
	sprintf_s(tempMsg, 4096, "Session: %p   %s\r\n", Session, msg);
	mbstowcs_s(&len, t, 4096, tempMsg, 4096);
	OutputDebugString(t);
#else
	printf("Session: %x   %s\r\n",Session,msg);
#endif
}
#define SESSION_TRACK(Session,msg) IGD_WebServer_SessionTrack(Session,msg)
#else
#define SESSION_TRACK(Session,msg)
#endif
struct IGD_WebServer_VirDir_Data
{
	voidfp callback;
	void *user;
};

struct IGD_WebServer_StateModule
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);

	void *Chain;
	void *ServerSocket;
	void *LifeTime;
	void *User;
	void *Tag;

	void *VirtualDirectoryTable;

	void (*OnSession)(struct IGD_WebServer_Session *SessionToken, void *User);
};

/*! \fn IGD_WebServer_SetTag(IGD_WebServer_ServerToken object, void *Tag)
\brief Sets the user tag associated with the server
\param object The IGD_WebServer to associate the user tag with
\param Tag The user tag to associate
*/
void IGD_WebServer_SetTag(IGD_WebServer_ServerToken object, void *Tag)
{
	struct IGD_WebServer_StateModule *s = (struct IGD_WebServer_StateModule*)object;
	s->Tag = Tag;
}

/*! \fn IGD_WebServer_GetTag(IGD_WebServer_ServerToken object)
\brief Gets the user tag associated with the server
\param object The IGD_WebServer to query
\returns The associated user tag
*/
void *IGD_WebServer_GetTag(IGD_WebServer_ServerToken object)
{
	struct IGD_WebServer_StateModule *s = (struct IGD_WebServer_StateModule*)object;
	return(s->Tag);
}

//
// Internal method dispatched by a timer to idle out a session
//
// A session can idle in two ways. 
// 1.) A TCP connection is established, but a request isn't received within an allotted time period
// 2.) A request is answered, and another request isn't received with an allotted time period
// 
void IGD_WebServer_IdleSink(void *object)
{
	struct IGD_WebServer_Session *session = (struct IGD_WebServer_Session*)object;
	if (IGD_AsyncSocket_IsFree(session->Reserved2) == 0)
	{
		// This is OK, because we're on the MicroStackThread
		IGD_AsyncServerSocket_Disconnect(session->Reserved1, session->Reserved2);
	}
}

//
// Chain Destroy handler
//
void IGD_WebServer_Destroy(void *object)
{
	struct IGD_WebServer_StateModule *s = (struct IGD_WebServer_StateModule*)object;
	void *en;
	void *data;
	char *key;
	int keyLength;

	if (s->VirtualDirectoryTable != NULL)
	{
		//
		// If there are registered Virtual Directories, we need to free the resources
		// associated with them
		//
		en = IGD_HashTree_GetEnumerator(s->VirtualDirectoryTable);
		while (IGD_HashTree_MoveNext(en) == 0)
		{
			IGD_HashTree_GetValue(en, &key, &keyLength, &data);
			free(data);
		}
		IGD_HashTree_DestroyEnumerator(en);
		IGD_DestroyHashTree(s->VirtualDirectoryTable);
	}
}
//
// Internal method dispatched from the underlying WebClient engine
//
// <param name="WebReaderToken">The WebClient token</param>
// <param name="InterruptFlag">Flag indicating session was interrupted</param>
// <param name="header">The HTTP header structure</param>
// <param name="bodyBuffer">buffer pointing to HTTP body</param>
// <param name="beginPointer">buffer pointer offset</param>
// <param name="endPointer">buffer length</param>
// <param name="done">Flag indicating if the entire packet has been read</param>
// <param name="user1"></param>
// <param name="user2">The IGD_WebServer uses this to pass the IGD_WebServer_Session object</param>
// <param name="PAUSE">Flag to pause data reads on the underlying WebClient engine</param>
void IGD_WebServer_OnResponse(
	void *WebReaderToken,
	int InterruptFlag,
	struct packetheader *header,
	char *bodyBuffer,
	int *beginPointer,
	int endPointer,
	int done,
	void *user1,
	void *user2,
	int *PAUSE)
{
	struct IGD_WebServer_Session *ws = (struct IGD_WebServer_Session*)user2;
	struct IGD_WebServer_StateModule *wsm = (struct IGD_WebServer_StateModule*)ws->Parent;

	char *tmp;
	int tmpLength;
	struct parser_result *pr;
	int PreSlash = 0;

	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( user1 );

	if (header == NULL) return;

	ws->buffer = bodyBuffer + *beginPointer;
	ws->bufferLength = endPointer - *beginPointer;
	ws->done = done;

#if defined(MAX_HTTP_HEADER_SIZE) || defined(MAX_HTTP_PACKET_SIZE)
	if (ws->done == (int)IGD_WebClient_DoneCode_HeaderTooBig ||
		ws->done == (int)IGD_WebClient_DoneCode_BodyTooBig)
	{
		//
		// We need to return a 413 Error code for this condition, to be nice
		//
		{
			char body[255];
			int bodyLength;
			bodyLength = snprintf(body, 255, "HTTP/1.1 413 Request Too Big (MaxHeader=%d)\r\n\r\n", MAX_HTTP_HEADER_SIZE);
			IGD_AsyncSocket_Send(ws->Reserved2, body, bodyLength, IGD_AsyncSocket_MemoryOwnership_USER);
		}
	}
#endif
	//
	// Reserved4 = Request Answered Flag
	//	If this flag is set, the request was answered
	// Reserved5 = Request Made Flag
	//	If this flag is set, a request has been received
	//
	if (ws->Reserved4 != 0 || ws->Reserved5 == 0)
	{
		//
		// This session is no longer idle
		//
		ws->Reserved4 = 0;
		ws->Reserved5 = 1;
		ws->Reserved8 = 0;
		IGD_LifeTime_Remove(((struct IGD_WebServer_StateModule*)ws->Parent)->LifeTime, ws);
	}

	//
	// Check to make sure that the request contains a host header, as required
	// by RFC-2616, for HTTP/1.1 requests
	//
	if (done!=0 && header != NULL && header->Directive != NULL && atof(header->Version) > 1)
	{
		if (IGD_GetHeaderLine(header, "host", 4) == NULL)
		{
			//
			// Host header is missing
			//
			char body[255];
			int bodyLength;
			bodyLength = snprintf(body, 255, "HTTP/1.1 400 Bad Request (Missing Host Field)\r\n\r\n");
			IGD_WebServer_Send_Raw(ws, body, bodyLength, IGD_AsyncSocket_MemoryOwnership_USER, 1);
			return;
		}
	}

	//
	// Check Virtual Directory
	//
	if (wsm->VirtualDirectoryTable != NULL)
	{
		//
		// Reserved7 = Virtual Directory State Object
		//
		if (ws->Reserved7 == NULL)
		{
			//
			// See if we can find the virtual directory.
			// If we do, set the State Object, so future responses don't need to 
			// do it again
			//
			pr = IGD_ParseString(header->DirectiveObj, 0, header->DirectiveObjLength, "/", 1);
			if (pr->FirstResult->datalength == 0)
			{
				// Does not start with '/'
				tmp = pr->FirstResult->NextResult->data;
				tmpLength = pr->FirstResult->NextResult->datalength;
				PreSlash = 1;
			}
			else
			{
				// Starts with '/'
				tmp = pr->FirstResult->data;
				tmpLength = pr->FirstResult->datalength;
			}
			IGD_DestructParserResults(pr);
			//
			// Does the Virtual Directory Exist?
			//
			if (IGD_HasEntry(wsm->VirtualDirectoryTable,tmp,tmpLength)!=0)
			{
				//
				// Virtual Directory is defined
				//
				header->Reserved = header->DirectiveObj;
				header->DirectiveObj = tmp + tmpLength;
				header->DirectiveObjLength -= (tmpLength + PreSlash);
				//
				// Set the StateObject, then call the handler
				//
				ws->Reserved7 = IGD_GetEntry(wsm->VirtualDirectoryTable, tmp, tmpLength);
				if (ws->Reserved7 != NULL) ((IGD_WebServer_VirtualDirectory)((struct IGD_WebServer_VirDir_Data*)ws->Reserved7)->callback)(ws,header,bodyBuffer,beginPointer,endPointer,done,((struct IGD_WebServer_VirDir_Data*)ws->Reserved7)->user);
			}
			else if (ws->OnReceive!=NULL)
			{
				//
				// If the virtual directory doesn't exist, just call the main handler
				//
				ws->OnReceive(ws, InterruptFlag, header, bodyBuffer, beginPointer, endPointer, done);
			}
		}
		else
		{
			if (ws->Reserved13==0)
			{
				//
				// The state object was already set, so we know this is the handler to use. So easy!
				//
				((IGD_WebServer_VirtualDirectory)((struct IGD_WebServer_VirDir_Data*)ws->Reserved7)->callback)(ws,header,bodyBuffer,beginPointer,endPointer,done,((struct IGD_WebServer_VirDir_Data*)ws->Reserved7)->user);
			}
			else
			{
				ws->OnReceive(ws,InterruptFlag,header,bodyBuffer,beginPointer,endPointer,done);
			}
		}
	}
	else if (ws->OnReceive!=NULL)
	{
		//
		// Since there is no Virtual Directory lookup table, none were registered,
		// so we know we have no choice but to call the regular handler
		//
		ws->OnReceive(ws,InterruptFlag,header,bodyBuffer,beginPointer,endPointer,done);
	}


	//
	// Reserved8 = RequestAnswered method has been called
	//
	if (done!=0 && InterruptFlag==0 && header!=NULL && ws->Reserved8==0)
	{
		//
		// The request hasn't been satisfied yet, so stop reading from the socket until it is
		//
		*PAUSE=1;
	}
}

//
// Internal method dispatched from the underlying IGD_AsyncServerSocket module
//
// This is dispatched when the underlying buffer has been reallocated, which may
// neccesitate extra processing
// <param name="AsyncServerSocketToken">AsyncServerSocket token</param>
// <param name="ConnectionToken">Connection token (Underlying IGD_AsyncSocket)</param>
// <param name="user">The IGD_WebServer_Session object</param>
// <param name="offSet">Offset to the new buffer location</param>
void IGD_WebServer_OnBufferReAllocated(void *AsyncServerSocketToken, void *ConnectionToken, void *user, ptrdiff_t offSet)
{
	struct IGD_WebServer_Session *ws = (struct IGD_WebServer_Session*)user;

	UNREFERENCED_PARAMETER( AsyncServerSocketToken );
	UNREFERENCED_PARAMETER( ConnectionToken );

	//
	// We need to pass this down to our internal IGD_WebClient for further processing
	// Reserved2 = ConnectionToken
	// Reserved3 = WebClientDataObject
	//
	IGD_WebClient_OnBufferReAllocate(ws->Reserved2,ws->Reserved3,offSet);
}
#if defined(MAX_HTTP_PACKET_SIZE)
void IGD_WebServer_OnBufferSizeExceeded(void *connectionToken, void *user)
{
	//
	// We need to return a 413 Error code for this condition, to be nice
	//

	char body[255];
	int bodyLength;
	bodyLength = sprintf(body, "HTTP/1.1 413 Request Too Big (MaxPacketSize=%d)\r\n\r\n", MAX_HTTP_PACKET_SIZE);
	IGD_AsyncSocket_Send(connectionToken, body, bodyLength, IGD_AsyncSocket_MemoryOwnership_USER);
}
#endif
//
// Internal method dispatched from the underlying IGD_AsyncServerSocket module
//
// <param name="AsyncServerSocketModule">AsyncServerSocket token</param>
// <param name="ConnectionToken">Connection token (Underlying IGD_AsyncSocket)</param>
// <param name="user">User object that can be set. (used here for IGD_WebServer_Session</param>
void IGD_WebServer_OnConnect(void *AsyncServerSocketModule, void *ConnectionToken, void **user)
{
	struct IGD_WebServer_StateModule *wsm;
	struct IGD_WebServer_Session *ws;

	//
	// Create a new IGD_WebServer_Session to represent this connection
	//
	wsm = (struct IGD_WebServer_StateModule*)IGD_AsyncServerSocket_GetTag(AsyncServerSocketModule);
	if ((ws = (struct IGD_WebServer_Session*)malloc(sizeof(struct IGD_WebServer_Session))) == NULL) ILIBCRITICALEXIT(254);
	memset(ws, 0, sizeof(struct IGD_WebServer_Session));
	sem_init(&(ws->Reserved11), 0, 1); // Initialize the SessionLock
	ws->Reserved12 = 1; // Reference Counter, Initial count should be 1

	//printf("#### ALLOCATED (%d) ####\r\n", ConnectionToken);

	ws->Parent = wsm;
	ws->Reserved1 = AsyncServerSocketModule;
	ws->Reserved2 = ConnectionToken;
	ws->Reserved3 = IGD_CreateWebClientEx(&IGD_WebServer_OnResponse, ConnectionToken, wsm, ws);
	ws->User = wsm->User;
	*user = ws;
#if defined(MAX_HTTP_PACKET_SIZE)
	IGD_AsyncSocket_SetMaximumBufferSize(ConnectionToken, MAX_HTTP_PACKET_SIZE, &IGD_WebServer_OnBufferSizeExceeded, ws);
#endif	
	//
	// We want to know when this connection reallocates its internal buffer, because we may
	// need to fix a few things
	//
	IGD_AsyncServerSocket_SetReAllocateNotificationCallback(AsyncServerSocketModule, ConnectionToken, &IGD_WebServer_OnBufferReAllocated);

	//
	// Add a timed callback, because if we don't receive a request within a specified
	// amount of time, we want to close the socket, so we don't waste resources
	//
	IGD_LifeTime_Add(wsm->LifeTime, ws, HTTP_SESSION_IDLE_TIMEOUT, &IGD_WebServer_IdleSink, NULL);

	SESSION_TRACK(ws, "* Allocated *");
	SESSION_TRACK(ws, "AddRef");
	//
	// Inform the user that a new session was established
	//
	if (wsm->OnSession != NULL) wsm->OnSession(ws, wsm->User);
}

//
// Internal method dispatched from the underlying AsyncServerSocket engine
// 
// <param name="AsyncServerSocketModule">The IGD_AsyncServerSocket token</param>
// <param name="ConnectionToken">The IGD_AsyncSocket connection token</param>
// <param name="user">The IGD_WebServer_Session object</param>
void IGD_WebServer_OnDisconnect(void *AsyncServerSocketModule, void *ConnectionToken, void *user)
{
	struct IGD_WebServer_Session *ws = (struct IGD_WebServer_Session*)user;

	UNREFERENCED_PARAMETER( AsyncServerSocketModule );
	UNREFERENCED_PARAMETER( ConnectionToken );

#ifdef _DEBUG
	//printf("#### RELEASE (%d) ####\r\n", ConnectionToken);
	if (ws == NULL) { PRINTERROR(); ILIBCRITICALEXIT(253); }
#endif

	//
	// If this was a non-persistent connection, the response will queue this up to be called.
	// Some clients may close the socket anyway, before the server does, so we should remove this
	// so we don't accidently close the socket again.
	//
	IGD_LifeTime_Remove(((struct IGD_WebServer_StateModule*)ws->Parent)->LifeTime, ws->Reserved2);
	if (ws->Reserved10 != NULL) ws->Reserved10 = NULL;

	//
	// Reserved4 = RequestAnsweredFlag
	// Reserved5 = RequestMadeFlag
	//
	if (ws->Reserved4 != 0 || ws->Reserved5 == 0)
	{
		IGD_LifeTime_Remove(((struct IGD_WebServer_StateModule*)ws->Parent)->LifeTime, ws);
		ws->Reserved4 = 0;
	}

	SESSION_TRACK(ws, "OnDisconnect");
	//
	// Notify the user that this session disconnected
	//
	if (ws->OnDisconnect != NULL) ws->OnDisconnect(ws);

	IGD_WebServer_Release((struct IGD_WebServer_Session *)user);
}
//
// Internal method dispatched from the underlying IGD_AsyncServerSocket engine
//
// <param name="AsyncServerSocketModule">The IGD_AsyncServerSocket token</param>
// <param name="ConnectionToken">The IGD_AsyncSocket connection token</param>
// <param name="buffer">The receive buffer</param>
// <param name="p_beginPointer">buffer offset</param>
// <param name="endPointer">buffer length</param>
// <param name="OnInterrupt">Function Pointer to handle Interrupts</param>
// <param name="user">IGD_WebServer_Session object</param>
// <param name="PAUSE">Flag to pause data reads on the underlying AsyncSocket engine</param>
void IGD_WebServer_OnReceive(void *AsyncServerSocketModule, void *ConnectionToken, char* buffer,int *p_beginPointer, int endPointer, void (**OnInterrupt)(void *AsyncServerSocketMoudle, void *ConnectionToken, void *user), void **user, int *PAUSE)
{
	//
	// Pipe the data down to our internal WebClient engine, which will do
	// all the HTTP processing
	//
	struct IGD_WebServer_Session *ws = (struct IGD_WebServer_Session*)(*user);

	UNREFERENCED_PARAMETER( AsyncServerSocketModule );
	UNREFERENCED_PARAMETER( OnInterrupt );

	IGD_WebClient_OnData(ConnectionToken,buffer,p_beginPointer,endPointer,NULL,&(ws->Reserved3),PAUSE);
}

//
// Internal method dispatched from the underlying IGD_AsyncServerSocket engine, signaling an interrupt
//
// <param name="AsyncServerSocketModule">The IGD_AsyncServerSocket token</param>
// <param name="ConnectionToken">The IGD_AsyncSocket connection token</param>
// <param name="user">The IGD_WebServer_Session object</param>
void IGD_WebServer_OnInterrupt(void *AsyncServerSocketModule, void *ConnectionToken, void *user)
{
	struct IGD_WebServer_Session *session = (struct IGD_WebServer_Session*)user;
	if (session == NULL) return;
	session->SessionInterrupted = 1;

	UNREFERENCED_PARAMETER( AsyncServerSocketModule );
	UNREFERENCED_PARAMETER( ConnectionToken );

	// This is ok, because this is MicroStackThread
	IGD_WebClient_DestroyWebClientDataObject(session->Reserved3);
}

//
// Internal method called when a request has been answered. Dispatched from Send routines
//
// <param name="session">The IGD_WebServer_Session object</param>
// <returns>Flag indicating if the session was closed</returns>
int IGD_WebServer_RequestAnswered(struct IGD_WebServer_Session *session)
{
	struct packetheader *hdr = IGD_WebClient_GetHeaderFromDataObject(session->Reserved3);
	struct packetheader_field_node *f;
	int PersistentConnection = 0;
	if (hdr == NULL) return 0;

	//
	// Reserved7 = Virtual Directory State Object
	//	We delete this, because the request is finished, so we don't need to direct
	//	data to this handler anymore. It needs to be recalculated next time
	//
	session->Reserved7 = NULL;

	//
	// Reserved8 = RequestAnswered method called
	//If this is set, this method was already called, so we can just exit
	//
	if (session->Reserved8 != 0) return(0);

	//
	// Set the flags, so if this re-enters, we don't process this again
	//
	session->Reserved8 = 1;
	f = hdr->FirstField;

	//
	// Reserved5 = Request Made. Since the request was answered, we can clear this.
	//
	session->Reserved5 = 0;

	//
	// Reserved6 = CloseOverrideFlag
	//	which means the session must be closed when request is complete
	//
	if (session->Reserved6==0)
	{
		//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
		if (hdr->VersionLength==3 && memcmp(hdr->Version,"1.0",3)!=0)
		{
			// HTTP 1.1+ , Check for CLOSE token
			PersistentConnection = 1;
			while (f!=NULL)
			{
				if (f->FieldLength==10 && strncasecmp(f->Field,"CONNECTION",10)==0)
				{
					if (f->FieldDataLength==5 && strncasecmp(f->FieldData,"CLOSE",5)==0)
					{
						PersistentConnection = 0;
						break;
					}
				}
				f = f->NextField;
			}
		}
		//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}
	}

	if (PersistentConnection==0)
	{
		//
		// Ensure calling on MicroStackThread. This will just result dispatching the callback on
		// the microstack thread
		//
		IGD_LifeTime_Add(((struct IGD_WebServer_StateModule*)session->Parent)->LifeTime, session->Reserved2, 0, &IGD_AsyncSocket_Disconnect, NULL);
	}
	else
	{
		//
		// This is a persistent connection. Set a timed callback, to idle this session if necessary
		//
		IGD_LifeTime_Add(((struct IGD_WebServer_StateModule*)session->Parent)->LifeTime, session, HTTP_SESSION_IDLE_TIMEOUT, &IGD_WebServer_IdleSink, NULL);
		IGD_WebClient_FinishedResponse_Server(session->Reserved3);
		//
		// Since we're done with this request, resume the underlying socket, so we can continue
		//
		IGD_WebClient_Resume(session->Reserved3);
	}
	return(PersistentConnection==0?IGD_WebServer_SEND_RESULTED_IN_DISCONNECT:0);
}

//
// Internal method dispatched from the underlying IGD_AsyncServerSocket engine
//
// <param name="AsyncServerSocketModule">The IGD_AsyncServerSocket token</param>
// <param name="ConnectionToken">The IGD_AsyncSocket connection token</param>
// <param name="user">The IGD_WebServer_Session object</param>
void IGD_WebServer_OnSendOK(void *AsyncServerSocketModule, void *ConnectionToken, void *user)
{
	int flag = 0;
	struct IGD_WebServer_Session *session = (struct IGD_WebServer_Session*)user;

	UNREFERENCED_PARAMETER( AsyncServerSocketModule );
	UNREFERENCED_PARAMETER( ConnectionToken );

	//
	// Reserved4 = RequestAnsweredFlag
	//
	if (session->Reserved4!=0)
	{
		// This is normally called when the response was sent. But since it couldn't get through
		// the first time, this method gets dispatched when it did, so now we have to call it.
		flag = IGD_WebServer_RequestAnswered(session);
	}
	if (session->OnSendOK!=NULL && flag != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT)
	{
		// Pass this event on, if everything is ok
		session->OnSendOK(session);
	}
}

#ifndef MICROSTACK_NOTLS
void IGD_WebServer_SetTLS(IGD_WebServer_ServerToken object, void *ssl_ctx)
{
	struct IGD_WebServer_StateModule *module = (struct IGD_WebServer_StateModule*)object;
	IGD_AsyncServerSocket_SetSSL_CTX(module->ServerSocket, ssl_ctx);
}
#endif

/*! \fn IGD_WebServer_Create(void *Chain, int MaxConnections, int PortNumber,IGD_WebServer_Session_OnSession OnSession, void *User)
\brief Constructor for IGD_WebServer
\param Chain The Chain to add this module to. (Chain must <B>not</B> be running)
\param MaxConnections The maximum number of simultaneous connections
\param PortNumber The Port number to listen to (0 = Random)
\param OnSession Function Pointer to dispatch on when new Sessions are established
\param User User state object to pass to OnSession
*/
IGD_WebServer_ServerToken IGD_WebServer_CreateEx(void *Chain, int MaxConnections, unsigned short PortNumber, int loopbackFlag, IGD_WebServer_Session_OnSession OnSession, void *User)
{
	struct IGD_WebServer_StateModule *RetVal = (struct IGD_WebServer_StateModule*)malloc(sizeof(struct IGD_WebServer_StateModule));

	if (RetVal == NULL) { PRINTERROR(); return NULL; }
	memset(RetVal, 0, sizeof(struct IGD_WebServer_StateModule));
	RetVal->Destroy = &IGD_WebServer_Destroy;
	RetVal->Chain = Chain;
	RetVal->OnSession = OnSession;

	//
	// Create the underling IGD_AsyncServerSocket
	//
	RetVal->ServerSocket = IGD_CreateAsyncServerSocketModule(
		Chain,
		MaxConnections,
		PortNumber,
		INITIAL_BUFFER_SIZE,
		loopbackFlag, 
		&IGD_WebServer_OnConnect,			// OnConnect
		&IGD_WebServer_OnDisconnect,		// OnDisconnect
		&IGD_WebServer_OnReceive,			// OnReceive
		&IGD_WebServer_OnInterrupt,			// OnInterrupt
		&IGD_WebServer_OnSendOK				// OnSendOK
		);

	if (RetVal->ServerSocket == NULL) { free(RetVal); return NULL; }

	//
	// Set ourselves in the User tag of the underlying IGD_AsyncServerSocket
	//
	IGD_AsyncServerSocket_SetTag(RetVal->ServerSocket, RetVal);
	RetVal->LifeTime = IGD_GetBaseTimer(Chain); //IGD_CreateLifeTime(Chain);
	RetVal->User = User;
	IGD_AddToChain(Chain, RetVal);
	return RetVal;
}

/*! \fn IGD_WebServer_GetPortNumber(IGD_WebServer_ServerToken WebServerToken)
\brief Returns the port number that this module is listening to
\param WebServerToken The IGD_WebServer to query
\returns The listening port number
*/
unsigned short IGD_WebServer_GetPortNumber(IGD_WebServer_ServerToken WebServerToken)
{
	struct IGD_WebServer_StateModule *WSM = (struct IGD_WebServer_StateModule*) WebServerToken;
	return(IGD_AsyncServerSocket_GetPortNumber(WSM->ServerSocket));
}

/*! \fn IGD_WebServer_Send(struct IGD_WebServer_Session *session, struct packetheader *packet)
\brief Send a response on a Session
\param session The IGD_WebServer_Session to send the response on
\param packet The packet to respond with
\returns Flag indicating send status.
*/
enum IGD_WebServer_Status IGD_WebServer_Send(struct IGD_WebServer_Session *session, struct packetheader *packet)
{
	char *buffer;
	int bufferSize;
	int RetVal = 0;

	if (session == NULL || (session != NULL && session->SessionInterrupted != 0)) 
	{
		IGD_DestructPacket(packet);
		return(IGD_WebServer_INVALID_SESSION);
	}
	session->Reserved4 = 1;
	bufferSize = IGD_GetRawPacket(packet, &buffer);

	if ((RetVal = IGD_AsyncServerSocket_Send(session->Reserved1, session->Reserved2, buffer, bufferSize, 0)) == 0)
	{
		// Completed Send
		RetVal = IGD_WebServer_RequestAnswered(session);
	}
	IGD_DestructPacket(packet);
	return(RetVal);
}

/*! \fn IGD_WebServer_Send_Raw(struct IGD_WebServer_Session *session, char *buffer, int bufferSize, int userFree, int done)
\brief Send a response on a Session, directly specifying the buffers to send
\param session The IGD_WebServer_Session to send the response on
\param buffer The buffer to send
\param bufferSize The length of the buffer
\param userFree The ownership flag of the buffer
\param done Flag indicating if this is everything
\returns Send Status
*/
enum IGD_WebServer_Status IGD_WebServer_Send_Raw(struct IGD_WebServer_Session *session, char *buffer, int bufferSize, int userFree, int done)
{
	int RetVal=0;
	if (session == NULL || (session != NULL && session->SessionInterrupted != 0)) 
	{
		if (userFree == IGD_AsyncSocket_MemoryOwnership_CHAIN) { free(buffer); }
		return(IGD_WebServer_INVALID_SESSION);
	}
	session->Reserved4 = done;

	RetVal = IGD_AsyncServerSocket_Send(session->Reserved1, session->Reserved2, buffer, bufferSize, userFree);
	if (RetVal == 0 && done != 0)
	{
		// Completed Send
		RetVal = IGD_WebServer_RequestAnswered(session);
	}
	return(RetVal);
}

/*! \fn IGD_WebServer_StreamHeader_Raw(struct IGD_WebServer_Session *session, int StatusCode,char *StatusData,char *ResponseHeaders, int ResponseHeaders_FREE)
\brief Streams the HTTP header response on a session, directly specifying the buffer
\par
\b DO \b NOT specify Content-Length or Transfer-Encoding.
\param session The IGD_WebServer_Session to send the response on
\param StatusCode The HTTP status code, eg: \b 200
\param StatusData The HTTP status data, eg: \b OK
\param ResponseHeaders Additional HTTP header fields
\param ResponseHeaders_FREE Ownership flag of the addition http header fields
\returns Send Status
*/
enum IGD_WebServer_Status IGD_WebServer_StreamHeader_Raw(struct IGD_WebServer_Session *session, int StatusCode,char *StatusData,char *ResponseHeaders, int ResponseHeaders_FREE)
{
	int len;
	char *temp;
	int RetVal;
	int tempLength;
	char *buffer;
	int bufferLength;
	struct packetheader *hdr;
	struct parser_result *pr,*pr2;
	struct parser_result_field *prf;

	if (session == NULL || (session != NULL && session->SessionInterrupted != 0)) 
	{
		if (ResponseHeaders_FREE == IGD_AsyncSocket_MemoryOwnership_CHAIN) { free(ResponseHeaders); }
		return(IGD_WebServer_INVALID_SESSION);
	}

	hdr = IGD_WebClient_GetHeaderFromDataObject(session->Reserved3);
	if (hdr == NULL)
	{
		if (ResponseHeaders_FREE == IGD_AsyncSocket_MemoryOwnership_CHAIN) { free(ResponseHeaders); }
		return(IGD_WebServer_INVALID_SESSION);
	}

	//
	// Allocate the response header buffer
	// ToDo: May want to make the response version dynamic or at least #define
	//
	buffer = (char*)malloc(20 + strlen(StatusData));
	if (buffer == NULL) ILIBCRITICALEXIT(254);
	bufferLength = snprintf(buffer, 20 + strlen(StatusData), "HTTP/%s %d %s", HTTPVERSION, StatusCode, StatusData);

	//
	// Send the first portion of the headers across
	//
	RetVal = IGD_WebServer_Send_Raw(session, buffer, bufferLength, 0, 0);
	if (RetVal != IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT)
	{
		//
		// The Send went through
		//
		//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
		if (!(hdr->VersionLength == 3 && memcmp(hdr->Version, "1.0", 3) == 0))
		{
			//
			// If this was not an HTTP/1.0 response, then we need to chunk
			//
			RetVal = IGD_WebServer_Send_Raw(session,"\r\nTransfer-Encoding: chunked",28,1,0);
		}
		else
		{
			//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}
			//
			// Since we are streaming over HTTP/1.0 , we are required to close the socket when done
			//
			session->Reserved6 = 1;
			//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
		}
		//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}
		if (ResponseHeaders!=NULL && RetVal != IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT)
		{
			//
			// Send the user specified headers
			//
			len = (int)strlen(ResponseHeaders);
			if (len<MAX_HEADER_LENGTH)
			{
				RetVal = IGD_WebServer_Send_Raw(session,ResponseHeaders,len,ResponseHeaders_FREE,0);
			}
			else
			{
				pr = IGD_ParseString(ResponseHeaders,0,len,"\r\n",2);
				prf = pr->FirstResult;
				while (prf != NULL)
				{
					if (prf->datalength != 0)
					{
						pr2 = IGD_ParseString(prf->data, 0, prf->datalength, ":", 1);
						if (pr2->NumResults!=1)
						{
							RetVal = IGD_WebServer_Send_Raw(session, "\r\n", 2, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
							if (RetVal != IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT)
							{
								RetVal = IGD_WebServer_Send_Raw(session,pr2->FirstResult->data,pr2->FirstResult->datalength+1,IGD_AsyncSocket_MemoryOwnership_USER,0);
								if (RetVal != IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT)
								{
									tempLength = IGD_FragmentText(prf->data+pr2->FirstResult->datalength+1,prf->datalength-pr2->FirstResult->datalength-1,"\r\n ",3,MAX_HEADER_LENGTH,&temp);
									RetVal = IGD_WebServer_Send_Raw(session,temp,tempLength,IGD_AsyncSocket_MemoryOwnership_CHAIN,0);
								}
								else
								{
									IGD_DestructParserResults(pr2);
									break;
								}
							}
							else
							{
								IGD_DestructParserResults(pr2);
								break;
							}
						}
						IGD_DestructParserResults(pr2);
					}
					prf = prf->NextResult;
				}
				IGD_DestructParserResults(pr);
			}
		}
		if (RetVal != IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT)
		{
			//
			// Send the Header Terminator
			//
			return(IGD_WebServer_Send_Raw(session, "\r\n\r\n", 4, 1, 0));
		}
		else
		{
			if (RetVal != 0 && session->Reserved10 != NULL)
			{
				session->Reserved10 = NULL;
			}
			return(RetVal);
		}
	}
	//
	// ToDo: May want to check logic if the sends didn't go through
	//
	if (RetVal!=0 && session->Reserved10 != NULL)
	{
		session->Reserved10 = NULL;
	}
	return(RetVal);
}

/*! \fn IGD_WebServer_StreamHeader(struct IGD_WebServer_Session *session, struct packetheader *header)
\brief Streams the HTTP header response on a session
\par
\b DO \b NOT specify Transfer-Encoding.
\param session The IGD_WebServer_Session to send the response on
\param header The headers to return
\returns Send Status
*/
enum IGD_WebServer_Status IGD_WebServer_StreamHeader(struct IGD_WebServer_Session *session, struct packetheader *header)
{
	struct packetheader *hdr;
	int bufferLength;
	char *buffer;
	int RetVal;

	if (session == NULL || (session != NULL && session->SessionInterrupted != 0)) 
	{
		IGD_DestructPacket(header);
		return(IGD_WebServer_INVALID_SESSION);
	}

	hdr = IGD_WebClient_GetHeaderFromDataObject(session->Reserved3);
	if (hdr == NULL)
	{
		IGD_DestructPacket(header);
		return(IGD_WebServer_INVALID_SESSION);
	}

	//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
	if (!(hdr->VersionLength == 3 && memcmp(hdr->Version, "1.0", 3)==0))
	{
		//
		// If this isn't an HTTP/1.0 connection, remove content-length, and chunk the response
		//
		IGD_DeleteHeaderLine(header, "Content-Length", 14);
		//
		// Add the Transfer-Encoding header
		//
		IGD_AddHeaderLine(header, "Transfer-Encoding", 17, "chunked", 7);
	}
	else
	{
		//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}
		// Check to see if they gave us a Content-Length
		if (IGD_GetHeaderLine(hdr, "Content-Length", 14)==NULL)
		{
			//
			// If it wasn't, we'll set the CloseOverrideFlag, because in order to be compliant
			// we must close the socket when done
			//
			session->Reserved6 = 1;
		}
		//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
	}
	//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}
	//
	// Grab the bytes and send it
	//
	bufferLength = IGD_GetRawPacket(header, &buffer);
	//
	// Since IGD_GetRawPacket allocates memory, we give ownership to the MicroStack, and
	// let it take care of it
	//
	RetVal = IGD_WebServer_Send_Raw(session, buffer, bufferLength, 0, 0);
	IGD_DestructPacket(header);
	if (RetVal!=0 && session->Reserved10 != NULL)
	{
		session->Reserved10 = NULL;
	}
	return(RetVal);
}

/*! \fn IGD_WebServer_StreamBody(struct IGD_WebServer_Session *session, char *buffer, int bufferSize, int userFree, int done)
\brief Streams the HTTP body on a session
\param session The IGD_WebServer_Session to send the response on
\param buffer The buffer to send
\param bufferSize The size of the buffer
\param userFree The ownership flag of the buffer
\param done Flag indicating if this is everything
\returns Send Status
*/
enum IGD_WebServer_Status IGD_WebServer_StreamBody(struct IGD_WebServer_Session *session, char *buffer, int bufferSize, int userFree, int done)
{
	struct packetheader *hdr;
	char *hex;
	int hexLen;
	int RetVal=0;

	if (session==NULL || (session!=NULL && session->SessionInterrupted!=0)) 
	{
		if (userFree == IGD_AsyncSocket_MemoryOwnership_CHAIN) { free(buffer); }
		return(IGD_WebServer_INVALID_SESSION);
	}
	
	hdr = IGD_WebClient_GetHeaderFromDataObject(session->Reserved3);
	if (hdr == NULL)
	{
		if (userFree == IGD_AsyncSocket_MemoryOwnership_CHAIN) { free(buffer); }
		return(IGD_WebServer_INVALID_SESSION);
	}

	//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
	if (hdr->VersionLength == 3 && memcmp(hdr->Version,"1.0",3) == 0)
	{
		//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}
		//
		// This is HTTP/1.0 , so we don't need to do anything special
		//
		if (bufferSize > 0)
		{
			//
			// If there is actually something to send, then send it
			//
			RetVal = IGD_WebServer_Send_Raw(session,buffer,bufferSize,userFree,done);
		}
		else if (done != 0)
		{
			//
			// Nothing to send?
			//
			RetVal = IGD_WebServer_RequestAnswered(session);
		}
		//{{{ REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT--> }}}
	}
	else
	{
		//
		// This is HTTP/1.1+ , so we need to chunk the body
		//
		if (bufferSize>0)
		{
			//
			// Calculate the length of the body in hex, and create the chunk header
			//
			if ((hex = (char*)malloc(16)) == NULL) ILIBCRITICALEXIT(254);
			hexLen = snprintf(hex, 16, "%X\r\n", bufferSize);
			RetVal = IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;

			//
			// Send the chunk header
			//
			if (IGD_WebServer_Send_Raw(session, hex, hexLen, 0, 0)!=IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR)
			{
				//
				// Send the data
				//
				if (IGD_WebServer_Send_Raw(session, buffer, bufferSize, userFree, 0)!=IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR)
				{
					//
					// The data must be terminated with a CRLF, (don't ask why, it just does)
					//
					RetVal = IGD_WebServer_Send_Raw(session, "\r\n", 2, 1, 0);
				}
				else
				{
					RetVal = IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;
				}
			}
			else
			{
				RetVal = IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;
				//
				// We didn't send the buffer yet, so we need to free it if
				// we own the memory
				//
				if (userFree == IGD_AsyncSocket_MemoryOwnership_CHAIN)
				{
					free(buffer);
				}
			}
			//
			// These protections with the IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR check is
			// to prevent broken pipe errors
			//

		}
		if (done!=0 && RetVal != IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR && RetVal != IGD_WebServer_SEND_RESULTED_IN_DISCONNECT &&
			!(hdr->DirectiveLength==4 && strncasecmp(hdr->Directive,"HEAD",4)==0))
		{
			//
			// Terminate the chunk
			//
			RetVal = IGD_WebServer_Send_Raw(session,"0\r\n\r\n",5,1,1);
		}
		else if (done!=0 && RetVal >=0)
		{
			RetVal = IGD_WebServer_RequestAnswered(session);
		}
	}
	//{{{ <--REMOVE_THIS_FOR_HTTP/1.0_ONLY_SUPPORT }}}

	if (RetVal!=0 && session->Reserved10 != NULL)
	{
		session->Reserved10 = NULL;
	}
	return(RetVal);
}


/*! \fn IGD_WebServer_GetRemoteInterface(struct IGD_WebServer_Session *session)
\brief Returns the remote interface of an HTTP session
\param session The IGD_WebServer_Session to query
\returns The remote interface
*/
int IGD_WebServer_GetRemoteInterface(struct IGD_WebServer_Session *session, struct sockaddr *remoteAddress)
{
	return IGD_AsyncSocket_GetRemoteInterface(session->Reserved2, remoteAddress);
}

/*! \fn IGD_WebServer_GetLocalInterface(struct IGD_WebServer_Session *session)
\brief Returns the local interface of an HTTP session
\param session The IGD_WebServer_Session to query
\returns The local interface
*/
int IGD_WebServer_GetLocalInterface(struct IGD_WebServer_Session *session, struct sockaddr *localAddress)
{
	return IGD_AsyncSocket_GetLocalInterface(session->Reserved2, localAddress);
}



/*! \fn IGD_WebServer_RegisterVirtualDirectory(IGD_WebServer_ServerToken WebServerToken, char *vd, int vdLength, IGD_WebServer_VirtualDirectory OnVirtualDirectory, void *user)
\brief Registers a Virtual Directory with the IGD_WebServer
\param WebServerToken The IGD_WebServer to register with
\param vd The virtual directory path
\param vdLength The length of the path
\param OnVirtualDirectory The Virtual Directory handler
\param user User state info to pass on
\returns 0 if successful, nonzero otherwise
*/
int IGD_WebServer_RegisterVirtualDirectory(IGD_WebServer_ServerToken WebServerToken, char *vd, int vdLength, IGD_WebServer_VirtualDirectory OnVirtualDirectory, void *user)
{
	struct IGD_WebServer_VirDir_Data *data;
	struct IGD_WebServer_StateModule *s = (struct IGD_WebServer_StateModule*)WebServerToken;
	if (s->VirtualDirectoryTable == NULL)
	{
		//
		// If no Virtual Directories have been registered yet, we need to initialize
		// the lookup table
		//
		s->VirtualDirectoryTable = IGD_InitHashTree();
	}

	if (IGD_HasEntry(s->VirtualDirectoryTable, vd, vdLength)!=0)
	{
		//
		// This Virtual Directory was already registered
		//
		return(1);
	}
	else
	{
		//
		// Add the necesary info into the lookup table
		//
		data = (struct IGD_WebServer_VirDir_Data*)malloc(sizeof(struct IGD_WebServer_VirDir_Data));
		if (data == NULL) ILIBCRITICALEXIT(254);
		data->callback = (voidfp)OnVirtualDirectory;
		data->user = user;
		IGD_AddEntry(s->VirtualDirectoryTable, vd, vdLength,data);
	}
	return(0);
}

/*! \fn IGD_WebServer_UnRegisterVirtualDirectory(IGD_WebServer_ServerToken WebServerToken, char *vd, int vdLength)
\brief UnRegisters a Virtual Directory from the IGD_WebServer
\param WebServerToken The IGD_WebServer to unregister from
\param vd The virtual directory path
\param vdLength The length of the path
\returns 0 if successful, nonzero otherwise
*/
int IGD_WebServer_UnRegisterVirtualDirectory(IGD_WebServer_ServerToken WebServerToken, char *vd, int vdLength)
{
	struct IGD_WebServer_StateModule *s = (struct IGD_WebServer_StateModule*)WebServerToken;
	if (IGD_HasEntry(s->VirtualDirectoryTable,vd,vdLength)!=0)
	{
		//
		// The virtual directory registry was found, delete it
		//
		free(IGD_GetEntry(s->VirtualDirectoryTable,vd,vdLength));
		IGD_DeleteEntry(s->VirtualDirectoryTable,vd,vdLength);
		return(0);
	}
	else
	{
		//
		// Couldn't find the virtual directory registry
		//
		return(1);
	}
}

/*! \fn IGD_WebServer_AddRef(struct IGD_WebServer_Session *session)
\brief Reference Counter for an \a IGD_WebServer_Session object
\param session The IGD_WebServer_Session object
*/
void IGD_WebServer_AddRef(struct IGD_WebServer_Session *session)
{
	SESSION_TRACK(session,"AddRef");
	sem_wait(&(session->Reserved11));
	++session->Reserved12;
	sem_post(&(session->Reserved11));
}

/*! \fn IGD_WebServer_Release(struct IGD_WebServer_Session *session)
\brief Decrements reference counter for \a IGD_WebServer_Session object
\par
When the counter reaches 0, the object is freed
\param session The IGD_WebServer_Session object
*/
void IGD_WebServer_Release(struct IGD_WebServer_Session *session)
{
	int OkToFree = 0;

	SESSION_TRACK(session,"Release");
	sem_wait(&(session->Reserved11));
	if (--session->Reserved12 == 0)
	{
		//
		// There are no more outstanding references, so we can
		// free this thing
		//
		OkToFree = 1;
	}
	sem_post(&(session->Reserved11));
	if (session->SessionInterrupted == 0)
	{
		IGD_LifeTime_Remove(((struct IGD_WebServer_StateModule*)session->Parent)->LifeTime, session);
	}

	if (OkToFree)
	{
		if (session->SessionInterrupted == 0)
		{
			IGD_WebClient_DestroyWebClientDataObject(session->Reserved3);
		}
		SESSION_TRACK(session,"** Destroyed **");
		sem_destroy(&(session->Reserved11));
		free(session);
	}
}
/*! \fn void IGD_WebServer_DisconnectSession(struct IGD_WebServer_Session *session)
\brief Terminates an IGD_WebServer_Session.
\par
<B>Note:</B> Normally this should never be called, as the session is automatically
managed by the system, such that it can take advantage of persistent connections and such.
\param session The IGD_WebServer_Session object to disconnect
*/
void IGD_WebServer_DisconnectSession(struct IGD_WebServer_Session *session)
{
	IGD_WebClient_Disconnect(session->Reserved3);
}
/*! \fn void IGD_WebServer_Pause(struct IGD_WebServer_Session *session)
\brief Pauses the IGD_WebServer_Session, such that reading of more data off the network is
temporarily suspended.
\par
<B>Note:</B> This method <B>MUST</B> only be called from either the \a IGD_WebServer_Session_OnReceive or \a IGD_WebServer_VirtualDirectory handlers.
\param session The IGD_WebServer_Session object to pause.
*/
__inline void IGD_WebServer_Pause(struct IGD_WebServer_Session *session)
{
	IGD_WebClient_Pause(session->Reserved3);
}
/*! \fn void IGD_WebServer_Resume(struct IGD_WebServer_Session *session)
\brief Resumes a paused IGD_WebServer_Session object.
\param session The IGD_WebServer_Session object to resume.
*/
__inline void IGD_WebServer_Resume(struct IGD_WebServer_Session *session)
{
	IGD_WebClient_Resume(session->Reserved3);
}
/*! \fn void IGD_WebServer_OverrideReceiveHandler(struct IGD_WebServer_Session *session, IGD_WebServer_Session_OnReceive OnReceive)
\brief Overrides the Receive handler, so that the passed in handler will get called whenever data is received.
\param session The IGD_WebServer_Session to hijack.
\param OnReceive The handler to handle the received data
*/
__inline void IGD_WebServer_OverrideReceiveHandler(struct IGD_WebServer_Session *session, IGD_WebServer_Session_OnReceive OnReceive)
{
	session->OnReceive = OnReceive;
	session->Reserved13 = 1;
}

/*
void *IGD_WebServer_FakeConnect(void *module, void *cd)
{
struct IGD_WebServer_StateModule *wsm = (struct IGD_WebServer_StateModule*)module;
return IGD_AsyncServerSocket_FakeConnect(wsm->ServerSocket, cd);
}
*/

// Private method used to send a file to the web session asynchronously
void IGD_WebServer_StreamFileSendOK(struct IGD_WebServer_Session *sender)
{
	FILE* pfile;
	size_t len;
	int status = 0;

	pfile = (FILE*)sender->User3;
	while ((len = fread(IGD_ScratchPad, 1, sizeof(IGD_ScratchPad), pfile)) != 0)
	{
		status = IGD_WebServer_StreamBody(sender, IGD_ScratchPad, (int)len, IGD_AsyncSocket_MemoryOwnership_USER, 0);
		if (status != IGD_WebServer_ALL_DATA_SENT || len < sizeof(IGD_ScratchPad)) break;
	}

	if (len < sizeof(IGD_ScratchPad) || status < 0)
	{
		// Finished sending the file or got a send error, close the session
		IGD_WebServer_StreamBody(sender, NULL, 0, IGD_AsyncSocket_MemoryOwnership_STATIC, 1);
		fclose(pfile);
	}
}

// Streams a file to the web session asynchronously, closes the session when done.
// Caller must supply a valid file handle.
void IGD_WebServer_StreamFile(struct IGD_WebServer_Session *session, FILE* pfile)
{
	session->OnSendOK = IGD_WebServer_StreamFileSendOK;
	session->User3 = (void*)pfile;
	IGD_WebServer_StreamFileSendOK(session);
}

