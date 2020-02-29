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
#define MSG_NOSIGNAL 0
#elif defined(WINSOCK1)
#include <winsock.h>
#include <wininet.h>
#endif

#ifdef __APPLE__
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

#include "IGD_Parsers.h"
#include "IGD_AsyncSocket.h"

#ifndef MICROSTACK_NOTLS
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

//#ifndef WINSOCK2
//#define SOCKET unsigned int
//#endif

#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))

#if defined(WIN32)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif

#ifdef SEMAPHORE_TRACKING
#define SEM_TRACK(x) x
void AsyncSocket_TrackLock(const char* MethodName, int Occurance, void *data)
{
	char v[100];
	wchar_t wv[100];
	size_t l;

	snprintf(v, 100, "  LOCK[%s, %d] (%x)\r\n",MethodName,Occurance,data);
#ifdef WIN32
	mbstowcs_s(&l, wv, 100, v, 100);
	OutputDebugString(wv);
#else
	printf(v);
#endif
}
void AsyncSocket_TrackUnLock(const char* MethodName, int Occurance, void *data)
{
	char v[100];
	wchar_t wv[100];
	size_t l;

	snprintf(v, 100, "UNLOCK[%s, %d] (%x)\r\n",MethodName,Occurance,data);
#ifdef WIN32
	mbstowcs_s(&l, wv, 100, v, 100);
	OutputDebugString(wv);
#else
	printf(v);
#endif
}
#else
#define SEM_TRACK(x)
#endif

struct IGD_AsyncSocket_SendData
{
	char* buffer;
	int bufferSize;
	int bytesSent;

	struct sockaddr_in6 remoteAddress;

	int UserFree;
	struct IGD_AsyncSocket_SendData *Next;
};

struct IGD_AsyncSocketModule
{
	void (*PreSelect)(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);
	void *Chain;

	unsigned int PendingBytesToSend;
	unsigned int TotalBytesSent;

#if defined(_WIN32_WCE) || defined(WIN32)
	SOCKET internalSocket;
#elif defined(_POSIX)
	int internalSocket;
#endif

	// The IPv4/IPv6 compliant address of the remote endpoint. We are not going to be using IPv6 all the time,
	// but we use the IPv6 structure to allocate the meximum space we need.
	struct sockaddr_in6 RemoteAddress;

	// Local interface of a given socket. This module will bind to any interface, but the actual interface used
	// is stored here.
	struct sockaddr_in6 LocalAddress;

	// Source address. Here is stored the actual source of a packet, usualy used with UDP where the source
	// of the traffic changes.
	struct sockaddr_in6 SourceAddress;

#ifdef MICROSTACK_PROXY
	// The address and port of a HTTPS proxy
	struct sockaddr_in6 ProxyAddress;
	int ProxyState;
	char* ProxyUser;
	char* ProxyPass;
#endif

	IGD_AsyncSocket_OnData OnData;
	IGD_AsyncSocket_OnConnect OnConnect;
	IGD_AsyncSocket_OnDisconnect OnDisconnect;
	IGD_AsyncSocket_OnSendOK OnSendOK;
	IGD_AsyncSocket_OnInterrupt OnInterrupt;

	IGD_AsyncSocket_OnBufferSizeExceeded OnBufferSizeExceeded;
	IGD_AsyncSocket_OnBufferReAllocated OnBufferReAllocated;

	void *LifeTime;
	void *user;
	void *user2;
	int user3;
	int PAUSE;
	int FinConnect;
	int BeginPointer;
	int EndPointer;
	char* buffer;
	int MallocSize;
	int InitialSize;

	struct IGD_AsyncSocket_SendData *PendingSend_Head;
	struct IGD_AsyncSocket_SendData *PendingSend_Tail;
	sem_t SendLock;

	int MaxBufferSize;
	int MaxBufferSizeExceeded;
	void *MaxBufferSizeUserObject;

	// Added for TLS support
	#ifndef MICROSTACK_NOTLS
	int SSLConnect;
	int SSLForceRead;
	SSL* ssl;
	int  sslstate;
	BIO* sslbio;
	SSL_CTX *ssl_ctx;
	#endif
};

