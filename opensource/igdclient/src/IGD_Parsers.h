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


/*! \file IGD_Parsers.h 
\brief MicroStack APIs for various functions and tasks
*/

#ifndef __IGD_Parsers__
#define __IGD_Parsers__

#ifdef __cplusplus
extern "C" {
#endif

	/*! \defgroup IGD_Parsers IGD_Parser Modules
	\{
	*/

	/*! \def MAX_HEADER_LENGTH
	Specifies the maximum allowed length for an HTTP Header
	*/
#define MAX_HEADER_LENGTH 800

#ifdef MEMORY_CHECK
#include <assert.h>
#define MEMCHECK(x) x
#else
#define MEMCHECK(x)
#endif

	//extern struct sockaddr_in6;

#if !defined(WIN32) && !defined(_WIN32_WCE)
#define HANDLE int
#define SOCKET int
#endif

extern char* IGD_CriticalLogFilename;

#ifdef _WIN32_WCE
#define REQUIRES_MEMORY_ALIGNMENT
#define errno 0
#ifndef ERANGE
#define ERANGE 1
#endif
#define time(x) GetTickCount()
#endif

#ifndef WIN32
#define REQUIRES_MEMORY_ALIGNMENT
#endif

#if defined(WIN32) || defined (_WIN32_WCE)
#ifndef MICROSTACK_NO_STDAFX
#include "stdafx.h"
#endif
#elif defined(_POSIX) || defined (__APPLE__)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif
#include <fcntl.h>
#include <signal.h>
#define UNREFERENCED_PARAMETER(P)
#elif defined(__SYMBIAN32__)
#include <libc\stddef.h>
#include <libc\sys\types.h>
#include <libc\sys\socket.h>
#include <libc\netinet\in.h>
#include <libc\arpa\inet.h>
#include <libc\sys\time.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>


#if defined(WIN32) || defined(_WIN32_WCE)
#include <windows.h>
#include <winioctl.h>
#include <winbase.h>
#endif

#ifdef __APPLE__
#include <pthread.h>
#define sem_t pthread_mutex_t
#define sem_init(x,pShared,InitValue) pthread_mutex_init(x, NULL)
#define sem_destroy(x) pthread_mutex_destroy(x)
#define sem_wait(x) pthread_mutex_lock(x)
#define sem_trywait(x) pthread_mutex_trylock(x)
#define sem_post(x) pthread_mutex_unlock(x)
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
#include <time.h>
#include <sys/timeb.h>
#endif
#if defined(__SYMBIAN32__)
#include "IGD_SymbianSemaphore.h"
#define sem_t void*
#define sem_init(x,pShared,InitValue) IGD_Symbian_CreateSemaphore(x,InitValue)
#define sem_destroy(x) IGD_Symbian_DestroySemaphore(x)
#define sem_wait(x) IGD_Symbian_WaitSemaphore(x)
#define sem_trywait(x) IGD_Symbian_TryWaitSemaphore(x)
#define sem_post(x) IGD_Symbian_SignalSemaphore(x)
#endif

#if defined(_DEBUG)
#define PRINTERROR() printf("ERROR in %s, line %d\r\n", __FILE__, __LINE__);
#else
#define PRINTERROR()
#endif


#if defined(WIN32) || defined(_WIN32_WCE)
#ifndef FD_SETSIZE
#define SEM_MAX_COUNT 64
#else
#define SEM_MAX_COUNT FD_SETSIZE
#endif
	void IGD_GetTimeOfDay(struct timeval *tp);

/*
// Implemented in Windows using events.
#define sem_t HANDLE
#define sem_init(x,pShared,InitValue) *x=CreateSemaphore(NULL,InitValue,SEM_MAX_COUNT,NULL)
#define sem_destroy(x) (CloseHandle(*x)==0?1:0)
#define sem_wait(x) WaitForSingleObject(*x,INFINITE)
#define sem_trywait(x) ((WaitForSingleObject(*x,0)==WAIT_OBJECT_0)?0:1)
#define sem_post(x) ReleaseSemaphore(*x,1,NULL)
*/

// Implemented in Windows using critical section.
#define sem_t CRITICAL_SECTION
#define sem_init(x,pShared,InitValue) InitializeCriticalSection(x);
#define sem_destroy(x) (DeleteCriticalSection(x))
#define sem_wait(x) EnterCriticalSection(x)
#define sem_trywait(x) TryEnterCriticalSection(x)
#define sem_post(x) LeaveCriticalSection(x)

#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#define strcasecmp(x,y) _stricmp(x,y)
#define gettimeofday(tp,tzp) IGD_GetTimeOfDay(tp)

#if !defined(_WIN32_WCE)
#define tzset() _tzset()
#define daylight _daylight
#define timezone _timezone
#endif

#ifndef stricmp
#define stricmp(x,y) _stricmp(x,y)
#endif

#ifndef strnicmp
#define strnicmp(x,y,z) _strnicmp(x,y,z)
#endif

#ifndef strcmpi
#define strcmpi(x,y) _stricmp(x,y)
#endif
#endif

	typedef void (*voidfp)(void);		// Generic function pointer
	extern char IGD_ScratchPad[4096];   // General buffer
	extern char IGD_ScratchPad2[65536]; // Often used for UDP packet processing

	/*! \def UPnPMIN(a,b)
	Returns the minimum of \a a and \a b.
	*/
#define UPnPMIN(a,b) (((a)<(b))?(a):(b))
	/*! \def IGD_IsChainBeingDestroyed(Chain)
	Determines if the specified chain is in the process of being disposed.
	*/
#define IGD_IsChainBeingDestroyed(Chain) (*((int*)Chain))
#define IGD_IsChainRunning(Chain) (((int*)Chain)[1])

	typedef enum
	{
		IGD_ServerScope_All=0,
		IGD_ServerScope_LocalLoopback=1,
		IGD_ServerScope_LocalSegment=2
	}IGD_ServerScope;


	typedef	void(*IGD_Chain_PreSelect)(void* object,void *readset, void *writeset, void *errorset, int* blocktime);
	typedef	void(*IGD_Chain_PostSelect)(void* object,int slct, void *readset, void *writeset, void *errorset);
	typedef	void(*IGD_Chain_Destroy)(void* object);

	int IGD_GetCurrentTimezoneOffset_Minutes();
	int IGD_IsDaylightSavingsTime();
	int IGD_Time_ParseEx(char *timeString, time_t *val);
	time_t IGD_Time_Parse(char *timeString);
	char* IGD_Time_Serialize(time_t timeVal);
	long long IGD_GetUptime();

	typedef void* IGD_ReaderWriterLock;
	IGD_ReaderWriterLock IGD_ReaderWriterLock_Create();
	IGD_ReaderWriterLock IGD_ReaderWriterLock_CreateEx(void *chain);
	int IGD_ReaderWriterLock_ReadLock(IGD_ReaderWriterLock rwLock);
	int IGD_ReaderWriterLock_ReadUnLock(IGD_ReaderWriterLock rwLock);
	int IGD_ReaderWriterLock_WriteLock(IGD_ReaderWriterLock rwLock);
	int IGD_ReaderWriterLock_WriteUnLock(IGD_ReaderWriterLock rwLock);
	void IGD_ReaderWriterLock_Destroy(IGD_ReaderWriterLock rwLock);

#if defined(__SYMBIAN32__)
	typedef void (*IGD_OnChainStopped)(void *user);
#endif

	/*! \defgroup DataStructures Data Structures
	\ingroup IGD_Parsers
	\{
	\}
	*/


	/*! \struct parser_result_field IGD_Parsers.h
	\brief Data Elements of \a parser_result
	\par
	This structure represents individual tokens, resulting from a call to
	\a IGD_ParseString and \a IGD_ParseStringAdv
	*/
	struct parser_result_field
	{
		/*! \var data
		\brief Pointer to string
		*/
		char* data;

		/*! \var datalength
		\brief Length of \a data
		*/
		int datalength;

		/*! \var NextResult
		\brief Pointer to next token
		*/
		struct parser_result_field *NextResult;
	};

	/*! \struct parser_result IGD_Parsers.h
	\brief String Parsing Result Index
	\par
	This is returned from a successfull call to either \a IGD_ParseString or
	\a IGD_ParseStringAdv.
	*/
	struct parser_result
	{
		/*! \var FirstResult
		\brief Pointer to the first token
		*/
		struct parser_result_field *FirstResult;
		/*! \var LastResult
		\brief Pointer to the last token
		*/
		struct parser_result_field *LastResult;
		/*! \var NumResults
		\brief The total number of tokens
		*/
		int NumResults;
	};

	/*! \struct packetheader_field_node IGD_Parsers.h
	\brief HTTP Headers
	\par
	This structure represents an individual header element. A list of these
	is referenced from a \a packetheader_field_node.
	*/
	struct packetheader_field_node
	{
		/*! \var Field
		\brief Header Name
		*/
		char* Field;
		/*! \var FieldLength
		\brief Length of \a Field
		*/
		int FieldLength;
		/*! \var FieldData
		\brief Header Value
		*/
		char* FieldData;
		/*! \var FieldDataLength
		\brief Length of \a FieldData
		*/
		int FieldDataLength;
		/*! \var UserAllocStrings
		\brief Boolean indicating if the above strings are non-static memory
		*/
		int UserAllocStrings;
		/*! \var NextField
		\brief Pointer to the Next Header entry. NULL if this is the last one
		*/
		struct packetheader_field_node* NextField;
	};

	/*! \struct packetheader IGD_Parsers.h
	\brief Structure representing a packet formatted according to HTTP encoding rules
	\par
	This can be created manually by calling \a IGD_CreateEmptyPacket(), or automatically via a call to \a IGD_ParsePacketHeader(...)
	*/
	struct packetheader
	{
		/*! \var Directive
		\brief HTTP Method
		\par
		eg: \b GET /index.html HTTP/1.1
		*/
		char* Directive;
		/*! \var DirectiveLength
		\brief Length of \a Directive
		*/
		int DirectiveLength;
		/*! \var DirectiveObj
		\brief HTTP Method Path
		\par
		eg: GET \b /index.html HTTP/1.1
		*/
		char* DirectiveObj;
		/*! \var DirectiveObjLength
		\brief Length of \a DirectiveObj
		*/

		void *Reserved;

		int DirectiveObjLength;
		/*! \var StatusCode
		\brief HTTP Response Code
		\par
		eg: HTTP/1.1 \b 200 OK
		*/
		int StatusCode;
		/* \var StatusData
		\brief Status Meta Data
		\par
		eg: HTTP/1.1 200 \b OK
		*/
		char* StatusData;
		/*! \var StatusDataLength
		\brief Length of \a StatusData
		*/
		int StatusDataLength;
		/*! \var Version
		\brief HTTP Version
		\par
		eg: 1.1
		*/
		char* Version;
		/*! \var VersionLength
		\brief Length of \a Version
		*/
		int VersionLength;
		/*! \var Body
		\brief Pointer to HTTP Body
		*/
		char* Body;
		/*! \var BodyLength
		\brief Length of \a Body
		*/
		int BodyLength;
		/*! \var UserAllocStrings
		\brief Boolean indicating if Directive/Obj are non-static
		\par
		This only needs to be set, if you manually populate \a Directive and \a DirectiveObj.<br>
		It is \b recommended that you use \a IGD_SetDirective
		*/
		int UserAllocStrings;	// Set flag if you allocate memory pointed to by Directive/Obj
		/*! \var UserAllocVersion
		\brief Boolean indicating if Version string is non-static
		\par
		This only needs to be set, if you manually populate \a Version.<br>
		It is \b recommended that you use \a IGD_SetVersion
		*/	
		int UserAllocVersion;	// Set flag if you allocate memory pointed to by Version
		int ClonedPacket;

		/*! \var FirstField
		\brief Pointer to the first Header field
		*/
		struct packetheader_field_node* FirstField;
		/*! \var LastField
		\brief Pointer to the last Header field
		*/
		struct packetheader_field_node* LastField;

		/*! \var Source
		\brief The origin of this packet
		\par
		This is only populated if you obtain this structure from either \a IGD_WebServer or
		\a IGD_WebClient.
		*/
		char Source[30];
		/*! \var ReceivingAddress
		\brief IP address that this packet was received on
		\par
		This is only populated if you obtain this structure from either \a IGD_WebServer or
		\a IGD_WebClient.
		*/
		char ReceivingAddress[30];

		void *HeaderTable;
	};

	/*! \struct IGD_XMLNode
	\brief An XML Tree
	\par
	This is obtained via a call to \a IGD_ParseXML. It is \b highly \b recommended
	that you call \a IGD_ProcessXMLNodeList, so that the node pointers at the end of this
	structure will be populated. That will greatly simplify node traversal.<br><br>
	In order for namespaces to be resolved, you must call \a IGD_XML_BuildNamespaceLookupTable(...)
	with root-most node that you would like to resolve namespaces for. It is recommended that you always use
	the root node, obtained from the initial call to \a IGD_ParseXML.<br><br>
	For most intents and purposes, you only need to work with the "StartElements"
	*/
	struct IGD_XMLNode
	{
		/*! \var Name
		\brief Local Name of the current element
		*/
		char* Name;			// Element Name
		/*! \var NameLength
		\brief Length of \a Name
		*/
		int NameLength;

		/*! \var NSTag
		\brief Namespace Prefix of the current element
		\par
		This can be resolved using a call to \a IGD_XML_LookupNamespace(...)
		*/
		char* NSTag;		// Element Prefix
		/*! \var NSLength
		\brief Length of \a NSTag
		*/
		int NSLength;

		/*! \var StartTag
		\brief boolean indicating if the current element is a start element
		*/
		int StartTag;		// Non zero if this is a StartElement
		/*! \var EmptyTag
		\brief boolean indicating if this element is an empty element
		*/
		int EmptyTag;		// Non zero if this is an EmptyElement

		void *Reserved;		// DO NOT TOUCH
		void *Reserved2;	// DO NOT TOUCH

		/*! \var Next
		\brief Pointer to the child of the current node
		*/
		struct IGD_XMLNode *Next;			// Next Node
		/*! \var Parent
		\brief Pointer to the Parent of the current node
		*/
		struct IGD_XMLNode *Parent;			// Parent Node
		/*! \var Peer
		\brief Pointer to the sibling of the current node
		*/
		struct IGD_XMLNode *Peer;			// Sibling Node
		struct IGD_XMLNode *ClosingTag;		// Pointer to closing node of this element
		struct IGD_XMLNode *StartingTag;	// Pointer to start node of this element
	};

	/*! \struct IGD_XMLAttribute
	\brief A list of XML Attributes for a specified XML node
	\par
	This can be obtained via a call to \a IGD_GetXMLAttributes(...)
	*/
	struct IGD_XMLAttribute
	{
		/*! \var Name
		\brief Local name of Attribute
		*/
		char* Name;						// Attribute Name
		/*! \var NameLength
		\brief Length of \a Name
		*/
		int NameLength;

		/*! \var Prefix
		\brief Namespace Prefix of this attribute
		\par
		This can be resolved by calling \a IGD_XML_LookupNamespace(...) and passing in \a Parent as the current node
		*/
		char* Prefix;					// Attribute Namespace Prefix
		/*! \var PrefixLength
		\brief Lenth of \a Prefix
		*/
		int PrefixLength;

		/*! \var Parent
		\brief Pointer to the XML Node that contains this attribute
		*/
		struct IGD_XMLNode *Parent;		// The XML Node this attribute belongs to

		/*! \var Value
		\brief Attribute Value
		*/
		char* Value;					// Attribute Value	
		/*! \var ValueLength
		\brief Length of \a Value
		*/
		int ValueLength;
		/*! \var Next
		\brief Pointer to the next attribute
		*/
		struct IGD_XMLAttribute *Next;	// Next Attribute
	};

	char *IGD_ReadFileFromDisk(char *FileName);
	int IGD_ReadFileFromDiskEx(char **Target, char *FileName);
	void IGD_WriteStringToDisk(char *FileName, char *data);
	void IGD_AppendStringToDiskEx(char *FileName, char *data, int dataLen);
	void IGD_WriteStringToDiskEx(char *FileName, char *data, int dataLen);
	void IGD_DeleteFileFromDisk(char *FileName);
	void IGD_GetDiskFreeSpace(void *i64FreeBytesToCaller, void *i64TotalBytes);


	/*! \defgroup StackGroup Stack
	\ingroup DataStructures
	Stack Methods
	\{
	*/
	void IGD_CreateStack(void **TheStack);
	void IGD_PushStack(void **TheStack, void *data);
	void *IGD_PopStack(void **TheStack);
	void *IGD_PeekStack(void **TheStack);
	void IGD_ClearStack(void **TheStack);
	/*! \} */

	/*! \defgroup QueueGroup Queue
	\ingroup DataStructures
	Queue Methods
	\{
	*/
	void *IGD_Queue_Create();
	void IGD_Queue_Destroy(void *q);
	int IGD_Queue_IsEmpty(void *q);
	void IGD_Queue_EnQueue(void *q, void *data);
	void *IGD_Queue_DeQueue(void *q);
	void *IGD_Queue_PeekQueue(void *q);
	void IGD_Queue_Lock(void *q);
	void IGD_Queue_UnLock(void *q);
	long IGD_Queue_GetCount(void *q);
	/* \} */


	/*! \defgroup XML XML Parsing Methods
	\ingroup IGD_Parsers
	MicroStack supplied XML Parsing Methods
	\par
	\b Note: None of the XML Parsing Methods copy or allocate memory
	The lifetime of any/all strings is bound to the underlying string that was
	parsed using IGD_ParseXML. If you wish to keep any of these strings for longer
	then the lifetime of the underlying string, you must copy the string.
	\{
	*/


	//
	// Parses an XML string. Returns a tree of IGD_XMLNode elements.
	//
	struct IGD_XMLNode *IGD_ParseXML(char *buffer, int offset, int length);

	//
	// Preprocesses the tree of IGD_XMLNode elements returned by IGD_ParseXML.
	// This method populates all the node pointers in each node for easy traversal.
	// In addition, this method will also determine if the given XML document was well formed.
	// Returns 0 if processing succeeded. Specific Error Codes are returned otherwise.
	//
	int IGD_ProcessXMLNodeList(struct IGD_XMLNode *nodeList);

	//
	// Initalizes a namespace lookup table for a given parent node. 
	// If you want to resolve namespaces, you must call this method exactly once
	//
	void IGD_XML_BuildNamespaceLookupTable(struct IGD_XMLNode *node);

	//
	// Resolves a namespace prefix.
	//
	char* IGD_XML_LookupNamespace(struct IGD_XMLNode *currentLocation, char *prefix, int prefixLength);

	//
	// Fetches all the data for an element. Returns the length of the populated data
	//
	int IGD_ReadInnerXML(struct IGD_XMLNode *node, char **RetVal);

	//
	// Returns the attributes of an XML element. Returned as a linked list of IGD_XMLAttribute.
	//
	struct IGD_XMLAttribute *IGD_GetXMLAttributes(struct IGD_XMLNode *node);

	void IGD_DestructXMLNodeList(struct IGD_XMLNode *node);
	void IGD_DestructXMLAttributeList(struct IGD_XMLAttribute *attribute);

	//
	// Escapes an XML string.
	// indata must be pre-allocated. 
	//
	int IGD_XmlEscape(char* outdata, const char* indata);

	//
	// Returns the required size string necessary to escape this XML string
	//
	int IGD_XmlEscapeLength(const char* data);

	//
	// Unescapes an XML string.
	// Since Unescaped strings are always shorter than escaped strings,
	// the resultant string will overwrite the supplied string, to save memory
	//
	int IGD_InPlaceXmlUnEscape(char* data);

	/*! \} */

	/*! \defgroup ChainGroup Chain Methods
	\ingroup IGD_Parsers
	\brief Chaining Methods
	\{
	*/
	void *IGD_CreateChain();
	void IGD_AddToChain(void *chain, void *object);
	void *IGD_GetBaseTimer(void *chain);
	void IGD_Chain_SafeAdd(void *chain, void *object);
	void IGD_Chain_SafeRemove(void *chain, void *object);
	void IGD_Chain_SafeAdd_SubChain(void *chain, void *subChain);
	void IGD_Chain_SafeRemove_SubChain(void *chain, void *subChain);
	void IGD_Chain_DestroyEx(void *chain);
	void IGD_StartChain(void *chain);
	void IGD_StopChain(void *chain);
#if defined(__SYMBIAN32__)
	void IGD_Chain_SetOnStoppedHandler(void *chain, void *user, IGD_OnChainStopped Handler);
#endif
	void IGD_ForceUnBlockChain(void *Chain);
	/* \} */



	/*! \defgroup LinkedListGroup Linked List
	\ingroup DataStructures
	\{
	*/

	typedef void* IGD_LinkedList;
	//
	// Initializes a new Linked List data structre
	//
	void* IGD_LinkedList_Create();

	//
	// Returns the Head node of a linked list data structure
	//
	void* IGD_LinkedList_GetNode_Head(void *LinkedList);		// Returns Node

	//
	// Returns the Tail node of a linked list data structure
	//
	void* IGD_LinkedList_GetNode_Tail(void *LinkedList);		// Returns Node

	//
	// Returns the Next node of a linked list data structure
	//
	void* IGD_LinkedList_GetNextNode(void *LinkedList_Node);	// Returns Node

	//
	// Returns the Previous node of a linked list data structure
	//
	void* IGD_LinkedList_GetPreviousNode(void *LinkedList_Node);// Returns Node

	//
	// Returns the number of nodes contained in a linked list data structure
	//
	long IGD_LinkedList_GetCount(void *LinkedList);

	//
	// Returns a shallow copy of a linked list data structure. That is, the structure
	// is copied, but none of the data contents are copied. The pointer values are just copied.
	//
	void* IGD_LinkedList_ShallowCopy(void *LinkedList);

	//
	// Returns the data pointer of a linked list element
	//
	void *IGD_LinkedList_GetDataFromNode(void *LinkedList_Node);

	//
	// Creates a new element, and inserts it before the given node
	//
	void IGD_LinkedList_InsertBefore(void *LinkedList_Node, void *data);

	//
	// Creates a new element, and inserts it after the given node
	//
	void IGD_LinkedList_InsertAfter(void *LinkedList_Node, void *data);

	//
	// Removes the given node from a linked list data structure
	//
	void* IGD_LinkedList_Remove(void *LinkedList_Node);

	//
	// Given a data pointer, will traverse the linked list data structure, deleting
	// elements that point to this data pointer.
	//
	int IGD_LinkedList_Remove_ByData(void *LinkedList, void *data);

	//
	// Creates a new element, and inserts it at the top of the linked list.
	//
	void IGD_LinkedList_AddHead(void *LinkedList, void *data);

	//
	// Creates a new element, and appends it to the end of the linked list
	//
	void IGD_LinkedList_AddTail(void *LinkedList, void *data);

	void IGD_LinkedList_Lock(void *LinkedList);
	void IGD_LinkedList_UnLock(void *LinkedList);
	void IGD_LinkedList_Destroy(void *LinkedList);
	/*! \} */



	/*! \defgroup HashTreeGroup Hash Table
	\ingroup DataStructures
	\b Note: Duplicate key entries will be overwritten.
	\{
	*/

	//
	// Initialises a new Hash Table (tree) data structure
	//
	void* IGD_InitHashTree();
	void* IGD_InitHashTree_CaseInSensitive();
	void IGD_DestroyHashTree(void *tree);

	//
	// Returns non-zero if the key entry is found in the table
	//
	int IGD_HasEntry(void *hashtree, char* key, int keylength);

	//
	// Add a new entry into the hashtable. If the key is already used, it will be overwriten.
	//
	void IGD_AddEntry(void* hashtree, char* key, int keylength, void *value);
	void IGD_AddEntryEx(void* hashtree, char* key, int keylength, void *value, int valueEx);
	void* IGD_GetEntry(void *hashtree, char* key, int keylength);
	void IGD_GetEntryEx(void *hashtree, char* key, int keylength, void **value, int *valueEx);
	void IGD_DeleteEntry(void *hashtree, char* key, int keylength);

	//
	// Returns an Enumerator to browse all the entries of the Hashtable
	//
	void *IGD_HashTree_GetEnumerator(void *tree);
	void IGD_HashTree_DestroyEnumerator(void *tree_enumerator);

	//
	// Advance the Enumerator to the next element.
	// Returns non-zero if there are no more entries to enumerate
	//
	int IGD_HashTree_MoveNext(void *tree_enumerator);

	//
	// Obtains the value of a Hashtable Entry.
	//
	void IGD_HashTree_GetValue(void *tree_enumerator, char **key, int *keyLength, void **data);
	void IGD_HashTree_GetValueEx(void *tree_enumerator, char **key, int *keyLength, void **data, int *dataEx);
	void IGD_HashTree_Lock(void *hashtree);
	void IGD_HashTree_UnLock(void *hashtree);

	/*! \} */

	/*! \defgroup LifeTimeMonitor LifeTimeMonitor
	\ingroup IGD_Parsers
	\brief Timed Callback Service
	\par
	These callbacks will always be triggered on the thread that calls IGD_StartChain().
	\{
	*/

	typedef void(*IGD_LifeTime_OnCallback)(void *obj);

	//
	// Adds an event trigger to be called after the specified time elapses, with the
	// specified data object
	//
#define IGD_LifeTime_Add(LifetimeMonitorObject, data, seconds, Callback, Destroy) IGD_LifeTime_AddEx(LifetimeMonitorObject, data, seconds * 1000, (IGD_LifeTime_OnCallback)Callback, (IGD_LifeTime_OnCallback)Destroy)

	void IGD_LifeTime_AddEx(void *LifetimeMonitorObject,void *data, int milliseconds, IGD_LifeTime_OnCallback Callback, IGD_LifeTime_OnCallback Destroy);

	//
	// Removes all event triggers that contain the specified data object.
	//
	void IGD_LifeTime_Remove(void *LifeTimeToken, void *data);

	//
	// Return the expiration time for an event
	//
	long long IGD_LifeTime_GetExpiration(void *LifetimeMonitorObject, void *data);

	//
	// Removes all events triggers
	//
	void IGD_LifeTime_Flush(void *LifeTimeToken);
	void *IGD_CreateLifeTime(void *Chain);
	long IGD_LifeTime_Count(void* LifeTimeToken);

	/* \} */


	/*! \defgroup StringParsing String Parsing
	\ingroup IGD_Parsers
	\{
	*/

	int IGD_FindEntryInTable(char *Entry, char **Table);

	//
	// Trims preceding and proceding whitespaces from a string
	//
	int IGD_TrimString(char **theString, int length);

	int IGD_String_IndexOfFirstWhiteSpace(const char *inString, int inStringLength);
	char* IGD_String_Cat(const char *inString1, int inString1Len, const char *inString2, int inString2Len);
	char *IGD_String_Copy(const char *inString, int length);
	int IGD_String_EndsWith(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength);
	int IGD_String_EndsWithEx(const char *inString, int inStringLength, const char *endWithString, int endWithStringLength, int caseSensitive);
	int IGD_String_StartsWith(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength);
	int IGD_String_StartsWithEx(const char *inString, int inStringLength, const char *startsWithString, int startsWithStringLength, int caseSensitive);
	int IGD_String_IndexOfEx(const char *inString, int inStringLength, const char *indexOf, int indexOfLength, int caseSensitive);
	int IGD_String_IndexOf(const char *inString, int inStringLength, const char *indexOf, int indexOfLength);
	int IGD_String_LastIndexOf(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength);
	int IGD_String_LastIndexOfEx(const char *inString, int inStringLength, const char *lastIndexOf, int lastIndexOfLength, int caseSensitive);
	char *IGD_String_Replace(const char *inString, int inStringLength, const char *replaceThis, int replaceThisLength, const char *replaceWithThis, int replaceWithThisLength);
	char *IGD_String_ToUpper(const char *inString, int length);
	char *IGD_String_ToLower(const char *inString, int length);
	void IGD_ToUpper(const char *in, int inLength, char *out);
	void IGD_ToLower(const char *in, int inLength, char *out);
	//
	// Parses the given string using the specified multichar delimiter.
	// Returns a parser_result object, which points to a linked list
	// of parser_result_field objects.
	//
	struct parser_result* IGD_ParseString (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength);

	//
	// Same as IGD_ParseString, except this method ignore all delimiters that are contains within
	// quotation marks
	//
	struct parser_result* IGD_ParseStringAdv (char* buffer, int offset, int length, const char* Delimiter, int DelimiterLength);

	//
	// Releases resources used by string parser
	//
	void IGD_DestructParserResults(struct parser_result *result);

	//
	// Parses a URI into Address, Port Number, and Path components
	// Note: IP and Path must be freed.
	//
	void IGD_ParseUri(const char* URI, char** Addr, unsigned short* Port, char** Path, struct sockaddr_in6* AddrStruct);

	//
	// Parses a string into a Long or unsigned Long. 
	// Returns non-zero on error condition
	//
	int IGD_GetLong(char *TestValue, int TestValueLength, long* NumericValue);
	int IGD_GetULong(const char *TestValue, const int TestValueLength, unsigned long* NumericValue);
	int IGD_FragmentText(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength, char **RetVal);
	int IGD_FragmentTextLength(char *text, int textLength, char *delimiter, int delimiterLength, int tokenLength);


	/* Base64 handling methods */
	int IGD_Base64Encode(unsigned char* input, const int inputlen, unsigned char** output);
	int IGD_Base64Decode(unsigned char* input, const int inputlen, unsigned char** output);

	/* Compression Handling Methods */
	char* IGD_DecompressString(unsigned char* CurrentCompressed, const int bufferLength, const int DecompressedLength);

	/* \} */


	/*! \defgroup PacketParsing Packet Parsing
	\ingroup IGD_Parsers
	\{
	*/

	/* Packet Methods */

	//
	// Allocates an empty HTTP Packet
	//
	struct packetheader *IGD_CreateEmptyPacket();

	//
	// Add a header into the packet. (String is copied)
	//
	void IGD_AddHeaderLine(struct packetheader *packet, const char* FieldName, int FieldNameLength, const char* FieldData, int FieldDataLength);
	void IGD_DeleteHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);

	//
	// Fetches the header value from the packet. (String is NOT copied)
	// Returns NULL if the header field does not exist
	//
	char* IGD_GetHeaderLine(struct packetheader *packet, char* FieldName, int FieldNameLength);

	//
	// Sets the HTTP version: 1.0, 1.1, etc. (string is copied)
	//
	void IGD_SetVersion(struct packetheader *packet, char* Version, int VersionLength);

	//
	// Set the status code and data line. The status data is copied.
	//
	void IGD_SetStatusCode(struct packetheader *packet, int StatusCode, char* StatusData, int StatusDataLength);

	//
	// Sets the method and path. (strings are copied)
	//
	void IGD_SetDirective(struct packetheader *packet, char* Directive, int DirectiveLength, char* DirectiveObj, int DirectiveObjLength);

	//
	// Releases all resources consumed by this packet structure
	//
	void IGD_DestructPacket(struct packetheader *packet);

	//
	// Parses a string into an packet structure.
	// None of the strings are copied, so the lifetime of all the values are bound
	// to the lifetime of the underlying string that is parsed.
	//
	struct packetheader* IGD_ParsePacketHeader(char* buffer, int offset, int length);

	//
	// Returns the packetized string and it's length. (must be freed)
	//
	int IGD_GetRawPacket(struct packetheader *packet,char **buffer);

	//
	// Performs a deep copy of a packet structure
	//
	struct packetheader* IGD_ClonePacket(struct packetheader *packet);

	//
	// Escapes a string according to HTTP escaping rules.
	// indata must be pre-allocated
	//
	int IGD_HTTPEscape(char* outdata, const char* indata);

	//
	// Returns the size of string required to escape this string,
	// according to HTTP escaping rules
	//
	int IGD_HTTPEscapeLength(const char* data);

	//
	// Unescapes the escaped string sequence
	// Since escaped string sequences are always longer than unescaped
	// string sequences, the resultant string is overwritten onto the supplied string
	//
	int IGD_InPlaceHTTPUnEscape(char* data);
	int IGD_InPlaceHTTPUnEscapeEx(char* data, int length);
	/* \} */

	/*! \defgroup NetworkHelper Network Helper
	\ingroup IGD_Parsers
	\{
	*/

	//
	// Obtain an array of IP Addresses available on the local machine.
	//
	int IGD_GetLocalIPv6IndexList(int** indexlist);
	int IGD_GetLocalIPv6List(struct sockaddr_in6** list);
	int IGD_GetLocalIPAddressList(int** pp_int);
	int IGD_GetLocalIPv4AddressList(struct sockaddr_in** addresslist, int includeloopback);
	//int IGD_GetLocalIPv6AddressList(struct sockaddr_in6** addresslist);

