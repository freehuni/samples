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

#if defined(WINSOCK2)
	#include <winsock2.h>
	#include <ws2tcpip.h>
#elif defined(WINSOCK1)
	#include <winsock.h>
	#include <wininet.h>
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
	#define _CRTDBG_MAP_ALLOC
#endif

#include "IGD_Parsers.h"
#include "IGD_SSDPClient.h"
#include "IGD_AsyncUDPSocket.h"

#ifndef _WIN32_WCE
#include <time.h>
#endif

#if defined(WIN32) && !defined(_WIN32_WCE)
	#include <crtdbg.h>
#endif

#if defined(WIN32)
#define snprintf(dst, len, frm, ...) _snprintf_s(dst, len, _TRUNCATE, frm, __VA_ARGS__)
#endif

#define UPNP_PORT 1900
#define UPNP_MCASTv4_GROUP "239.255.255.250"
#define UPNP_MCASTv6_GROUP "FF05:0:0:0:0:0:0:C"
#define UPNP_MCASTv6_GROUPB "[FF05:0:0:0:0:0:0:C]"
#define UPNP_MCASTv6_LINK_GROUP "FF02:0:0:0:0:0:0:C"
#define UPNP_MCASTv6_LINK_GROUPB "[FF02:0:0:0:0:0:0:C]"
#define DEBUGSTATEMENT(x)
#define INET_SOCKADDR_LENGTH(x) ((x==AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))

struct SSDPClientModule
{
	IGD_Chain_PreSelect PreSelect;
	IGD_Chain_PostSelect PostSelect;
	IGD_Chain_Destroy Destroy;
	void (*FunctionCallback)(void *sender, char* UDN, int Alive, char* LocationURL, struct sockaddr* LocationAddr, int Timeout, UPnPSSDP_MESSAGE m, void *user);
	char* DeviceURN;
	int DeviceURNLength;

	char* DeviceURN_Prefix;
	int DeviceURN_PrefixLength;
	int BaseDeviceVersionNumber;
	
	// Multicast Addresses
	struct sockaddr_in MulticastAddrV4;
	struct sockaddr_in6 MulticastAddrV6LL;
	struct sockaddr_in6 MulticastAddrV6SL;

	// Local Address Lists
	struct sockaddr_in *IPAddressListV4;
	int IPAddressListV4Length;
	struct sockaddr_in6 *IPAddressListV6;
	int IPAddressListV6Length;

	void *HashTable;
	void *TIMER;
	void *UNICAST_Socket4;
	void *UNICAST_Socket6;

	void* SSDPListenSocket4;
	void* SSDPListenSocket6;
	void* MSEARCH_Response_Socket4;
	void* MSEARCH_Response_Socket6;

	int Terminate;
	void *Reserved;
};


struct LocationInfo
{
	int BootID;
	int ConfigID;
	char* LocationURL;
	struct sockaddr_in6 HttpAddr;
	struct sockaddr_in6 SearchAddr;
	int IsInteresting;
	int Timeout;
	char *UDN;
	struct SSDPClientModule* parent;
};