void IGD_AsyncSocket_PostSelect(void* object,int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
void IGD_AsyncSocket_PreSelect(void* object,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);

//
// An internal method called by Chain as Destroy, to cleanup AsyncSocket
//
// <param name="socketModule">The AsyncSocketModule</param>
void IGD_AsyncSocket_Destroy(void *socketModule)
{
	struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;
	struct IGD_AsyncSocket_SendData *temp, *current;

	// Call the interrupt event if necessary
	if (!IGD_AsyncSocket_IsFree(module))
	{
		if (module->OnInterrupt != NULL) module->OnInterrupt(module, module->user);
	}

	#ifndef MICROSTACK_NOTLS
	// If this is an SSL socket, free the SSL state
	if (module->ssl != NULL)
	{
		SSL_free(module->ssl); // Frees SSL session and BIO buffer at the same time
		module->ssl = NULL;
		module->sslstate = 0;
		module->sslbio = NULL;
	}
	#endif

	// Close socket if necessary
	if (module->internalSocket != ~0)
	{
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
		shutdown(module->internalSocket, SD_BOTH);
#endif
		closesocket(module->internalSocket);
#elif defined(_POSIX)
		shutdown(module->internalSocket,SHUT_RDWR);
		close(module->internalSocket);
#endif
		module->internalSocket = (SOCKET)~0;
	}

	// Free the buffer if necessary
	if (module->buffer != NULL)
	{
		if (module->buffer != IGD_ScratchPad2) free(module->buffer);
		module->buffer = NULL;
		module->MallocSize = 0;
	}

	// Clear all the data that is pending to be sent
	temp = current = module->PendingSend_Head;
	while (current != NULL)
	{
		temp = current->Next;
		if (current->UserFree == 0) free(current->buffer);
		free(current);
		current = temp;
	}

	module->FinConnect = 0;
	module->user = NULL;
	#ifndef MICROSTACK_NOTLS
	module->SSLConnect = 0;
	module->sslstate = 0;
	#endif
	sem_destroy(&(module->SendLock));
}
/*! \fn IGD_AsyncSocket_SetReAllocateNotificationCallback(IGD_AsyncSocket_SocketModule AsyncSocketToken, IGD_AsyncSocket_OnBufferReAllocated Callback)
\brief Set the callback handler for when the internal data buffer has been resized
\param AsyncSocketToken The specific connection to set the callback with
\param Callback The callback handler to set
*/
void IGD_AsyncSocket_SetReAllocateNotificationCallback(IGD_AsyncSocket_SocketModule AsyncSocketToken, IGD_AsyncSocket_OnBufferReAllocated Callback)
{
	if (AsyncSocketToken != NULL) { ((struct IGD_AsyncSocketModule*)AsyncSocketToken)->OnBufferReAllocated = Callback; }
}

/*! \fn IGD_CreateAsyncSocketModule(void *Chain, int initialBufferSize, IGD_AsyncSocket_OnData OnData, IGD_AsyncSocket_OnConnect OnConnect, IGD_AsyncSocket_OnDisconnect OnDisconnect,IGD_AsyncSocket_OnSendOK OnSendOK)
\brief Creates a new AsyncSocketModule
\param Chain The chain to add this module to. (Chain must <B>not</B> be running)
\param initialBufferSize The initial size of the receive buffer
\param OnData Function Pointer that triggers when Data is received
\param OnConnect Function Pointer that triggers upon successfull connection establishment
\param OnDisconnect Function Pointer that triggers upon disconnect
\param OnSendOK Function Pointer that triggers when pending sends are complete
\returns An IGD_AsyncSocket token
*/
IGD_AsyncSocket_SocketModule IGD_CreateAsyncSocketModule(void *Chain, int initialBufferSize, IGD_AsyncSocket_OnData OnData, IGD_AsyncSocket_OnConnect OnConnect, IGD_AsyncSocket_OnDisconnect OnDisconnect, IGD_AsyncSocket_OnSendOK OnSendOK)
{
	struct IGD_AsyncSocketModule *RetVal = (struct IGD_AsyncSocketModule*)malloc(sizeof(struct IGD_AsyncSocketModule));
	if (RetVal == NULL) return NULL;
	memset(RetVal, 0, sizeof(struct IGD_AsyncSocketModule));
	if (initialBufferSize != 0)
	{
		// Use a new buffer
		if ((RetVal->buffer = (char*)malloc(initialBufferSize)) == NULL) ILIBCRITICALEXIT(254);
	}
	else
	{
		// Use a static buffer, often used for UDP.
		initialBufferSize = sizeof(IGD_ScratchPad2);
		RetVal->buffer = IGD_ScratchPad2;
	}
	RetVal->PreSelect = &IGD_AsyncSocket_PreSelect;
	RetVal->PostSelect = &IGD_AsyncSocket_PostSelect;
	RetVal->Destroy = &IGD_AsyncSocket_Destroy;
	RetVal->internalSocket = (SOCKET)~0;
	RetVal->OnData = OnData;
	RetVal->OnConnect = OnConnect;
	RetVal->OnDisconnect = OnDisconnect;
	RetVal->OnSendOK = OnSendOK;
	RetVal->InitialSize = initialBufferSize;
	RetVal->MallocSize = initialBufferSize;
	RetVal->LifeTime = IGD_GetBaseTimer(Chain); //IGD_CreateLifeTime(Chain);

	sem_init(&(RetVal->SendLock), 0, 1);

	RetVal->Chain = Chain;
	IGD_AddToChain(Chain, RetVal);

	return((void*)RetVal);
}

/*! \fn IGD_AsyncSocket_ClearPendingSend(IGD_AsyncSocket_SocketModule socketModule)
\brief Clears all the pending data to be sent for an AsyncSocket
\param socketModule The IGD_AsyncSocket to clear
*/
void IGD_AsyncSocket_ClearPendingSend(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	struct IGD_AsyncSocket_SendData *data, *temp;

	data = module->PendingSend_Head;
	module->PendingSend_Tail = NULL;
	while (data != NULL)
	{
		temp = data->Next;
		// We only need to free this if we have ownership of this memory
		if (data->UserFree == 0) free(data->buffer);
		free(data);
		data = temp;
	}
	module->PendingSend_Head = NULL;
	module->PendingBytesToSend = 0;
}

/*! \fn IGD_AsyncSocket_SendTo(IGD_AsyncSocket_SocketModule socketModule, char* buffer, int length, int remoteAddress, unsigned short remotePort, enum IGD_AsyncSocket_MemoryOwnership UserFree)
\brief Sends data on an AsyncSocket module to a specific destination. (Valid only for <B>UDP</B>)
\param socketModule The IGD_AsyncSocket module to send data on
\param buffer The buffer to send
\param length The length of the buffer to send
\param remoteAddress The IPAddress of the destination 
\param remotePort The Port number of the destination
\param UserFree Flag indicating memory ownership. 
\returns \a IGD_AsyncSocket_SendStatus indicating the send status
*/
enum IGD_AsyncSocket_SendStatus IGD_AsyncSocket_SendTo(IGD_AsyncSocket_SocketModule socketModule, char* buffer, int length, struct sockaddr *remoteAddress, enum IGD_AsyncSocket_MemoryOwnership UserFree)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	struct IGD_AsyncSocket_SendData *data;
	int unblock = 0;
	int bytesSent = 0;

	// If the socket is empty, return now.
	if (socketModule == NULL) return(IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR);

	// Setup a new send data structure
	if ((data = (struct IGD_AsyncSocket_SendData*)malloc(sizeof(struct IGD_AsyncSocket_SendData))) == NULL) ILIBCRITICALEXIT(254);
	memset(data, 0, sizeof(struct IGD_AsyncSocket_SendData));
	data->buffer = buffer;
	data->bufferSize = length;
	data->bytesSent = 0;
	data->UserFree = UserFree;
	data->Next = NULL;

	// Copy the address to the send data structure
	memset(&(data->remoteAddress), 0, sizeof(struct sockaddr_in6));
	if (remoteAddress != NULL) memcpy(&(data->remoteAddress), remoteAddress, INET_SOCKADDR_LENGTH(remoteAddress->sa_family));

	SEM_TRACK(AsyncSocket_TrackLock("IGD_AsyncSocket_Send", 1, module);)
	sem_wait(&(module->SendLock));

	if (module->internalSocket == ~0)
	{
		// Too Bad, the socket closed
		if (UserFree == 0) {free(buffer);}
		free(data);
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_Send", 2, module);)
		sem_post(&(module->SendLock));
		return IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;
	}

	module->PendingBytesToSend += length;
	if (module->PendingSend_Tail != NULL || module->FinConnect == 0)
	{
		// There are still bytes that are pending to be sent, or pending connection, so we need to queue this up
		if (module->PendingSend_Tail == NULL)
		{
			module->PendingSend_Tail = data;
			module->PendingSend_Head = data;
		}
		else
		{
			module->PendingSend_Tail->Next = data;
			module->PendingSend_Tail = data;
		}
		unblock = 1;
		if (UserFree == IGD_AsyncSocket_MemoryOwnership_USER)
		{
			// If we don't own this memory, we need to copy the buffer,
			// because the user may free this memory before we have a chance to send it
			if ((data->buffer = (char*)malloc(data->bufferSize)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(data->buffer, buffer, length);
			MEMCHECK(assert(length <= data->bufferSize);)
			data->UserFree = IGD_AsyncSocket_MemoryOwnership_CHAIN;
		}
	}
	else
	{
		// There is no data pending to be sent, so lets go ahead and try to send this data
		module->PendingSend_Tail = data;
		module->PendingSend_Head = data;

		#ifndef MICROSTACK_NOTLS
		if (module->ssl != NULL || remoteAddress == NULL)
		{
			if (module->ssl == NULL)
			{
				// Send on non-SSL socket, set MSG_NOSIGNAL since we don't want to get Broken Pipe signals in Linux, ignored if Windows.
				bytesSent = send(module->internalSocket, module->PendingSend_Head->buffer + module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent, MSG_NOSIGNAL);
			}
			else
			{
				// Send on SSL socket, set MSG_NOSIGNAL since we don't want to get Broken Pipe signals in Linux, ignored if Windows.
				bytesSent = SSL_write(module->ssl, module->PendingSend_Head->buffer + module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent);
			}
		}
		else
		{
			bytesSent = sendto(module->internalSocket, module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent, MSG_NOSIGNAL, (struct sockaddr*)remoteAddress, INET_SOCKADDR_LENGTH(remoteAddress->sa_family));
		}
		#else
		if (remoteAddress == NULL)
		{
			// Set MSG_NOSIGNAL since we don't want to get Broken Pipe signals in Linux, ignored if Windows.
			bytesSent = send(module->internalSocket, module->PendingSend_Head->buffer + module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent, MSG_NOSIGNAL);
		}
		else
		{
			bytesSent = sendto(module->internalSocket, module->PendingSend_Head->buffer+module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize-module->PendingSend_Head->bytesSent, MSG_NOSIGNAL, (struct sockaddr*)remoteAddress, INET_SOCKADDR_LENGTH(remoteAddress->sa_family));
		}
		#endif

		if (bytesSent > 0)
		{
			// We were able to send something, so lets increment the counters
			module->PendingSend_Head->bytesSent += bytesSent;
			module->PendingBytesToSend -= bytesSent;
			module->TotalBytesSent += bytesSent;
		}

		#ifndef MICROSTACK_NOTLS
		if (bytesSent == -1 && module->ssl != NULL)
		{
			// OpenSSL returned an error
			bytesSent = SSL_get_error(module->ssl, bytesSent);
#ifdef WIN32
			if (bytesSent != SSL_ERROR_WANT_WRITE && bytesSent != SSL_ERROR_SSL && !(bytesSent == SSL_ERROR_SYSCALL && WSAGetLastError() == WSAEWOULDBLOCK)) 
#else
			if (bytesSent != SSL_ERROR_WANT_WRITE && bytesSent != SSL_ERROR_SSL) // "bytesSent != SSL_ERROR_SSL" portion is weird, but if not present, flowcontrol fails.
#endif
			{
				//ILIBMESSAGE2("SSL WRITE ERROR1", bytesSent);	// DEBUG
				//ILIBMESSAGE2("SSL WRITE ERROR2", WSAGetLastError());

				// Most likely the socket closed while we tried to send
				if (UserFree == 0) { free(buffer); }
				module->PendingSend_Head = module->PendingSend_Tail = NULL;
				free(data);
				SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_Send", 3, module);)
				sem_post(&(module->SendLock));

				// Ensure Calling On_Disconnect with MicroStackThread
				IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);

				return IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;
			}
		}
		#endif

		#ifndef MICROSTACK_NOTLS
		if (bytesSent == -1 && module->ssl == NULL)
		#else
		if (bytesSent == -1)
		#endif
		{
			// Send returned an error, so lets figure out what it was, as it could be normal
#if defined(_WIN32_WCE) || defined(WIN32)
			bytesSent = WSAGetLastError();
			if (bytesSent != WSAEWOULDBLOCK)
#elif defined(_POSIX)
			if (errno != EWOULDBLOCK)
#endif
			{
				// Most likely the socket closed while we tried to send
				if (UserFree == 0) {free(buffer);}
				module->PendingSend_Head = module->PendingSend_Tail = NULL;
				free(data);
				SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_Send", 3, module);)
				sem_post(&(module->SendLock));

				// Ensure Calling On_Disconnect with MicroStackThread
				IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);

				return IGD_AsyncSocket_SEND_ON_CLOSED_SOCKET_ERROR;
			}
		}
		if (module->PendingSend_Head->bytesSent == module->PendingSend_Head->bufferSize)
		{
			// All of the data has been sent
			if (UserFree == 0) {free(module->PendingSend_Head->buffer);}
			module->PendingSend_Tail = NULL;
			free(module->PendingSend_Head);
			module->PendingSend_Head = NULL;
		}
		else
		{
			// All of the data wasn't sent, so we need to copy the buffer
			// if we don't own the memory, because the user may free the
			// memory, before we have a chance to complete sending it.
			if (UserFree == IGD_AsyncSocket_MemoryOwnership_USER)
			{
				if ((data->buffer = (char*)malloc(data->bufferSize)) == NULL) ILIBCRITICALEXIT(254);
				memcpy(data->buffer,buffer,length);
				MEMCHECK(assert(length <= data->bufferSize);)
				data->UserFree = IGD_AsyncSocket_MemoryOwnership_CHAIN;
			}
			unblock = 1;
		}

	}
	SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_Send", 4, module);)
	sem_post(&(module->SendLock));
	if (unblock != 0) IGD_ForceUnBlockChain(module->Chain);
	return (enum IGD_AsyncSocket_SendStatus)unblock;
}