#if defined(WINSOCK2)
	int IGD_GetLocalIPAddressNetMask(unsigned int address);
#endif

	SOCKET IGD_GetSocket(struct sockaddr *localif, int type, int protocol);

	/* \} */

	void* dbg_malloc(int sz);
	void dbg_free(void* ptr);
	int dbg_GetCount();

	//
	// IPv6 helper methods
	//
	void IGD_MakeIPv6Addr(struct sockaddr *addr4, struct sockaddr_in6* addr6);
	int IGD_MakeHttpHeaderAddr(struct sockaddr *addr, char** str);
	int IGD_IsIPv4MappedAddr(struct sockaddr *addr);
	int IGD_IsLoopback(struct sockaddr *addr);
	int IGD_GetAddrBlob(struct sockaddr *addr, char** ptr);
	void IGD_GetAddrFromBlob(char* ptr, int len, unsigned short port, struct sockaddr_in6 *addr);
	int IGD_DetectIPv6Support();
	extern int g_IGD_DetectIPv6Support;
	char* IGD_Inet_ntop2(struct sockaddr* addr, char *dst, size_t dstsize);
	char* IGD_Inet_ntop(int af, const void *src, char *dst, size_t dstsize);
	int IGD_Inet_pton(int af, const char *src, void *dst);
	long long htonll(long long value);
	int IGD_Resolve(char* hostname, char* service, struct sockaddr_in6* addr6);
	
	//
	// Used to log critical problems
	//