void IGD_SSDP_UnicastSearch(void *SSDPToken, struct sockaddr *addr)
{
	char* group;
	int bufferlength;
	struct SSDPClientModule *module = (struct SSDPClientModule*)SSDPToken;

	// Setup the multicast group & build the packet
	if (addr->sa_family == AF_INET) group = UPNP_MCASTv4_GROUP; else if (IGD_AsyncSocket_IsIPv6LinkLocal(addr)) group = UPNP_MCASTv6_LINK_GROUPB; else group = UPNP_MCASTv6_GROUPB;
	bufferlength = snprintf(IGD_ScratchPad, sizeof(IGD_ScratchPad), "M-SEARCH * HTTP/1.1\r\nMX: 3\r\nST: %s\r\nHOST: %s:1900\r\nMAN: \"ssdp:discover\"\r\n\r\n", module->DeviceURN, group);
	
	//printf("UNICASTING to %s:%u...\r\n", IP, Port);
	if (addr->sa_family == AF_INET6) IGD_AsyncUDPSocket_SendTo(module->UNICAST_Socket6, addr, IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
	else if (addr->sa_family == AF_INET) IGD_AsyncUDPSocket_SendTo(module->UNICAST_Socket4, addr, IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
}

void IGD_SSDP_LocationSink(void *obj)
{
	struct LocationInfo* LI = (struct LocationInfo*)obj;
	
	// Send UNICAST M-SEARCH to the device
	IGD_SSDP_UnicastSearch(LI->parent, (struct sockaddr*)&(LI->SearchAddr));
}

//void IGD_SSDP_LocationSink_Destroy(void *obj)
//{
//	struct LocationInfo* LI = (struct LocationInfo*)obj;
//	free(LI->LocationURL);
//	free(LI);
//}

void IGD_ReadSSDP(struct packetheader *packet, struct sockaddr_in6 *remoteInterface, struct SSDPClientModule *module)
{
	struct packetheader_field_node *node;
	struct parser_result *pnode, *pnode2;
	struct parser_result_field *prf;
	
	char* Location = NULL;
	char* UDN = NULL;
	int Timeout = 0;
	int Alive = 0;
	int OK;
	int rt;

	int BootID=0;
	int ConfigID=0;
	int MaxVersion=1;
	unsigned short SearchPort = UPNP_PORT;

	char *IP;
	unsigned short PORT;
	char *PATH;
	int MATCH = 0;
	size_t len;

	struct LocationInfo *LI;
	struct sockaddr_in6 taddr;

	if (packet->Directive == NULL)
	{
		// M-SEARCH Response
		if (packet->StatusCode == 200)
		{
			node = packet->FirstField;
			while(node != NULL)
			{
				if (strncasecmp(node->Field, "BOOTID.UPNP.ORG", 15) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					BootID = atoi(node->FieldData);
				}
				else if (strncasecmp(node->Field, "CONFIGID.UPNP.ORG", 17) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					ConfigID = atoi(node->FieldData);
				}
				else if (strncasecmp(node->Field, "SEARCHPORT.UPNP.ORG", 19) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					SearchPort = (unsigned short)atoi(node->FieldData);
				}
				else if (strncasecmp(node->Field, "LOCATION", 8) == 0)
				{
					Location = node->FieldData;
					Location[node->FieldDataLength] = 0;
					//if ((Location = (char*)malloc(node->FieldDataLength + 1) == NULL) ILIBCRITICALEXIT(254);
					//memcpy(Location, node->FieldData, node->FieldDataLength);
					//Location[node->FieldDataLength] = '\0';
				}
				else if (strncasecmp(node->Field, "CACHE-CONTROL", 13) == 0)
				{
					pnode = IGD_ParseString(node->FieldData, 0, node->FieldDataLength, ", ", 1);
					prf = pnode->FirstResult;
					while(prf != NULL)
					{
						pnode2 = IGD_ParseString(prf->data, 0, prf->datalength, "=", 1);
						pnode2->FirstResult->datalength = IGD_TrimString(&(pnode2->FirstResult->data), pnode2->FirstResult->datalength);
						pnode2->FirstResult->data[pnode2->FirstResult->datalength]=0;
						if (strcasecmp(pnode2->FirstResult->data, "max-age") == 0)
						{
							pnode2->LastResult->datalength = IGD_TrimString(&(pnode2->LastResult->data), pnode2->LastResult->datalength);
							pnode2->LastResult->data[pnode2->LastResult->datalength]=0;
							Timeout = atoi(pnode2->LastResult->data);
							IGD_DestructParserResults(pnode2);
							break;
						}
						prf = prf->NextResult;
						IGD_DestructParserResults(pnode2);
					}
					IGD_DestructParserResults(pnode);
				}
				else if (strncasecmp(node->Field, "USN", 3) == 0)
				{
					pnode = IGD_ParseString(node->FieldData, 0, node->FieldDataLength, "::", 2);
					pnode->FirstResult->data[pnode->FirstResult->datalength] = '\0';
					UDN = pnode->FirstResult->data+5;
					IGD_DestructParserResults(pnode);
				}
				node = node->NextField;
			}
			IGD_ParseUri(Location, &IP, &PORT, &PATH, &taddr);
			if (taddr.sin6_family == AF_INET6) taddr.sin6_scope_id = remoteInterface->sin6_scope_id; // If this is IPv6, fill in the scope ID.

			//if (remoteInterface == inet_addr(IP)) // TODO: Perform IPv4 & IPv6 check
			{
				LI = (struct LocationInfo*)IGD_GetEntry(module->HashTable, Location, (int)strlen(Location));
				if (LI != NULL)
				{
					LI->IsInteresting = 1;
					LI->Timeout = Timeout;
				}
				else
				{
					if ((LI = (struct LocationInfo*)malloc(sizeof(struct LocationInfo))) == NULL) ILIBCRITICALEXIT(254);
					memset(LI, 0, sizeof(struct LocationInfo));
					
					// Lets create the official HTTP Address & Port struct.
					memcpy(&(LI->HttpAddr), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
					if (LI->HttpAddr.sin6_family == AF_INET6) LI->HttpAddr.sin6_port = PORT; else ((struct sockaddr_in*)&LI->HttpAddr)->sin_port = PORT;

					// Lets create the official Search Address & Port struct.
					memcpy(&(LI->SearchAddr), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
					if (LI->SearchAddr.sin6_family == AF_INET6) LI->SearchAddr.sin6_port = SearchPort; else ((struct sockaddr_in*)&LI->SearchAddr)->sin_port = SearchPort;

					LI->ConfigID = ConfigID;
					len = strlen(Location) + 1;
					if ((LI->LocationURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
					memcpy(LI->LocationURL, Location, len);
					LI->parent = module;
					LI->IsInteresting = 1;
					LI->Timeout = Timeout;
					len = strlen(UDN) + 1;
					if ((LI->UDN = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
					memcpy(LI->UDN, UDN, len);
					IGD_AddEntry(module->HashTable, Location, (int)strlen(Location), LI);
				}

				if (module->FunctionCallback != NULL)
				{
					module->FunctionCallback(module, UDN, -1, Location, (struct sockaddr*)remoteInterface, Timeout, UPnPSSDP_MSEARCH, module->Reserved);
				}
			}
			free(IP);
			free(PATH);
		}
	}
	else
	{
		// Notify Packet
		if (strncasecmp(packet->Directive, "NOTIFY", 6) == 0)
		{
			OK = 0;
			rt = 0;
			node = packet->FirstField;

			// Find MaxVersion first
			while(node != NULL)
			{
				node->Field[node->FieldLength] = '\0';
				if (strncasecmp(node->Field, "MAXVERSION.UPNP.ORG", 19) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					MaxVersion = atoi(node->FieldData);
				}
				node = node->NextField;
			}
			node = packet->FirstField;

			while(node != NULL)
			{
				node->Field[node->FieldLength] = '\0';
				if (strncasecmp(node->Field, "NT", 2) == 0 && node->FieldLength == 2)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					if (strncasecmp(node->FieldData, module->DeviceURN_Prefix, module->DeviceURN_PrefixLength) == 0)
					{

						if (atoi(node->FieldData+module->DeviceURN_PrefixLength)>=module->BaseDeviceVersionNumber || MaxVersion >= module->BaseDeviceVersionNumber)
						{
							OK = -1;
						}
					}

				}
				else if (strncasecmp(node->Field, "NTS", 3) == 0)
				{
					if (strncasecmp(node->FieldData, "ssdp:alive", 10) == 0)
					{
						Alive = -1;
						rt = 0;
					}
					else
					{
						Alive = 0;
						OK = 0;
					}
				}
				else if (strncasecmp(node->Field, "USN", 3) == 0)
				{
					pnode = IGD_ParseString(node->FieldData, 0, node->FieldDataLength, "::", 2);
					pnode->FirstResult->data[pnode->FirstResult->datalength] = '\0';
					UDN = pnode->FirstResult->data+5;
					IGD_DestructParserResults(pnode);
				}
				else if (strncasecmp(node->Field, "LOCATION", 8) == 0)
				{
					Location = node->FieldData;
					Location[node->FieldDataLength] = 0;
				}
				else if (strncasecmp(node->Field, "BOOTID.UPNP.ORG", 15) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					BootID = atoi(node->FieldData);
				}
				else if (strncasecmp(node->Field, "CONFIGID.UPNP.ORG", 17) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					ConfigID = atoi(node->FieldData);
				}
				else if (strncasecmp(node->Field, "SEARCHPORT.UPNP.ORG", 19) == 0)
				{
					node->FieldData[node->FieldDataLength] = '\0';
					SearchPort = (unsigned short)atoi(node->FieldData);
				}
				else if (strncasecmp(node->Field, "CACHE-CONTROL", 13) == 0)
				{
					pnode = IGD_ParseString(node->FieldData, 0, node->FieldDataLength, ", ", 1);
					prf = pnode->FirstResult;
					while(prf != NULL)
					{
						pnode2 = IGD_ParseString(prf->data, 0, prf->datalength, "=", 1);
						pnode2->FirstResult->datalength = IGD_TrimString(&(pnode2->FirstResult->data), pnode2->FirstResult->datalength);
						pnode2->FirstResult->data[pnode2->FirstResult->datalength]=0;
						if (strcasecmp(pnode2->FirstResult->data, "max-age") == 0)
						{
							pnode2->LastResult->datalength = IGD_TrimString(&(pnode2->LastResult->data), pnode2->LastResult->datalength);
							pnode2->LastResult->data[pnode2->LastResult->datalength]=0;
							Timeout = atoi(pnode2->LastResult->data);
							IGD_DestructParserResults(pnode2);
							break;
						}
						prf = prf->NextResult;
						IGD_DestructParserResults(pnode2);
					}
					IGD_DestructParserResults(pnode);					
				}
				node = node->NextField;
			}
			if ((OK != 0 && Alive != 0) || (Alive == 0)) 
			{
				if (Location != NULL)
				{
					IGD_ParseUri(Location, &IP, &PORT, &PATH, &taddr);
					if (taddr.sin6_family == AF_INET6) taddr.sin6_scope_id = remoteInterface->sin6_scope_id; // If this is IPv6, fill in the scope ID.

					//if (remoteInterface == inet_addr(IP)) // TODO: Perform IPv4 and IPv6 compare
					//{
						MATCH = 1;
					//}
					//else
					//{
					//	MATCH = 0;
					//}
					free(IP);
					free(PATH);

					if (IGD_HasEntry(module->HashTable, Location, (int)strlen(Location)) == 0)
					{
						if ((LI = (struct LocationInfo*)malloc(sizeof(struct LocationInfo))) == NULL) ILIBCRITICALEXIT(254);
						memset(LI, 0, sizeof(struct LocationInfo));
						
						// Lets create the official HTTP Address & Port struct.
						memcpy(&(LI->HttpAddr), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
						if (LI->HttpAddr.sin6_family == AF_INET6) LI->HttpAddr.sin6_port = PORT; else ((struct sockaddr_in*)&LI->HttpAddr)->sin_port = PORT;

						// Lets create the official Search Address & Port struct.
						memcpy(&(LI->SearchAddr), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
						if (LI->SearchAddr.sin6_family == AF_INET6) LI->SearchAddr.sin6_port = SearchPort; else ((struct sockaddr_in*)&LI->SearchAddr)->sin_port = SearchPort;

						LI->ConfigID = ConfigID;
						len = strlen(Location) + 1;
						if ((LI->LocationURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(LI->LocationURL, Location, len);
						LI->parent = module;
						LI->IsInteresting = 1;
						LI->Timeout = Timeout;
						len = strlen(UDN) + 1;
						if ((LI->UDN = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(LI->UDN, UDN, len);
						IGD_AddEntry(module->HashTable, Location, (int)strlen(Location), LI);
					}
					else
					{
						LI = (struct LocationInfo*)IGD_GetEntry(module->HashTable, Location, (int)strlen(Location));
						if (LI != NULL)
						{
							LI->IsInteresting = 1;
							LI->Timeout = Timeout;
							IGD_LifeTime_Remove(module->TIMER, LI);
						}
					}

				}
				if (Alive == 0 || MATCH != 0)
				{
					if (module->FunctionCallback != NULL)
					{
						if (Location != NULL)
						{
							IGD_ParseUri(Location, &IP, &PORT, &PATH, &taddr);
							if (taddr.sin6_family == AF_INET6) taddr.sin6_scope_id = remoteInterface->sin6_scope_id; // If this is IPv6, fill in the scope ID.
							free(IP);
							free(PATH);
						}
						module->FunctionCallback(module, UDN, Alive, Location, (struct sockaddr*)&taddr, Timeout, UPnPSSDP_NOTIFY, module->Reserved);
					}
				}
			}

			else if (Location != NULL)
			{
				IGD_ParseUri(Location, NULL, &PORT, NULL, &taddr);

				// Didn't match, lets Unicast search to make sure (1.1)
				if (IGD_HasEntry(module->HashTable, Location, (int)strlen(Location)) == 0)
				{
					if ((LI = (struct LocationInfo*)malloc(sizeof(struct LocationInfo))) == NULL) ILIBCRITICALEXIT(254);
					memset(LI, 0, sizeof(struct LocationInfo));
					
					// Lets create the official HTTP Address & Port struct.
					memcpy(&(LI->HttpAddr), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
					if (LI->HttpAddr.sin6_family == AF_INET6) LI->HttpAddr.sin6_port = PORT; else ((struct sockaddr_in*)&LI->HttpAddr)->sin_port = PORT;

					// Lets create the official Search Address & Port struct.
					memcpy(&(LI->SearchAddr), remoteInterface, INET_SOCKADDR_LENGTH(remoteInterface->sin6_family));
					if (LI->SearchAddr.sin6_family == AF_INET6) LI->SearchAddr.sin6_port = SearchPort; else ((struct sockaddr_in*)&LI->SearchAddr)->sin_port = SearchPort;

					LI->ConfigID = ConfigID;
					len = strlen(Location) + 1;
					if ((LI->LocationURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
					memcpy(LI->LocationURL, Location, len);
					LI->parent = module;
					LI->IsInteresting = 0;
					LI->Timeout = Timeout;

					IGD_AddEntry(module->HashTable, Location, (int)strlen(Location), LI);
					IGD_LifeTime_Add(module->TIMER, LI, 10, &IGD_SSDP_LocationSink, NULL);
				}
				else
				{
					// There is a match, is this an interesting device?
					LI = (struct LocationInfo*)IGD_GetEntry(module->HashTable, Location, (int)strlen(Location));
					if (LI != NULL)
					{
						if (LI->IsInteresting != 0)
						{
							if (module->FunctionCallback != NULL)
							{
								module->FunctionCallback(module, UDN, Alive, Location, (struct sockaddr*)&(LI->HttpAddr), Timeout, UPnPSSDP_MSEARCH, module->Reserved);
							}
						}
					}
				}
			}

		}
	}
}

void IGD_SSDPClient_OnData(IGD_AsyncUDPSocket_SocketModule socketModule, char* buffer, int bufferLength, struct sockaddr_in6 *remoteInterface, void *user, void *user2, int *PAUSE)
{
	struct packetheader *packet;

	UNREFERENCED_PARAMETER( socketModule );
	UNREFERENCED_PARAMETER( user2 );
	UNREFERENCED_PARAMETER( PAUSE );

	packet = IGD_ParsePacketHeader(buffer, 0, bufferLength);
	if (packet == NULL) {return;}

	IGD_ReadSSDP(packet, remoteInterface, (struct SSDPClientModule*)user);
	IGD_DestructPacket(packet);
}

void IGD_SSDPClientModule_Destroy(void *object)
{
	char *key;
	int keyLength;
	void *data;
	struct SSDPClientModule *s = (struct SSDPClientModule*)object;
	void *en = IGD_HashTree_GetEnumerator(s->HashTable);
	
	while(IGD_HashTree_MoveNext(en) == 0)
	{
		IGD_HashTree_GetValue(en, &key, &keyLength, &data);
		free(((struct LocationInfo*)data)->LocationURL);
		if (((struct LocationInfo*)data)->UDN != NULL) free(((struct LocationInfo*)data)->UDN);
		free(data);
	}

	IGD_HashTree_DestroyEnumerator(en);
	IGD_DestroyHashTree(s->HashTable);

	free(s->DeviceURN);
	if (s->IPAddressListV4 != NULL) { free(s->IPAddressListV4); s->IPAddressListV4 = NULL; }
	if (s->IPAddressListV6 != NULL) { free(s->IPAddressListV6); s->IPAddressListV6 = NULL; }
}

void IGD_SSDP_IPAddressListChanged(void *SSDPToken)
{
	int i;
	int bufferlength, bufferlength2;
	struct SSDPClientModule *RetVal = (struct SSDPClientModule*)SSDPToken;

	// Free the local IP address lists
	if (RetVal->IPAddressListV4 != NULL) { free(RetVal->IPAddressListV4); RetVal->IPAddressListV4 = NULL; }
	if (RetVal->IPAddressListV6 != NULL) { free(RetVal->IPAddressListV6); RetVal->IPAddressListV6 = NULL; }

	// Create a new list of local IP addresses
	RetVal->IPAddressListV4Length = IGD_GetLocalIPv4AddressList(&(RetVal->IPAddressListV4), 1);
	RetVal->IPAddressListV6Length = IGD_GetLocalIPv6List(&(RetVal->IPAddressListV6));

	// Join the SSDP listening socket to the multicast group on all local interfaces
	for (i = 0; i < RetVal->IPAddressListV4Length; ++i) IGD_AsyncUDPSocket_JoinMulticastGroupV4(RetVal->SSDPListenSocket4, (struct sockaddr_in*)&(RetVal->MulticastAddrV4), (struct sockaddr*)&(RetVal->IPAddressListV4[i]));
	if (RetVal->SSDPListenSocket6 != NULL) 
	{
		for (i = 0; i < RetVal->IPAddressListV6Length; ++i)
		{
			if (IGD_AsyncSocket_IsIPv6LinkLocal((struct sockaddr*)&(RetVal->IPAddressListV6[i])))
			{
				IGD_AsyncUDPSocket_JoinMulticastGroupV6(RetVal->SSDPListenSocket6, (struct sockaddr_in6*)&(RetVal->MulticastAddrV6LL), RetVal->IPAddressListV6[i].sin6_scope_id);
			}
			else
			{
				IGD_AsyncUDPSocket_JoinMulticastGroupV6(RetVal->SSDPListenSocket6, (struct sockaddr_in6*)&(RetVal->MulticastAddrV6SL), RetVal->IPAddressListV6[i].sin6_scope_id);
			}
		}
	}

	// Create an IPv4 search packet & send it on all interfaces
	bufferlength = snprintf(IGD_ScratchPad, sizeof(IGD_ScratchPad), "M-SEARCH * HTTP/1.1\r\nMX: 3\r\nST: %s\r\nHOST: %s:1900\r\nMAN: \"ssdp:discover\"\r\n\r\n", 
		RetVal->DeviceURN, UPNP_MCASTv4_GROUP);
	for (i = 0; i < RetVal->IPAddressListV4Length; ++i)
	{
#if defined(_WIN32_WCE) && _WIN32_WCE < 400
		IGD_AsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket4, RetVal->MulticastAddrV4, UPNP_PORT, IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
#else
		IGD_AsyncUDPSocket_SetMulticastInterface(RetVal->MSEARCH_Response_Socket4, (struct sockaddr*)&(RetVal->IPAddressListV4[i]));
		IGD_AsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket4, (struct sockaddr*)&(RetVal->MulticastAddrV4), IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
#endif
	}

	if (RetVal->MSEARCH_Response_Socket6 != NULL) 
	{
		// Create an IPv6 search packet & send it on all interfaces
		bufferlength = snprintf(IGD_ScratchPad, sizeof(IGD_ScratchPad), "M-SEARCH * HTTP/1.1\r\nMX: 3\r\nST: %s\r\nHOST: %s:1900\r\nMAN: \"ssdp:discover\"\r\n\r\n", RetVal->DeviceURN, UPNP_MCASTv6_GROUPB);
		bufferlength2 = snprintf(IGD_ScratchPad2, sizeof(IGD_ScratchPad), "M-SEARCH * HTTP/1.1\r\nMX: 3\r\nST: %s\r\nHOST: %s:1900\r\nMAN: \"ssdp:discover\"\r\n\r\n", RetVal->DeviceURN, UPNP_MCASTv6_LINK_GROUPB);

		for (i = 0; i < RetVal->IPAddressListV6Length; ++i)
		{
			if (IGD_AsyncSocket_IsIPv6LinkLocal((struct sockaddr*)&(RetVal->IPAddressListV6[i])))
			{
#if defined(_WIN32_WCE) && _WIN32_WCE < 400
				IGD_AsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket6, RetVal->MulticastAddrV6, UPNP_PORT, IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
#else
				IGD_AsyncUDPSocket_SetMulticastInterface(RetVal->MSEARCH_Response_Socket6, (struct sockaddr*)&(RetVal->IPAddressListV6[i]));
				IGD_AsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket6, (struct sockaddr*)&(RetVal->MulticastAddrV6LL), IGD_ScratchPad2, bufferlength2, IGD_AsyncSocket_MemoryOwnership_USER);
#endif
			}
			else
			{
#if defined(_WIN32_WCE) && _WIN32_WCE < 400
				IGD_AsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket6, RetVal->MulticastAddrV6, UPNP_PORT, IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
#else
				IGD_AsyncUDPSocket_SetMulticastInterface(RetVal->MSEARCH_Response_Socket6, (struct sockaddr*)&(RetVal->IPAddressListV6[i]));
				IGD_AsyncUDPSocket_SendTo(RetVal->MSEARCH_Response_Socket6, (struct sockaddr*)&(RetVal->MulticastAddrV6SL), IGD_ScratchPad, bufferlength, IGD_AsyncSocket_MemoryOwnership_USER);
#endif   
			}
		}
	}
}


void IGD_SSDPClientModule_PreSelect(void* object, void *readset, void *writeset, void *errorset, int* blocktime)
{
	struct SSDPClientModule *s = (struct SSDPClientModule*)object;

	UNREFERENCED_PARAMETER( readset );
	UNREFERENCED_PARAMETER( writeset );
	UNREFERENCED_PARAMETER( errorset );
	UNREFERENCED_PARAMETER( blocktime );

	s->PreSelect = NULL;
	IGD_SSDP_IPAddressListChanged(object);
}

void* IGD_CreateSSDPClientModule(void *chain, char* DeviceURN, int DeviceURNLength, void (*CallbackPtr)(void *sender, char* UDN, int Alive, char* LocationURL, struct sockaddr* LocationAddr, int Timeout, UPnPSSDP_MESSAGE m, void *user), void *user)
{
	struct SSDPClientModule *RetVal;
	int TTL = 4;
	struct parser_result *pr;
	struct sockaddr_in local4;
	struct sockaddr_in6 local6;

	// Create the master state structure and clear it.
	if ((RetVal = (struct SSDPClientModule*)malloc(sizeof(struct SSDPClientModule))) == NULL) ILIBCRITICALEXIT(254);
	memset(RetVal, 0, sizeof(struct SSDPClientModule));

	// Initialize the structure
	RetVal->Destroy = &IGD_SSDPClientModule_Destroy;
	RetVal->PreSelect = &IGD_SSDPClientModule_PreSelect;
	RetVal->PostSelect = NULL;
	RetVal->Reserved = user;
	RetVal->Terminate = 0;
	RetVal->FunctionCallback = CallbackPtr;
	if ((RetVal->DeviceURN = (char*)malloc(DeviceURNLength + 1)) == NULL) ILIBCRITICALEXIT(254);
	memcpy(RetVal->DeviceURN, DeviceURN, DeviceURNLength);
	RetVal->DeviceURN[DeviceURNLength] = '\0';
	RetVal->DeviceURNLength = DeviceURNLength;

	// Populate the Prefix portion of the URN, for matching purposes
	RetVal->DeviceURN_Prefix = RetVal->DeviceURN;
	pr = IGD_ParseString(RetVal->DeviceURN, 0, RetVal->DeviceURNLength, ":", 1);
	RetVal->DeviceURN_PrefixLength = (int)((pr->LastResult->data)-(RetVal->DeviceURN));
	pr->LastResult->data[pr->LastResult->datalength] = 0;
	RetVal->BaseDeviceVersionNumber = atoi(pr->LastResult->data);
	IGD_DestructParserResults(pr);

	// Setup multicast IPv4 address, setup IPv6 if supported
	RetVal->MulticastAddrV4.sin_family = AF_INET;
	RetVal->MulticastAddrV4.sin_port = htons(UPNP_PORT);
	IGD_Inet_pton(AF_INET, UPNP_MCASTv4_GROUP, &(RetVal->MulticastAddrV4.sin_addr));

	// Setup the local IPv4 address
	memset(&local4, 0, sizeof(struct sockaddr_in));
	local4.sin_family = AF_INET;
	local4.sin_port = htons(UPNP_PORT);

	// Create sockets
	RetVal->SSDPListenSocket4 = IGD_AsyncUDPSocket_CreateEx(chain, 4096, (struct sockaddr*)&local4, IGD_AsyncUDPSocket_Reuse_SHARED, IGD_SSDPClient_OnData, NULL, RetVal);
	IGD_AsyncUDPSocket_SetMulticastLoopback(RetVal->SSDPListenSocket4, 1);
	local4.sin_port = 0;
	RetVal->MSEARCH_Response_Socket4 = IGD_AsyncUDPSocket_CreateEx(chain, 4096, (struct sockaddr*)&local4, IGD_AsyncUDPSocket_Reuse_EXCLUSIVE, IGD_SSDPClient_OnData, NULL, RetVal);
	IGD_AsyncUDPSocket_SetMulticastTTL(RetVal->MSEARCH_Response_Socket4, TTL);
	RetVal->UNICAST_Socket4 = IGD_AsyncUDPSocket_CreateEx(chain, 4096, (struct sockaddr*)&local4, IGD_AsyncUDPSocket_Reuse_EXCLUSIVE, IGD_SSDPClient_OnData, NULL, RetVal);
	IGD_AsyncUDPSocket_SetMulticastTTL(RetVal->UNICAST_Socket4, TTL);
	IGD_AsyncUDPSocket_SetMulticastLoopback(RetVal->UNICAST_Socket4, 1);

	// If IPv6 is detected, lets perform more setup.
	if (IGD_DetectIPv6Support())
	{
		// Setup multicast IPv6 address (Site Local)
		RetVal->MulticastAddrV6SL.sin6_family = AF_INET6;
		RetVal->MulticastAddrV6SL.sin6_port = htons(UPNP_PORT);
		IGD_Inet_pton(AF_INET6, UPNP_MCASTv6_GROUP, &(RetVal->MulticastAddrV6SL.sin6_addr));

		// Setup multicast IPv6 address (Link Local)
		RetVal->MulticastAddrV6LL.sin6_family = AF_INET6;
		RetVal->MulticastAddrV6LL.sin6_port = htons(UPNP_PORT);
		IGD_Inet_pton(AF_INET6, UPNP_MCASTv6_LINK_GROUP, &(RetVal->MulticastAddrV6LL.sin6_addr));

		// Setup IPv6
		memset(&local6, 0, sizeof(struct sockaddr_in6));
		local6.sin6_family = AF_INET6;
		local6.sin6_port = htons(UPNP_PORT);

		RetVal->SSDPListenSocket6 = IGD_AsyncUDPSocket_CreateEx(chain, 4096, (struct sockaddr*)&local6, IGD_AsyncUDPSocket_Reuse_SHARED, IGD_SSDPClient_OnData, NULL, RetVal);
		IGD_AsyncUDPSocket_SetMulticastLoopback(RetVal->SSDPListenSocket6, 1);
		local6.sin6_port = 0;
		RetVal->MSEARCH_Response_Socket6 = IGD_AsyncUDPSocket_CreateEx(chain, 4096, (struct sockaddr*)&local6, IGD_AsyncUDPSocket_Reuse_EXCLUSIVE, IGD_SSDPClient_OnData, NULL, RetVal);
		IGD_AsyncUDPSocket_SetMulticastTTL(RetVal->MSEARCH_Response_Socket6, TTL);
		IGD_AsyncUDPSocket_SetMulticastLoopback(RetVal->MSEARCH_Response_Socket6, 1);
		RetVal->UNICAST_Socket6 = IGD_AsyncUDPSocket_CreateEx(chain, 4096, (struct sockaddr*)&local6, IGD_AsyncUDPSocket_Reuse_EXCLUSIVE, IGD_SSDPClient_OnData, NULL, RetVal);
		IGD_AsyncUDPSocket_SetMulticastLoopback(RetVal->UNICAST_Socket6, 1);
	}

	IGD_AddToChain(chain, RetVal);

	RetVal->HashTable = IGD_InitHashTree();
	RetVal->TIMER = IGD_CreateLifeTime(chain);

	return RetVal;
}