/*! \fn IGD_AsyncSocket_Disconnect(IGD_AsyncSocket_SocketModule socketModule)
\brief Disconnects an IGD_AsyncSocket
\param socketModule The IGD_AsyncSocket to disconnect
*/
void IGD_AsyncSocket_Disconnect(IGD_AsyncSocket_SocketModule socketModule)
{
#if defined(_WIN32_WCE) || defined(WIN32)
	SOCKET s;
#else
	int s;
#endif
	#ifndef MICROSTACK_NOTLS
	SSL *wasssl;
	#endif

	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;

	SEM_TRACK(AsyncSocket_TrackLock("IGD_AsyncSocket_Disconnect", 1, module);)
	sem_wait(&(module->SendLock));

	#ifndef MICROSTACK_NOTLS
	wasssl = module->ssl;
	if (module->ssl != NULL)
	{
		sem_post(&(module->SendLock));
		SSL_free(module->ssl); // Frees SSL session and both BIO buffers at the same time
		sem_wait(&(module->SendLock));
		module->ssl = NULL;
		module->sslstate = 0;
		module->sslbio = NULL;
	}
	#endif

	if (module->internalSocket != ~0)
	{
		// There is an associated socket that is still valid, so we need to close it
		module->PAUSE = 1;
		s = module->internalSocket;
		module->internalSocket = (SOCKET)~0;
		if (s != -1)
		{
#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
			shutdown(s, SD_BOTH);
#endif
			closesocket(s);
#elif defined(_POSIX)
			shutdown(s, SHUT_RDWR);
			close(s);
#endif
		}

		// Since the socket is closing, we need to clear the data that is pending to be sent
		IGD_AsyncSocket_ClearPendingSend(socketModule);
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_Disconnect", 2, module);)
		sem_post(&(module->SendLock));

		#ifndef MICROSTACK_NOTLS
		if (wasssl == NULL)
		{
		#endif
			// This was a normal socket, fire the event notifying the user. Depending on connection state, we event differently
			if (module->FinConnect <= 0 && module->OnConnect != NULL) { module->OnConnect(module, 0, module->user); } // Connection Failed
			if (module->FinConnect > 0 && module->OnDisconnect != NULL) { module->OnDisconnect(module, module->user); } // Socket Disconnected
		#ifndef MICROSTACK_NOTLS
		}
		else
		{
			// This was a SSL socket, fire the event notifying the user. Depending on connection state, we event differently
			if (module->SSLConnect == 0 && module->OnConnect != NULL) { module->OnConnect(module, 0, module->user); } // Connection Failed
			if (module->SSLConnect != 0 && module->OnDisconnect != NULL) { module->OnDisconnect(module, module->user); } // Socket Disconnected
		}
		#endif
		module->FinConnect = 0;
		module->user = NULL;
		#ifndef MICROSTACK_NOTLS
		module->SSLConnect = 0;
		module->sslstate = 0;
		#endif
	}
	else
	{
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_Disconnect", 3, module);)
		sem_post(&(module->SendLock));
	}
}

void IGD_ProcessAsyncSocket(struct IGD_AsyncSocketModule *Reader, int pendingRead);
void IGD_AsyncSocket_Callback(IGD_AsyncSocket_SocketModule socketModule, int connectDisconnectReadWrite)
{
	if (socketModule != NULL)
	{
		struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
		if (connectDisconnectReadWrite == 0) // Connected
		{
			memset(&(module->LocalAddress), 0, sizeof(struct sockaddr_in6));

			module->FinConnect = 1;
			module->PAUSE = 0;

			#ifndef MICROSTACK_NOTLS
			if (module->ssl != NULL)
			{
				// If SSL enabled, we need to complete the SSL handshake before we tell the application we are connected.
				SSL_connect(module->ssl);
			}
			else
			{
			#endif
				// No SSL, tell application we are connected.
				module->OnConnect(module, -1, module->user);			
			#ifndef MICROSTACK_NOTLS
			}
			#endif
		}
		else if (connectDisconnectReadWrite == 1) // Disconnected
			IGD_AsyncSocket_Disconnect(module);
		else if (connectDisconnectReadWrite == 2) // Data read
			IGD_ProcessAsyncSocket(module, 1);
	}
}


/*! \fn IGD_AsyncSocket_ConnectTo(IGD_AsyncSocket_SocketModule socketModule, int localInterface, int remoteInterface, int remotePortNumber, IGD_AsyncSocket_OnInterrupt InterruptPtr,void *user)
\brief Attempts to establish a TCP connection
\param socketModule The IGD_AsyncSocket to initiate the connection
\param localInterface The interface to use to establish the connection
\param remoteInterface The remote interface to connect to
\param remotePortNumber The remote port to connect to
\param InterruptPtr Function Pointer that triggers if connection attempt is interrupted
\param user User object that will be passed to the \a OnConnect method
*/
void IGD_AsyncSocket_ConnectTo(void* socketModule, struct sockaddr *localInterface, struct sockaddr *remoteAddress, IGD_AsyncSocket_OnInterrupt InterruptPtr, void *user)
{
	int flags = 1;
	char *tmp;
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	struct sockaddr_in6 any;

	// If there is something going on and we try to connect using this socket, fail! This is not supposed to happen.
	if (module->internalSocket != -1)
	{
		PRINTERROR(); ILIBCRITICALEXIT2(253, (int)(module->internalSocket));
	}

	// Clean up
	memset(&(module->RemoteAddress), 0, sizeof(struct sockaddr_in6));
	memset(&(module->LocalAddress) , 0, sizeof(struct sockaddr_in6));
	memset(&(module->SourceAddress), 0, sizeof(struct sockaddr_in6));

	// Setup
	memcpy(&(module->RemoteAddress), remoteAddress, INET_SOCKADDR_LENGTH(remoteAddress->sa_family));
	module->PendingBytesToSend = 0;
	module->TotalBytesSent = 0;
	module->PAUSE = 0;
	module->user = user;
	module->OnInterrupt = InterruptPtr;
	if ((tmp = (char*)realloc(module->buffer, module->InitialSize)) == NULL) ILIBCRITICALEXIT(254);
	module->buffer = tmp;
	module->MallocSize = module->InitialSize;

	// If localInterface is NULL, we will assume INADDRANY - IPv4/IPv6 based on remote address
	if (localInterface == NULL)
	{
		memset(&any, 0, sizeof(struct sockaddr_in6));
		#ifdef MICROSTACK_PROXY
		if (module->ProxyAddress.sin6_family == 0) any.sin6_family = remoteAddress->sa_family; else any.sin6_family = module->ProxyAddress.sin6_family;
		#else
		any.sin6_family = remoteAddress->sa_family;
		#endif
		localInterface = (struct sockaddr*)&any;
	}

	// The local port should always be zero
#ifdef _DEBUG
	if (localInterface->sa_family == AF_INET && ((struct sockaddr_in*)localInterface)->sin_port != 0) { PRINTERROR(); ILIBCRITICALEXIT(253); }
	if (localInterface->sa_family == AF_INET6 && ((struct sockaddr_in*)localInterface)->sin_port != 0) { PRINTERROR(); ILIBCRITICALEXIT(253); }
#endif

	// Allocate a new socket
	if ((module->internalSocket = IGD_GetSocket(localInterface, SOCK_STREAM, IPPROTO_TCP)) == 0) { PRINTERROR(); ILIBCRITICALEXIT(253); }

	// Initialise the buffer pointers, since no data is in them yet.
	module->FinConnect = 0;
	#ifndef MICROSTACK_NOTLS
	module->SSLConnect = 0;
	module->sslstate = 0;
	#endif
	module->BeginPointer = 0;
	module->EndPointer = 0;

	// Set the socket to non-blocking mode, because we need to play nice and share the MicroStack thread
#if defined(_WIN32_WCE) || defined(WIN32)
	ioctlsocket(module->internalSocket, FIONBIO, (u_long *)(&flags));
#elif defined(_POSIX)
	flags = fcntl(module->internalSocket, F_GETFL,0);
	fcntl(module->internalSocket, F_SETFL, O_NONBLOCK | flags);
#endif

	// Turn on keep-alives for the socket
	if (setsockopt(module->internalSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&flags, sizeof(flags)) != 0) ILIBCRITICALERREXIT(253);

	// Connect the socket, and force the chain to unblock, since the select statement doesn't have us in the fdset yet.
#ifdef MICROSTACK_PROXY
	if (module->ProxyAddress.sin6_family != 0)
	{
		if (connect(module->internalSocket, (struct sockaddr*)&(module->ProxyAddress), INET_SOCKADDR_LENGTH(module->ProxyAddress.sin6_family)) != -1)
		{
			// Connect failed. Set a short time and call disconnect.
			module->FinConnect = -1;
			IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);
			return;
		}
	}
	else
#endif
	if (connect(module->internalSocket, (struct sockaddr*)remoteAddress, INET_SOCKADDR_LENGTH(remoteAddress->sa_family)) != -1)
	{
		// Connect failed. Set a short time and call disconnect.
		module->FinConnect = -1;
		IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);
		return;
	}

#ifdef _DEBUG
	#ifdef _POSIX
		if (errno != EINPROGRESS) // The result of the connect should always be "WOULD BLOCK" on Linux. But sometimes this fails.
		{
			// This happens when the interface is no longer available. Disconnect socket.
			module->FinConnect = -1;
			IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);
			return;
		}
	#endif
	#ifdef WIN32
		{
			if (GetLastError() != WSAEWOULDBLOCK) // The result of the connect should always be "WOULD BLOCK" on Windows.
			{
				// This happens when the interface is no longer available. Disconnect socket.
				module->FinConnect = -1;
				IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);
				return;
			}
		}
	#endif
#endif

	IGD_ForceUnBlockChain(module->Chain);
}