#if defined(WIN32)
#define ILIBCRITICALERREXIT(ex) { IGD_CriticalLog(NULL, __FILE__, __LINE__, GetLastError(), 0); exit(ex); }
#define ILIBCRITICALEXIT(ex) {IGD_CriticalLog(NULL, __FILE__, __LINE__, ex, GetLastError());printf("CRITICALEXIT, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT2(ex,u) {IGD_CriticalLog(NULL, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT2, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT3(ex,m,u) {IGD_CriticalLog(m, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT3, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBMARKPOSITION(ex) {IGD_CriticalLog(NULL, __FILE__, __LINE__, ex, GetLastError());}
#define ILIBMESSAGE(m) {IGD_CriticalLog(m, __FILE__, __LINE__, 0, GetLastError());printf("ILIBMSG: %s (%d).\r\n", m, (int)GetLastError());}
#define ILIBMESSAGE2(m,u) {IGD_CriticalLog(m, __FILE__, __LINE__, u, GetLastError());printf("ILIBMSG: %s (%d,%d).\r\n", m, u, (int)GetLastError());}
#else
#define ILIBCRITICALERREXIT(ex) { IGD_CriticalLog(NULL, __FILE__, __LINE__, errno, 0); exit(ex); }
#define ILIBCRITICALEXIT(ex) {IGD_CriticalLog(NULL, __FILE__, __LINE__, ex, errno); printf("CRITICALEXIT, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT2(ex,u) {IGD_CriticalLog(NULL, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT2, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBCRITICALEXIT3(ex,m,u) {IGD_CriticalLog(m, __FILE__, __LINE__, ex, u); printf("CRITICALEXIT3, FILE: %s, LINE: %d\r\n", __FILE__, __LINE__); exit(ex);}
#define ILIBMARKPOSITION(ex) {IGD_CriticalLog(NULL, __FILE__, __LINE__, ex, errno);}
#define ILIBMESSAGE(m) {IGD_CriticalLog(m, __FILE__, __LINE__, 0, errno);printf("ILIBMSG: %s\r\n", m);}
#define ILIBMESSAGE2(m,u) {IGD_CriticalLog(m, __FILE__, __LINE__, u, errno);printf("ILIBMSG: %s (%d)\r\n", m, u);}
#endif

	void IGD_CriticalLog(const char* msg, const char* file, int line, int user1, int user2);


	void* IGD_SpawnNormalThread(voidfp method, void* arg);
	void  IGD_EndThisThread();

	char* IGD_ToHex(char* data, int len, char* out);

#ifdef __cplusplus
}
#endif

/* \} */   // End of IGD_Parser Group
#endif