#ifdef MICROSTACK_PROXY
// Connect to a remote access using an HTTPS proxy. If "proxyAddress" is set to NULL, this call acts just to a normal connect call without a proxy.
void IGD_AsyncSocket_ConnectToProxy(void* socketModule, struct sockaddr *localInterface, struct sockaddr *remoteAddress, struct sockaddr *proxyAddress, char* proxyUser, char* proxyPass, IGD_AsyncSocket_OnInterrupt InterruptPtr, void *user)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	memset(&(module->ProxyAddress), 0, sizeof(struct sockaddr_in6));
	module->ProxyState = 0;
	module->ProxyUser = proxyUser; // Proxy user & password are kept by reference!!!
	module->ProxyPass = proxyPass;

	if (proxyAddress != NULL) memcpy(&(module->ProxyAddress), proxyAddress, INET_SOCKADDR_LENGTH(proxyAddress->sa_family));
	IGD_AsyncSocket_ConnectTo(socketModule, localInterface, remoteAddress, InterruptPtr, user);
}
#endif

//
// Internal method called when data is ready to be processed on an IGD_AsyncSocket
//
// <param name="Reader">The IGD_AsyncSocket with pending data</param>
void IGD_ProcessAsyncSocket(struct IGD_AsyncSocketModule *Reader, int pendingRead)
{
	#ifndef MICROSTACK_NOTLS
	int ssllen;
	int sslstate;
	int sslerror;
	SSL *wasssl;
	#endif
	int iBeginPointer = 0;
	int iEndPointer = 0;
	int iPointer = 0;
	int bytesReceived = 0;
	int len;
	char *temp;

	//
	// If the thing isn't paused, and the user set the pointers such that we still have data
	// in our buffers, we need to call the user back with that data, before we attempt to read
	// more data off the network
	//
	if (!pendingRead)
	{
		if (Reader->internalSocket != ~0 && Reader->PAUSE <= 0 && Reader->BeginPointer != Reader->EndPointer)
		{
			iBeginPointer = Reader->BeginPointer;
			iEndPointer = Reader->EndPointer;
			iPointer = 0;

			while (Reader->internalSocket != ~0 && Reader->PAUSE <= 0 && Reader->BeginPointer != Reader->EndPointer && Reader->EndPointer != 0)
			{
				Reader->EndPointer = Reader->EndPointer-Reader->BeginPointer;
				Reader->BeginPointer = 0;
				if (Reader->OnData != NULL)
				{
					Reader->OnData(Reader, Reader->buffer + iBeginPointer, &(iPointer), Reader->EndPointer, &(Reader->OnInterrupt), &(Reader->user), &(Reader->PAUSE));
				}
				iBeginPointer += iPointer;
				Reader->EndPointer -= iPointer;
				if (iPointer == 0) break;
				iPointer = 0;
			}
			Reader->BeginPointer = iBeginPointer;
			Reader->EndPointer = iEndPointer;
		}
	}

	// Reading Body Only
	if (Reader->BeginPointer == Reader->EndPointer)
	{
		Reader->BeginPointer = 0;
		Reader->EndPointer = 0;
	}
	if (!pendingRead || Reader->PAUSE > 0) return;

	//
	// If we need to grow the buffer, do it now
	//
	if (bytesReceived > (Reader->MallocSize - Reader->EndPointer) || 1024 > (Reader->MallocSize - Reader->EndPointer))// the 1st portion is for ssl & cd
	{
		//
		// This memory reallocation sometimes causes Insure++
		// to incorrectly report a READ_DANGLING (usually in 
		// a call to IGD_WebServer_StreamHeader_Raw.)
		// 
		// We verified that the problem is with Insure++ by
		// noting the value of 'temp' (0x008fa8e8), 
		// 'Reader->buffer' (0x00c55e80), and
		// 'MEMORYCHUNKSIZE' (0x00001800).
		//
		// When Insure++ reported the error, it (incorrectly) 
		// claimed that a pointer to memory address 0x00c55ea4
		// was invalid, while (correctly) citing the old memory
		// (0x008fa8e8-0x008fb0e7) as freed memory.
		// Normally Insure++ reports that the invalid pointer 
		// is pointing to someplace in the deallocated block,
		// but that wasn't the case.
		//
		if (Reader->MaxBufferSize == 0 || Reader->MallocSize < Reader->MaxBufferSize)
		{
			if (Reader->MaxBufferSize > 0 && (Reader->MaxBufferSize - Reader->MallocSize < MEMORYCHUNKSIZE))
			{
				Reader->MallocSize = Reader->MaxBufferSize;
			}
			else if (bytesReceived > 0)
			{
				Reader->MallocSize += bytesReceived - (Reader->MallocSize - Reader->EndPointer);
			}
			else
			{
				Reader->MallocSize += MEMORYCHUNKSIZE;
			}

			temp = Reader->buffer;
			if ((Reader->buffer = (char*)realloc(Reader->buffer, Reader->MallocSize)) == NULL) ILIBCRITICALEXIT(254);
			//
			// If this realloc moved the buffer somewhere, we need to inform people of it
			//
			if (Reader->buffer != temp && Reader->OnBufferReAllocated != NULL) Reader->OnBufferReAllocated(Reader, Reader->user, Reader->buffer - temp);
		}
		else
		{
			//
			// If we grow the buffer anymore, it will exceed the maximum allowed buffer size
			//
			Reader->MaxBufferSizeExceeded = 1;
			if (Reader->OnBufferSizeExceeded != NULL) Reader->OnBufferSizeExceeded(Reader, Reader->MaxBufferSizeUserObject);
			IGD_AsyncSocket_Disconnect(Reader);
			return;
		}
	}
	else if (Reader->BeginPointer != 0 && bytesReceived == 0)
	{
		//
		// We can save some cycles by moving the data back to the top
		// of the buffer, instead of just allocating more memory.
		//
		temp = Reader->buffer + Reader->BeginPointer;;
		memmove(Reader->buffer, temp, Reader->EndPointer-Reader->BeginPointer);
		Reader->EndPointer -= Reader->BeginPointer;
		Reader->BeginPointer = 0;

		//
		// Even though we didn't allocate new memory, we still moved data in the buffer, 
		// so we need to inform people of that, because it might be important
		//
		if (Reader->OnBufferReAllocated != NULL) Reader->OnBufferReAllocated(Reader, Reader->user, temp-Reader->buffer);
	}

	#ifndef MICROSTACK_NOTLS
	if (Reader->ssl != NULL)
	{
		// Read data off the SSL socket.

		// Now we will tell OpenSSL to process that data in the steam. This read may return nothing, but OpenSSL may
		// put data in the output buffer to be sent back out.
		bytesReceived = 0;
		do
		{
			// Read data from the SSL socket, this will read one SSL record at a time.
			ssllen = SSL_read(Reader->ssl, Reader->buffer+Reader->EndPointer + bytesReceived, Reader->MallocSize-Reader->EndPointer - bytesReceived);
			if (ssllen > 0) bytesReceived += ssllen;
		}
		while (ssllen > 0 && ((Reader->MallocSize-Reader->EndPointer - bytesReceived) > 0));

		// printf("SSL READ: LastLen = %d, Total = %d, State = %d, Error = %d\r\n", ssllen, bytesReceived, sslstate, sslerror);

		// We need to force more reading on the next chain loop
		if ((Reader->MallocSize-Reader->EndPointer - bytesReceived) == 0) Reader->SSLForceRead = 1;

		// Read the current SSL error. We do this only is no data was read, if we have any data lets process it.
		if (bytesReceived <= 0)
		{
			sslerror = SSL_get_error(Reader->ssl, ssllen);
			
			#ifdef WIN32
			if (!(sslerror == SSL_ERROR_WANT_READ || (sslerror == SSL_ERROR_SYSCALL && GetLastError() == 0)))
			{
				// There is no more data on the socket or error, shut it down.
				Reader->sslstate = 0;
				bytesReceived = -1;
				//ILIBMESSAGE2("IGD_ProcessAsyncSocket-SSLERROR1", sslerror);
			}
			#else
			if (!(sslerror == SSL_ERROR_WANT_READ || sslerror == SSL_ERROR_SYSCALL))
			{
				// There is no more data on the socket or error, shut it down.
				Reader->sslstate = 0;
				bytesReceived = -1;
				//ILIBMESSAGE2("IGD_ProcessAsyncSocket-SSLERROR1", sslerror);
			}
			#endif
		}

		sslstate = SSL_state(Reader->ssl);
		if (Reader->sslstate != 3 && sslstate == 3)
		{
			// If the SSL state changed to connected, we need to tell the application about the connection.
			Reader->sslstate = 3;
			if (Reader->SSLConnect == 0) // This is still a mistery, but if this check is not present, it's possible to signal connect more than once.
			{
				Reader->SSLConnect = 1;
				if (Reader->OnConnect != NULL) Reader->OnConnect(Reader, -1, Reader->user);
			}
		}
		if (bytesReceived == 0)
		{
			// We received no data, lets investigate why
			if (ssllen == 0 && bytesReceived == 0)
			{
				// There is no more data on the socket, shut it down.
				Reader->sslstate = 0;
				bytesReceived = -1;
			}
			else if (ssllen == -1 && sslstate == 0x2112)
			{
				ILIBMESSAGE("IGD_ProcessAsyncSocket-ReceivedNothingDisconnect-X1");

				// There is no more data on the socket, shut it down.
				Reader->sslstate = 0;
				bytesReceived = -1;
			}
			else return;
		}
	}
	else
	#endif
	{
		// Read data off the non-SSL, generic socket.
		// Set the receive address buffer size and read from the socket.
		len = sizeof(struct sockaddr_in6);
#if defined(WINSOCK2)
		bytesReceived = recvfrom(Reader->internalSocket, Reader->buffer+Reader->EndPointer, Reader->MallocSize-Reader->EndPointer, 0, (struct sockaddr*)&(Reader->SourceAddress), (int*)&len);
#else
		bytesReceived = recvfrom(Reader->internalSocket, Reader->buffer+Reader->EndPointer, Reader->MallocSize-Reader->EndPointer, 0, (struct sockaddr*)&(Reader->SourceAddress), (socklen_t*)&len);
#endif
	}

	sem_wait(&(Reader->SendLock));

	if (bytesReceived <= 0)
	{
		// If a UDP packet is larger than the buffer, drop it.
		#if defined(WINSOCK2)
		if (bytesReceived == SOCKET_ERROR && WSAGetLastError() == 10040) { sem_post(&(Reader->SendLock)); return; }
		#else
		// TODO: Linux errno
		//if (bytesReceived == -1 && errno != 0) printf("ERROR: errno = %d, %s\r\n", errno, strerror(errno));
		#endif

		//
		// This means the socket was gracefully closed by the remote endpoint
		//
		SEM_TRACK(AsyncSocket_TrackLock("IGD_ProcessAsyncSocket", 1, Reader);)
		IGD_AsyncSocket_ClearPendingSend(Reader);
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_ProcessAsyncSocket", 2, Reader);)

#if defined(_WIN32_WCE) || defined(WIN32)
#if defined(WINSOCK2)
		shutdown(Reader->internalSocket, SD_BOTH);
#endif
		closesocket(Reader->internalSocket);
#elif defined(_POSIX)
		shutdown(Reader->internalSocket,SHUT_RDWR);
		close(Reader->internalSocket);
#endif
		Reader->internalSocket = (SOCKET)~0;

		IGD_AsyncSocket_ClearPendingSend(Reader);
		
		#ifndef MICROSTACK_NOTLS
		wasssl = Reader->ssl;
		if (Reader->ssl != NULL)
		{
			sem_post(&(Reader->SendLock));
			SSL_free(Reader->ssl); // Frees SSL session and BIO buffer at the same time
			sem_wait(&(Reader->SendLock));
			Reader->ssl = NULL;
			Reader->sslstate = 0;
			Reader->sslbio = NULL;
		}
		#endif
		sem_post(&(Reader->SendLock));

		//
		// Inform the user the socket has closed
		//
		#ifndef MICROSTACK_NOTLS
		if (wasssl != NULL)
		{
			// This was a SSL socket, fire the event notifying the user. Depending on connection state, we event differently
			if (Reader->SSLConnect == 0 && Reader->OnConnect != NULL) { Reader->OnConnect(Reader, 0, Reader->user); } // Connection Failed
			if (Reader->SSLConnect != 0 && Reader->OnDisconnect != NULL) { Reader->OnDisconnect(Reader, Reader->user); } // Socket Disconnected
		}
		else
		{
		#endif
			// This was a normal socket, fire the event notifying the user. Depending on connection state, we event differently
			if (Reader->FinConnect <= 0 && Reader->OnConnect != NULL) { Reader->OnConnect(Reader, 0, Reader->user); } // Connection Failed
			if (Reader->FinConnect > 0 && Reader->OnDisconnect != NULL) { Reader->OnDisconnect(Reader, Reader->user); } // Socket Disconnected
		#ifndef MICROSTACK_NOTLS
		}
		Reader->SSLConnect = 0;
		Reader->sslstate = 0;
		#endif
		Reader->FinConnect = 0;

		//
		// If we need to free the buffer, do so
		//
		if (Reader->buffer != NULL)
		{
			if (Reader->buffer != IGD_ScratchPad2) free(Reader->buffer);
			Reader->buffer = NULL;
			Reader->MallocSize = 0;
		}
	}
	else
	{
		sem_post(&(Reader->SendLock));

		//
		// Data was read, so increment our counters
		//
		Reader->EndPointer += bytesReceived;

		//
		// Tell the user we have some data
		//
		if (Reader->OnData != NULL)
		{
			iBeginPointer = Reader->BeginPointer;
			iPointer = 0;
			Reader->OnData(Reader, Reader->buffer + Reader->BeginPointer, &(iPointer), Reader->EndPointer - Reader->BeginPointer, &(Reader->OnInterrupt), &(Reader->user),&(Reader->PAUSE));
			Reader->BeginPointer += iPointer;
		}
		//
		// If the user set the pointers, and we still have data, call them back with the data
		//
		if (Reader->internalSocket != ~0 && Reader->PAUSE <= 0 && Reader->BeginPointer!=Reader->EndPointer && Reader->BeginPointer != 0)
		{
			iBeginPointer = Reader->BeginPointer;
			iEndPointer = Reader->EndPointer;
			iPointer = 0;

			while (Reader->internalSocket != ~0 && Reader->PAUSE <= 0 && Reader->BeginPointer != Reader->EndPointer && Reader->EndPointer != 0)
			{				
				Reader->EndPointer = Reader->EndPointer-Reader->BeginPointer;
				Reader->BeginPointer = 0;
				if (Reader->OnData != NULL)
				{
					Reader->OnData(Reader, Reader->buffer + iBeginPointer, &(iPointer), Reader->EndPointer, &(Reader->OnInterrupt), &(Reader->user), &(Reader->PAUSE));
				}
				iBeginPointer += iPointer;
				Reader->EndPointer -= iPointer;
				if (iPointer == 0) break;
				iPointer = 0;
			}
			Reader->BeginPointer = iBeginPointer;
			Reader->EndPointer = iEndPointer;
		}

		//
		// If the user consumed all of the buffer, we can recycle it
		//
		if (Reader->BeginPointer == Reader->EndPointer)
		{
			Reader->BeginPointer = 0;
			Reader->EndPointer = 0;
		}
	}
}

/*! \fn IGD_AsyncSocket_GetUser(IGD_AsyncSocket_SocketModule socketModule)
\brief Returns the user object
\param socketModule The IGD_AsyncSocket token to fetch the user object from
\returns The user object
*/
void *IGD_AsyncSocket_GetUser(IGD_AsyncSocket_SocketModule socketModule)
{
	return(socketModule == NULL?NULL:((struct IGD_AsyncSocketModule*)socketModule)->user);
}

void IGD_AsyncSocket_SetUser(IGD_AsyncSocket_SocketModule socketModule, void* user)
{
	if (socketModule == NULL) return;
	((struct IGD_AsyncSocketModule*)socketModule)->user = user;
}

void *IGD_AsyncSocket_GetUser2(IGD_AsyncSocket_SocketModule socketModule)
{
	return(socketModule == NULL?NULL:((struct IGD_AsyncSocketModule*)socketModule)->user2);
}

void IGD_AsyncSocket_SetUser2(IGD_AsyncSocket_SocketModule socketModule, void* user2)
{
	if (socketModule == NULL) return;
	((struct IGD_AsyncSocketModule*)socketModule)->user2 = user2;
}

int IGD_AsyncSocket_GetUser3(IGD_AsyncSocket_SocketModule socketModule)
{
	return(socketModule == NULL?-1:((struct IGD_AsyncSocketModule*)socketModule)->user3);
}

void IGD_AsyncSocket_SetUser3(IGD_AsyncSocket_SocketModule socketModule, int user3)
{
	if (socketModule == NULL) return;
	((struct IGD_AsyncSocketModule*)socketModule)->user3 = user3;
}

//
// Chained PreSelect handler for IGD_AsyncSocket
//
// <param name="readset"></param>
// <param name="writeset"></param>
// <param name="errorset"></param>
// <param name="blocktime"></param>
void IGD_AsyncSocket_PreSelect(void* socketModule,fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	if (module->internalSocket == -1) return; // If there is not internal socket, just return now.

	SEM_TRACK(AsyncSocket_TrackLock("IGD_AsyncSocket_PreSelect", 1, module);)
	sem_wait(&(module->SendLock));

	if (module->internalSocket != -1)
	{
	#ifndef MICROSTACK_NOTLS
		if (module->SSLForceRead) *blocktime = 0; // If the previous loop filled the read buffer, force more reading.
	#endif
		if (module->PAUSE < 0) *blocktime = 0;
		if (module->FinConnect == 0)
		{
			// Not Connected Yet
			#if defined(WIN32)
			#pragma warning( push, 3 ) // warning C4127: conditional expression is constant
			#endif
			FD_SET(module->internalSocket, writeset);
			FD_SET(module->internalSocket, errorset);
			#if defined(WIN32)
			#pragma warning( pop )
			#endif
		}
		else
		{
			if (module->PAUSE == 0) // Only if this is zero. <0 is resume, so we want to process first
			{
				// Already Connected, just needs reading
				#if defined(WIN32)
				#pragma warning( push, 3 ) // warning C4127: conditional expression is constant
				#endif
				FD_SET(module->internalSocket, readset);
				FD_SET(module->internalSocket, errorset);
				#if defined(WIN32)
				#pragma warning( pop )
				#endif
			}
		}

		if (module->PendingSend_Head != NULL)
		{
			// If there is pending data to be sent, then we need to check when the socket is writable
			#if defined(WIN32)
			#pragma warning( push, 3 ) // warning C4127: conditional expression is constant
			#endif
			FD_SET(module->internalSocket, writeset);
			#if defined(WIN32)
			#pragma warning( pop )
			#endif
		}
	}
	SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PreSelect", 2, module);)
	sem_post(&(module->SendLock));
}

void IGD_AsyncSocket_PrivateShutdown(void* socketModule)
{
	#ifndef MICROSTACK_NOTLS
	SSL *wasssl;
	#endif
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;

	// If this is an SSL socket, close down the SSL state
	#ifndef MICROSTACK_NOTLS
	if ((wasssl = module->ssl) != NULL)
	{
		SSL_free(module->ssl); // Frees SSL session and BIO buffer at the same time
		module->ssl = NULL;
		module->sslstate = 0;
		module->sslbio = NULL;
	}
	#endif

	// Now shutdown the socket and set it to zero
	#if defined(_WIN32_WCE) || defined(WIN32)
	#if defined(WINSOCK2)
		shutdown(module->internalSocket, SD_BOTH);
	#endif
		closesocket(module->internalSocket);
	#elif defined(_POSIX)
		shutdown(module->internalSocket, SHUT_RDWR);
		close(module->internalSocket);
	#endif
	module->internalSocket = (SOCKET)~0;
		
	#ifndef MICROSTACK_NOTLS
	if (wasssl != NULL)
	{
		// This was a SSL socket, fire the event notifying the user. Depending on connection state, we event differently
		if (module->SSLConnect == 0 && module->OnConnect != NULL) { module->OnConnect(module, 0, module->user); } // Connection Failed
		if (module->SSLConnect != 0 && module->OnDisconnect != NULL) { module->OnDisconnect(module, module->user); } // Socket Disconnected
	}
	else
	{
	#endif
		// This was a normal socket, fire the event notifying the user. Depending on connection state, we event differently
		if (module->FinConnect <= 0 && module->OnConnect != NULL) { module->OnConnect(module, 0, module->user); } // Connection Failed
		if (module->FinConnect > 0 && module->OnDisconnect != NULL) { module->OnDisconnect(module, module->user); } // Socket Disconnected
	#ifndef MICROSTACK_NOTLS
	}
	module->SSLConnect = 0;
	module->sslstate = 0;
	#endif
	module->FinConnect = 0;
}

//
// Chained PostSelect handler for IGD_AsyncSocket
//
// <param name="socketModule"></param>
// <param name="slct"></param>
// <param name="readset"></param>
// <param name="writeset"></param>
// <param name="errorset"></param>
void IGD_AsyncSocket_PostSelect(void* socketModule, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset)
{
	int TriggerSendOK = 0;
	struct IGD_AsyncSocket_SendData *temp;
	int bytesSent = 0;
	int flags, len;
	int TRY_TO_SEND = 1;
	int triggerReadSet = 0;
	int triggerResume = 0;
	int triggerWriteSet = 0;
	int serr = 0, serrlen = sizeof(serr);
	int fd_error, fd_read, fd_write;
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;

	// If there is no internal socket or no events, just return now.
	if (module->internalSocket == -1 || module->FinConnect == -1) return;
	fd_error = FD_ISSET(module->internalSocket, errorset);
	fd_read = FD_ISSET(module->internalSocket, readset);
	fd_write = FD_ISSET(module->internalSocket, writeset);

#ifndef MICROSTACK_NOTLS
	if (module->SSLForceRead) { fd_read = 1; module->SSLForceRead = 0; } // If the previous loop filled the read buffer, force more reading.
#endif
	
	UNREFERENCED_PARAMETER( slct );

	SEM_TRACK(AsyncSocket_TrackLock("IGD_AsyncSocket_PostSelect", 1, module);)
	sem_wait(&(module->SendLock)); // Lock!
	
	//if (fd_error != 0) printf("IGD_AsyncSocket_PostSelect-ERROR\r\n");
	//if (fd_read != 0) printf("IGD_AsyncSocket_PostSelect-READ\r\n");
	//if (fd_write != 0) printf("IGD_AsyncSocket_PostSelect-WRITE\r\n");

	//
	// Error Handling. If the ERROR flag is set we have a problem. If not, we must check the socket status for an error.
	// Yes, this is odd, but it's possible for a socket to report a read set and still have an error, in this past this
	// was not handled and caused a lot of problems.
	//
	if (fd_error != 0)
	{
		serr = 1;
	}
	else
	{
		// Fetch the socket error code
#if defined(WINSOCK2)
		getsockopt(module->internalSocket, SOL_SOCKET, SO_ERROR, (char*)&serr, (int*)&serrlen);
#else
		getsockopt(module->internalSocket, SOL_SOCKET, SO_ERROR, (char*)&serr, (socklen_t*)&serrlen);
#endif
	}

	#ifdef MICROSTACK_PROXY
	// Handle proxy, we need to read the proxy response, all of it and not a byte more.
	if (module->FinConnect == 1 && module->ProxyState == 1 && serr == 0 && fd_read != 0)
	{
		char *ptr1, *ptr2;
		int len2, len3;
		serr = 555; // Fake proxy error
		len2 = recv(module->internalSocket, IGD_ScratchPad2, 1024, MSG_PEEK);
		if (len2 > 0 && len2 < 1024)
		{
			IGD_ScratchPad2[len2] = 0;
			ptr1 = strstr(IGD_ScratchPad2, "\r\n\r\n");
			ptr2 = strstr(IGD_ScratchPad2, " 200 ");
			if (ptr1 != NULL && ptr2 != NULL && ptr2 < ptr1)
			{
				len3 = (int)((ptr1 + 4) - IGD_ScratchPad2);
				recv(module->internalSocket, IGD_ScratchPad2, len3, 0);
				module->FinConnect = 0; // Let pretend we never connected, this will trigger all the connection stuff.
				module->ProxyState = 2; // Move the proxy connection state forward.
				serr = 0;				// Proxy connected collectly.
			}
		}
	}
	#endif

	// If there are any errors, shutdown this socket
	if (serr != 0)
	{
		// Unlock before fireing the event
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 4, module);)
		sem_post(&(module->SendLock));
		IGD_AsyncSocket_PrivateShutdown(module);
	}
	else
	{
		// There are no errors, lets keep processing the socket normally
		if (module->FinConnect == 0)
		{
			// Check to see if the socket is connected
#ifdef MICROSTACK_PROXY
			if (fd_write != 0 || module->ProxyState == 2)
#else
			if (fd_write != 0)
#endif
			{
				// Connected
				len = sizeof(struct sockaddr_in6);
#if defined(WINSOCK2)
				getsockname(module->internalSocket, (struct sockaddr*)(&module->LocalAddress), (int*)&len);
#else
				getsockname(module->internalSocket, (struct sockaddr*)(&module->LocalAddress), (socklen_t*)&len);
#endif
				module->FinConnect = 1;
				module->PAUSE = 0;

				// Set the socket to non-blocking mode, so we can play nice and share the thread
				#if defined(_WIN32_WCE) || defined(WIN32)
				flags = 1;
				ioctlsocket(module->internalSocket, FIONBIO, (u_long *)(&flags));
				#elif defined(_POSIX)
				flags = fcntl(module->internalSocket, F_GETFL,0);
				fcntl(module->internalSocket, F_SETFL, O_NONBLOCK|flags);
				#endif

				// If this is a proxy connection, send the proxy connect header now.
#ifdef MICROSTACK_PROXY
				if (module->ProxyAddress.sin6_family != 0 && module->ProxyState == 0)
				{
					int len2, len3;
					IGD_Inet_ntop((int)(module->RemoteAddress.sin6_family), (void*)&(((struct sockaddr_in*)&(module->RemoteAddress))->sin_addr), IGD_ScratchPad, 4096);
					if (module->ProxyUser == NULL || module->ProxyPass == NULL)
					{
						len2 = snprintf(IGD_ScratchPad2, 4096, "CONNECT %s:%d HTTP/1.1\r\nProxy-Connection: keep-alive\r\nHost: %s\r\n\r\n", IGD_ScratchPad, ntohs(module->RemoteAddress.sin6_port), IGD_ScratchPad);
					} else {
						char* ProxyAuth = NULL;
						len2 = snprintf(IGD_ScratchPad2, 4096, "%s:%s", module->ProxyUser, module->ProxyPass);
						len2 = IGD_Base64Encode((unsigned char*)IGD_ScratchPad2, len2, (unsigned char**)&ProxyAuth);
						len2 = snprintf(IGD_ScratchPad2, 4096, "CONNECT %s:%d HTTP/1.1\r\nProxy-Connection: keep-alive\r\nHost: %s\r\nProxy-authorization: basic %s\r\n\r\n", IGD_ScratchPad, ntohs(module->RemoteAddress.sin6_port), IGD_ScratchPad, ProxyAuth);
						if (ProxyAuth != NULL) free(ProxyAuth);
					}
					len3 = send(module->internalSocket, IGD_ScratchPad2, len2, MSG_NOSIGNAL);
					module->ProxyState = 1;
					// TODO: Set timeout. If the proxy does not respond, we need to close this connection.
					// On the other hand... This is not generally a problem, proxies will disconnect after a timeout anyway.
					
					SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 4, module);)
					sem_post(&(module->SendLock));
					return;
				}
				if (module->ProxyState == 2) module->ProxyState = 3;
#endif

				// Connection Complete
				triggerWriteSet = 1;
			}

			// Unlock before fireing the event
			SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 4, module);)
			sem_post(&(module->SendLock));

			// If we did connect, we got more things to do
			if (triggerWriteSet != 0)
			{
				#ifndef MICROSTACK_NOTLS
				if (module->ssl_ctx != NULL)
				{
					// Make this call to setup the SSL stuff
					IGD_AsyncSocket_SetSSLContext(module, module->ssl_ctx, 0);

					// If this is an SSL socket, launch the SSL connection process
					if ((serr = SSL_connect(module->ssl)) != 1 && SSL_get_error(module->ssl, serr) != SSL_ERROR_WANT_READ)
					{
						IGD_AsyncSocket_PrivateShutdown(module); // On Linux it's possible to get BROKEN PIPE on SSL_connect()
					}
				}
				else
				#endif
				{
					// If this is a normal socket, event the connection now.
					if (module->OnConnect != NULL) module->OnConnect(module, -1, module->user);
				}
			}
		}
		else
		{
			// Connected socket, we need to read data
			if (fd_read != 0)
			{
				triggerReadSet = 1; // Data Available
			}
			else if (module->PAUSE < 0)
			{
				// Someone resumed a paused connection, but the FD_SET was not triggered because there is no new data on the socket.
				triggerResume = 1;
				++module->PAUSE;
			}

			// Unlock before fireing the event
			SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 4, module);)
			sem_post(&(module->SendLock));

			if (triggerReadSet != 0 || triggerResume != 0) IGD_ProcessAsyncSocket(module, triggerReadSet);
		}
	}
	SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 4, module);)


	SEM_TRACK(AsyncSocket_TrackLock("IGD_AsyncSocket_PostSelect", 1, module);)
	sem_wait(&(module->SendLock));

	// Write Handling
	if (module->FinConnect > 0 && module->internalSocket != ~0 && fd_write != 0 && module->PendingSend_Head != NULL)
	{
		//
		// Keep trying to send data, until we are told we can't
		//
		while (TRY_TO_SEND != 0)
		{
			if (module->PendingSend_Head == NULL) break;
			#ifndef MICROSTACK_NOTLS
			if (module->ssl != NULL)
			{
				// Send on SSL socket
				bytesSent = SSL_write(module->ssl, module->PendingSend_Head->buffer + module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize - module->PendingSend_Head->bytesSent);
			}
			else
			#endif
			if (module->PendingSend_Head->remoteAddress.sin6_family == 0)
			{
				bytesSent = send(module->internalSocket, module->PendingSend_Head->buffer + module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize - module->PendingSend_Head->bytesSent, MSG_NOSIGNAL);
			}
			else
			{
				bytesSent = sendto(module->internalSocket, module->PendingSend_Head->buffer + module->PendingSend_Head->bytesSent, module->PendingSend_Head->bufferSize - module->PendingSend_Head->bytesSent, MSG_NOSIGNAL, (struct sockaddr*)&module->PendingSend_Head->remoteAddress, INET_SOCKADDR_LENGTH(module->PendingSend_Head->remoteAddress.sin6_family));
			}

			if (bytesSent > 0)
			{
				module->PendingBytesToSend -= bytesSent;
				module->TotalBytesSent += bytesSent;
				module->PendingSend_Head->bytesSent += bytesSent;
				if (module->PendingSend_Head->bytesSent == module->PendingSend_Head->bufferSize)
				{
					// Finished Sending this block
					if (module->PendingSend_Head == module->PendingSend_Tail)
					{
						module->PendingSend_Tail = NULL;
					}
					if (module->PendingSend_Head->UserFree == 0)
					{
						free(module->PendingSend_Head->buffer);
					}
					temp = module->PendingSend_Head->Next;
					free(module->PendingSend_Head);
					module->PendingSend_Head = temp;
					if (module->PendingSend_Head == NULL) { TRY_TO_SEND = 0; }
				}
				else
				{
					//
					// We sent data, but not everything that needs to get sent was sent, try again
					//
					TRY_TO_SEND = 1;
				}
			}
			#ifndef MICROSTACK_NOTLS
			if (bytesSent == -1 && module->ssl == NULL)
			#else
			if (bytesSent == -1)
			#endif
			{
				// Error, clean up everything
				TRY_TO_SEND = 0;
#if defined(_WIN32_WCE) || defined(WIN32)
				if (WSAGetLastError() != WSAEWOULDBLOCK)
#elif defined(_POSIX)
				if (errno != EWOULDBLOCK)
#endif
				{
					//
					// There was an error sending
					//
					IGD_AsyncSocket_ClearPendingSend(socketModule);
					IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);
				}
			}
			#ifndef MICROSTACK_NOTLS
			else if (bytesSent == -1 && module->ssl != NULL)
			{
				// OpenSSL returned an error
				TRY_TO_SEND = 0;
				bytesSent = SSL_get_error(module->ssl, bytesSent);
				if (bytesSent != SSL_ERROR_WANT_WRITE)
				{
					//
					// There was an error sending
					//
					IGD_AsyncSocket_ClearPendingSend(socketModule);
					IGD_LifeTime_Add(module->LifeTime, socketModule, 0, &IGD_AsyncSocket_Disconnect, NULL);
				}
			}
			#endif
		}
		//
		// This triggers OnSendOK, if all the pending data has been sent.
		//
		if (module->PendingSend_Head == NULL && bytesSent != -1) { TriggerSendOK = 1; }
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 2, module);)
		sem_post(&(module->SendLock));
		if (TriggerSendOK != 0) module->OnSendOK(module, module->user);
	}
	else
	{
		SEM_TRACK(AsyncSocket_TrackUnLock("IGD_AsyncSocket_PostSelect", 2, module);)
		sem_post(&(module->SendLock));
	}
}

/*! \fn IGD_AsyncSocket_IsFree(IGD_AsyncSocket_SocketModule socketModule)
\brief Determines if an IGD_AsyncSocket is in use
\param socketModule The IGD_AsyncSocket to query
\returns 0 if in use, nonzero otherwise
*/
int IGD_AsyncSocket_IsFree(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	return(module->internalSocket==~0?1:0);
}

int IGD_AsyncSocket_IsConnected(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	return module->FinConnect;
}

/*! \fn IGD_AsyncSocket_GetPendingBytesToSend(IGD_AsyncSocket_SocketModule socketModule)
\brief Returns the number of bytes that are pending to be sent
\param socketModule The IGD_AsyncSocket to query
\returns Number of pending bytes
*/
unsigned int IGD_AsyncSocket_GetPendingBytesToSend(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	return(module->PendingBytesToSend);
}

/*! \fn IGD_AsyncSocket_GetTotalBytesSent(IGD_AsyncSocket_SocketModule socketModule)
\brief Returns the total number of bytes that have been sent, since the last reset
\param socketModule The IGD_AsyncSocket to query
\returns Number of bytes sent
*/
unsigned int IGD_AsyncSocket_GetTotalBytesSent(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	return(module->TotalBytesSent);
}

/*! \fn IGD_AsyncSocket_ResetTotalBytesSent(IGD_AsyncSocket_SocketModule socketModule)
\brief Resets the total bytes sent counter
\param socketModule The IGD_AsyncSocket to reset
*/
void IGD_AsyncSocket_ResetTotalBytesSent(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	module->TotalBytesSent = 0;
}

/*! \fn IGD_AsyncSocket_GetBuffer(IGD_AsyncSocket_SocketModule socketModule, char **buffer, int *BeginPointer, int *EndPointer)
\brief Returns the buffer associated with an IGD_AsyncSocket
\param socketModule The IGD_AsyncSocket to obtain the buffer from
\param[out] buffer The buffer
\param[out] BeginPointer Stating offset of the buffer
\param[out] EndPointer Length of buffer
*/
void IGD_AsyncSocket_GetBuffer(IGD_AsyncSocket_SocketModule socketModule, char **buffer, int *BeginPointer, int *EndPointer)
{
	struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;

	*buffer = module->buffer;
	*BeginPointer = module->BeginPointer;
	*EndPointer = module->EndPointer;
}

void IGD_AsyncSocket_ModuleOnConnect(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;
	if (module != NULL && module->OnConnect != NULL) module->OnConnect(module, -1, module->user);
}

// Set the SSL client context used by all connections done by this socket module. The SSL context must
// be set before using this module. If left to NULL, all connections are in the clear using TCP.
//
// This is utilized by the IGD_AsyncServerSocket module
// <param name="socketModule">The IGD_AsyncSocket to modify</param>
// <param name="ssl_ctx">The ssl_ctx structure</param>
#ifndef MICROSTACK_NOTLS
void IGD_AsyncSocket_SetSSLContext(IGD_AsyncSocket_SocketModule socketModule, SSL_CTX *ssl_ctx, int server)
{
	if (socketModule != NULL)
	{
		struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;
		if (ssl_ctx == NULL) return;

		if (module->ssl_ctx == NULL)
		{
			module->ssl_ctx = ssl_ctx;
			SSL_CTX_set_mode(ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
		}

		// If a socket is ready, setup SSL right now (otherwise, we will do this upon connection).
		if (module->internalSocket != 0 && module->internalSocket != ~0 && module->ssl == NULL)
		{
			module->ssl = SSL_new(ssl_ctx);
			module->sslstate = 0;
			module->sslbio = BIO_new_socket((int)(module->internalSocket), BIO_NOCLOSE);	// This is an odd conversion from SOCKET (possible 64bit) to 32 bit integer, but has to be done.
			//BIO_set_nbio(module->sslbio, 1); // Set BIO to non-blocking
			//SSL_CTX_set_mode(module->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
			SSL_set_bio(module->ssl, module->sslbio, module->sslbio);

			if (server != 0) SSL_set_accept_state(module->ssl); // Setup server SSL state
			else SSL_set_connect_state(module->ssl); // Setup client SSL state
		}
	}
}

SSL_CTX *IGD_AsyncSocket_GetSSLContext(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;
	return module->ssl_ctx;
}
#endif

//
// Sets the remote address field
//
// This is utilized by the IGD_AsyncServerSocket module
// <param name="socketModule">The IGD_AsyncSocket to modify</param>
// <param name="RemoteAddress">The remote interface</param>
void IGD_AsyncSocket_SetRemoteAddress(IGD_AsyncSocket_SocketModule socketModule, struct sockaddr *remoteAddress)
{
	if (socketModule != NULL)
	{
		struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;
		memcpy(&(module->RemoteAddress), remoteAddress, INET_SOCKADDR_LENGTH(remoteAddress->sa_family));
	}
}

/*! \fn IGD_AsyncSocket_UseThisSocket(IGD_AsyncSocket_SocketModule socketModule,void* UseThisSocket,IGD_AsyncSocket_OnInterrupt InterruptPtr,void *user)
\brief Associates an actual socket with IGD_AsyncSocket
\par
Instead of calling \a ConnectTo, you can call this method to associate with an already
connected socket.
\param socketModule The IGD_AsyncSocket to associate
\param UseThisSocket The socket to associate
\param InterruptPtr Function Pointer that triggers when the TCP connection is interrupted
\param user User object to associate with this session
*/
void IGD_AsyncSocket_UseThisSocket(IGD_AsyncSocket_SocketModule socketModule, void* UseThisSocket, IGD_AsyncSocket_OnInterrupt InterruptPtr, void *user)
{
#if defined(_WIN32_WCE) || defined(WIN32)
	SOCKET TheSocket = *((SOCKET*)UseThisSocket);
#elif defined(_POSIX)
	int TheSocket = *((int*)UseThisSocket);
#endif
	int flags;
	char *tmp;
	struct IGD_AsyncSocketModule* module = (struct IGD_AsyncSocketModule*)socketModule;

	module->PendingBytesToSend = 0;
	module->TotalBytesSent = 0;
	module->internalSocket = TheSocket;
	module->OnInterrupt = InterruptPtr;
	module->user = user;
	module->FinConnect = 1;
	module->PAUSE = 0;
	#ifndef MICROSTACK_NOTLS
	module->SSLConnect = 0;
	#endif

	//
	// If the buffer is too small/big, we need to realloc it to the minimum specified size
	//
	if (module->buffer != IGD_ScratchPad2)
	{
		if ((tmp = (char*)realloc(module->buffer, module->InitialSize)) == NULL) ILIBCRITICALEXIT(254);
		module->buffer = tmp;
		module->MallocSize = module->InitialSize;
	}
	module->BeginPointer = 0;
	module->EndPointer = 0;

	#ifndef MICROSTACK_NOTLS
	if (module->ssl_ctx != NULL)
	{
		module->ssl = SSL_new(module->ssl_ctx);
		module->sslstate = 0;
		module->sslbio = BIO_new_socket((int)(module->internalSocket), BIO_NOCLOSE);	// This is an odd conversion from SOCKET (possible 64bit) to 32 bit integer, but has to be done.
		//BIO_set_nbio(module->sslbio, 1); // Set BIO to non-blocking
		//SSL_CTX_set_mode(module->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
		SSL_set_bio(module->ssl, module->sslbio, module->sslbio);
		SSL_set_accept_state(module->ssl); // Setup server SSL state
	}
	#endif

	//
	// Make sure the socket is non-blocking, so we can play nice and share the thread
	//
#if defined(_WIN32_WCE) || defined(WIN32)
	flags = 1;
	ioctlsocket(module->internalSocket, FIONBIO,(u_long *)(&flags));
#elif defined(_POSIX)
	flags = fcntl(module->internalSocket,F_GETFL,0);
	fcntl(module->internalSocket,F_SETFL,O_NONBLOCK|flags);
#endif
}

/*! \fn IGD_AsyncSocket_GetRemoteInterface(IGD_AsyncSocket_SocketModule socketModule)
\brief Returns the Remote Interface of a connected session
\param socketModule The IGD_AsyncSocket to query
\returns The remote interface
*/
int IGD_AsyncSocket_GetRemoteInterface(IGD_AsyncSocket_SocketModule socketModule, struct sockaddr *remoteAddress)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	if (module->RemoteAddress.sin6_family != 0)
	{
		memcpy(remoteAddress, &(module->RemoteAddress), INET_SOCKADDR_LENGTH(module->RemoteAddress.sin6_family));
		return INET_SOCKADDR_LENGTH(module->RemoteAddress.sin6_family);
	}
	memcpy(remoteAddress, &(module->SourceAddress), INET_SOCKADDR_LENGTH(module->SourceAddress.sin6_family));
	return INET_SOCKADDR_LENGTH(module->SourceAddress.sin6_family);
}

/*! \fn IGD_AsyncSocket_GetLocalInterface(IGD_AsyncSocket_SocketModule socketModule)
\brief Returns the Local Interface of a connected session, in network order
\param socketModule The IGD_AsyncSocket to query
\returns The local interface
*/
int IGD_AsyncSocket_GetLocalInterface(IGD_AsyncSocket_SocketModule socketModule, struct sockaddr *localAddress)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	int receivingAddressLength = sizeof(struct sockaddr_in6);

	if (module->LocalAddress.sin6_family !=0)
	{
		memcpy(localAddress, &(module->LocalAddress), INET_SOCKADDR_LENGTH(module->LocalAddress.sin6_family));
		return INET_SOCKADDR_LENGTH(module->LocalAddress.sin6_family);
	}
	else
	{
#if defined(WINSOCK2)
		getsockname(module->internalSocket, localAddress, (int*)&receivingAddressLength);
#else
		getsockname(module->internalSocket, localAddress, (socklen_t*)&receivingAddressLength);
#endif
		return receivingAddressLength;
	}
}

unsigned short IGD_AsyncSocket_GetLocalPort(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	int receivingAddressLength = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 localAddress;

	if (module->LocalAddress.sin6_family == AF_INET6) return ntohs(module->LocalAddress.sin6_port);
	if (module->LocalAddress.sin6_family == AF_INET) return ntohs((((struct sockaddr_in*)(&(module->LocalAddress)))->sin_port));
#if defined(WINSOCK2)
	getsockname(module->internalSocket, (struct sockaddr*)&localAddress, (int*)&receivingAddressLength);
#else
	getsockname(module->internalSocket, (struct sockaddr*)&localAddress, (socklen_t*)&receivingAddressLength);
#endif
	if (localAddress.sin6_family == AF_INET6) return ntohs(localAddress.sin6_port);
	if (localAddress.sin6_family == AF_INET) return ntohs((((struct sockaddr_in*)(&localAddress))->sin_port));
	return 0;
}

/*! \fn IGD_AsyncSocket_Resume(IGD_AsyncSocket_SocketModule socketModule)
\brief Resumes a paused session
\par
Sessions can be paused, such that further data is not read from the socket until resumed
\param socketModule The IGD_AsyncSocket to resume
*/
void IGD_AsyncSocket_Resume(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)socketModule;
	if (sm != NULL && sm->PAUSE > 0)
	{
		sm->PAUSE = -1;
		IGD_ForceUnBlockChain(sm->Chain);
	}
}

/*! \fn IGD_AsyncSocket_GetSocket(IGD_AsyncSocket_SocketModule module)
\brief Obtain the underlying raw socket
\param module The IGD_AsyncSocket to query
\returns The raw socket
*/
void* IGD_AsyncSocket_GetSocket(IGD_AsyncSocket_SocketModule module)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)module;
	return(&(sm->internalSocket));
}

void IGD_AsyncSocket_SetLocalInterface(IGD_AsyncSocket_SocketModule module, struct sockaddr *LocalAddress)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)module;
	memcpy(&(sm->LocalAddress), LocalAddress, INET_SOCKADDR_LENGTH(LocalAddress->sa_family));
}

void IGD_AsyncSocket_SetMaximumBufferSize(IGD_AsyncSocket_SocketModule module, int maxSize, IGD_AsyncSocket_OnBufferSizeExceeded OnBufferSizeExceededCallback, void *user)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)module;
	sm->MaxBufferSize = maxSize;
	sm->OnBufferSizeExceeded = OnBufferSizeExceededCallback;
	sm->MaxBufferSizeUserObject = user;
}

void IGD_AsyncSocket_SetSendOK(IGD_AsyncSocket_SocketModule module, IGD_AsyncSocket_OnSendOK OnSendOK)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)module;
	sm->OnSendOK = OnSendOK;
}

int IGD_AsyncSocket_IsIPv6LinkLocal(struct sockaddr *LocalAddress)
{
	struct sockaddr_in6 *x = (struct sockaddr_in6*)LocalAddress;
#if defined(_WIN32_WCE) || defined(WIN32)
	if (LocalAddress->sa_family == AF_INET6 && x->sin6_addr.u.Byte[0] == 0xFE && x->sin6_addr.u.Byte[1] == 0x80) return 1;
#else
	if (LocalAddress->sa_family == AF_INET6 && x->sin6_addr.s6_addr[0] == 0xFE && x->sin6_addr.s6_addr[1] == 0x80) return 1;
#endif
	return 0;
}

int IGD_AsyncSocket_IsModuleIPv6LinkLocal(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *module = (struct IGD_AsyncSocketModule*)socketModule;
	return IGD_AsyncSocket_IsIPv6LinkLocal((struct sockaddr*)&(module->LocalAddress));
}

int IGD_AsyncSocket_WasClosedBecauseBufferSizeExceeded(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)socketModule;
	return(sm->MaxBufferSizeExceeded);
}

#ifndef MICROSTACK_NOTLS
X509 *IGD_AsyncSocket_SslGetCert(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)socketModule;
	return SSL_get_peer_certificate(sm->ssl);
}

STACK_OF(X509) *IGD_AsyncSocket_SslGetCerts(IGD_AsyncSocket_SocketModule socketModule)
{
	struct IGD_AsyncSocketModule *sm = (struct IGD_AsyncSocketModule*)socketModule;
	return SSL_get_peer_cert_chain(sm->ssl);
}
#endif

