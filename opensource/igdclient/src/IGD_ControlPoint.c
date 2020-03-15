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


#if defined(WIN32) || defined(_WIN32_WCE)
#	ifndef MICROSTACK_NO_STDAFX
#		include "stdafx.h"
#	endif
char* IGD_PLATFORM = "WINDOWS UPnP/1.0 MicroStack/1.0.5329";
#elif defined(__SYMBIAN32__)
char* IGD_PLATFORM = "SYMBIAN UPnP/1.0 MicroStack/1.0.5329";
#else
char* IGD_PLATFORM = "POSIX UPnP/1.0 MicroStack/1.0.5329";
#endif



#include "IGD_Parsers.h"
#include "IGD_SSDPClient.h"
#include "IGD_WebServer.h"
#include "IGD_WebClient.h"
#include "IGD_AsyncSocket.h"
#include "IGD_ControlPoint.h"
#include "IGD_Client.h"

#if defined(WIN32) && !defined(_WIN32_WCE)
#include <crtdbg.h>
#endif

#define UPNP_PORT 1900
#define UPNP_MCASTv4_GROUP "239.255.255.250"
#define UPNP_MCASTv6_GROUP "FF05:0:0:0:0:0:0:C"
#define UPNP_MCASTv6_GROUPB "[FF05:0:0:0:0:0:0:C]"
#define IGD_MIN(a, b) (((a)<(b))?(a):(b))

#define IGD_InvocationPriorityLevel IGD_AsyncSocket_QOS_CONTROL

#define INVALID_DATA 0
#define DEBUGSTATEMENT(x)
#define LVL3DEBUG(x)
#define INET_SOCKADDR_LENGTH(x) ((x == AF_INET6?sizeof(struct sockaddr_in6):sizeof(struct sockaddr_in)))

static const char *UPNPCP_SOAP_Header = "POST %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: %s\r\nSOAPACTION: \"%s#%s\"\r\nContent-Type: text/xml; charset=\"utf-8\"\r\nContent-Length: %d\r\n\r\n";
static const char *UPNPCP_SOAP_BodyHead = "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"><s:Body><u:";
static const char *UPNPCP_SOAP_BodyTail = "></s:Body></s:Envelope>";

void IGD_Renew(void *state);
void IGD_SSDP_Sink(void *sender, char* UDN, int Alive, char* LocationURL, struct sockaddr* LocationAddr, int Timeout, UPnPSSDP_MESSAGE m, void *cp);

struct CustomUserData
{
	int Timeout;
	char* buffer;
	char *UDN;
	char *LocationURL;
	struct sockaddr_in6 LocationAddr;
};

struct IGD_CP
{
	void (*PreSelect)(void* object, fd_set *readset, fd_set *writeset, fd_set *errorset, int* blocktime);
	void (*PostSelect)(void* object, int slct, fd_set *readset, fd_set *writeset, fd_set *errorset);
	void (*Destroy)(void* object);
	void (*DiscoverSink)(struct UPnPDevice *device);
	void (*RemoveSink)(struct UPnPDevice *device);
	
	// ToDo: Check to see if this is necessary
	void (*EventCallback_DimmingService_LoadLevelStatus)(struct UPnPService* Service, unsigned char value);
	void (*EventCallback_SwitchPower_Status)(struct UPnPService* Service, int value);
	
	struct LifeTimeMonitorStruct *LifeTimeMonitor;
	
	void *HTTP;
	void *SSDP;
	void *WebServer;
	void *User;
	
	sem_t DeviceLock;
	void* SIDTable;
	void* DeviceTable_UDN;
	void* DeviceTable_Tokens;
	void* DeviceTable_URI;
	
	void *Chain;
	int RecheckFlag;
	
	struct sockaddr_in *IPAddressListV4;
	int IPAddressListV4Length;
	struct sockaddr_in6 *IPAddressListV6;
	int IPAddressListV6Length;
	
	UPnPDeviceDiscoveryErrorHandler ErrorDispatch;
};

IGD_EventCallback_DiscoverSink					IGD_EventCallback_WANIPConnectionDevice_DiscoverSink = NULL;
IGD_EventCallback_RemoveSink					IGD_EventCallback_WANIPConnectionDevice_RemoveSink = NULL;

IGD_EventCallback_PortMappingNumberOfEntries	IGD_EventCallback_WANIPConnection_PortMappingNumberOfEntries = NULL;
IGD_EventCallback_ExternalIPAddress				IGD_EventCallback_WANIPConnection_ExternalIPAddress = NULL;
IGD_EventCallback_ConnectionStatus				IGD_EventCallback_WANIPConnection_ConnectionStatus = NULL;
IGD_EventCallback_PossibleConnectionTypes		IGD_EventCallback_WANIPConnection_PossibleConnectionTypes = NULL;

struct InvokeStruct
{
	struct UPnPService *Service;
	voidfp CallbackPtr;
	void *User;
};

struct UPnPServiceInfo
{
	char* serviceType;
	char* SCPDURL;
	char* controlURL;
	char* eventSubURL;
	char* serviceId;
	struct UPnPServiceInfo *Next;
};

struct IGD_Stack
{
	void *data;
	struct IGD_Stack *next;
};

void IGD_SetUser(void *token, void *user)
{
	((struct IGD_CP*)token)->User = user;
}

void* IGD_GetUser(void *token)
{
	return(((struct IGD_CP*)token)->User);
}

void IGD_CP_ProcessDeviceRemoval(struct IGD_CP* CP, struct UPnPDevice *device);

void IGD_DestructUPnPService(struct UPnPService *service)
{
	struct UPnPAction *a1, *a2;
	struct UPnPStateVariable *sv1, *sv2;
	int i;
	
	a1 = service->Actions;
	while (a1 != NULL)
	{
		a2 = a1->Next;
		free(a1->Name);
		free(a1);
		a1 = a2;
	}
	
	sv1 = service->Variables;
	while (sv1 != NULL)
	{
		sv2 = sv1->Next;
		free(sv1->Name);
		if (sv1->Min != NULL) {free(sv1->Min);}
		if (sv1->Max != NULL) {free(sv1->Max);}
		if (sv1->Step != NULL) {free(sv1->Step);}
		for(i=0;i<sv1->NumAllowedValues;++i) free(sv1->AllowedValues[i]);
		if (sv1->AllowedValues != NULL) {free(sv1->AllowedValues);}
		free(sv1);
		sv1 = sv2;
	}
	if (service->ControlURL != NULL) {free(service->ControlURL);}
	if (service->SCPDURL != NULL) {free(service->SCPDURL);}
	if (service->ServiceId != NULL) {free(service->ServiceId);}
	if (service->ServiceType != NULL) {free(service->ServiceType);}
	if (service->SubscriptionURL != NULL) {free(service->SubscriptionURL);}
	if (service->SubscriptionID != NULL)
	{
		IGD_LifeTime_Remove(((struct IGD_CP*)service->Parent->CP)->LifeTimeMonitor, service);
		IGD_DeleteEntry(((struct IGD_CP*)service->Parent->CP)->SIDTable, service->SubscriptionID, (int)strlen(service->SubscriptionID));
		free(service->SubscriptionID);
		service->SubscriptionID = NULL;
	}
	
	free(service);
}
void IGD_DestructUPnPDevice(struct UPnPDevice *device)
{
	struct UPnPDevice *d1, *d2;
	struct UPnPService *s1, *s2;
	int iconIndex;
	
	d1 = device->EmbeddedDevices;
	while (d1 != NULL)
	{
		d2 = d1->Next;
		IGD_DestructUPnPDevice(d1);
		d1 = d2;
	}
	
	s1 = device->Services;
	while (s1 != NULL)
	{
		s2 = s1->Next;
		IGD_DestructUPnPService(s1);
		s1 = s2;
	}
	
	LVL3DEBUG(printf("\r\n\r\nDevice Destructed\r\n");)
	if (device->PresentationURL != NULL) {free(device->PresentationURL);}
	if (device->ManufacturerName != NULL) {free(device->ManufacturerName);}
	if (device->ManufacturerURL != NULL) {free(device->ManufacturerURL);}
	if (device->ModelName != NULL) {free(device->ModelName);}
	if (device->ModelNumber != NULL) {free(device->ModelNumber);}
	if (device->ModelURL != NULL) {free(device->ModelURL);}
	if (device->ModelDescription != NULL) {free(device->ModelDescription);}
	if (device->DeviceType != NULL) {free(device->DeviceType);}
	if (device->FriendlyName != NULL) {free(device->FriendlyName);}
	if (device->LocationURL != NULL) {free(device->LocationURL);}
	if (device->UDN != NULL) {free(device->UDN);}
	if (device->InterfaceToHost != NULL) {free(device->InterfaceToHost);}
	if (device->Reserved3 != NULL) {free(device->Reserved3);}
	if (device->IconsLength>0)
	{
		for(iconIndex=0;iconIndex<device->IconsLength;++iconIndex)
		{
			if (device->Icons[iconIndex].mimeType != NULL){free(device->Icons[iconIndex].mimeType);}
			if (device->Icons[iconIndex].url != NULL){free(device->Icons[iconIndex].url);}
		}
		free(device->Icons);
	}
	
	
	free(device);
}


/*! \fn IGD_AddRef(struct UPnPDevice *device)
\brief Increments the reference counter for a UPnP device
\param device UPnP device
*/
void IGD_AddRef(struct UPnPDevice *device)
{
	struct IGD_CP *CP = (struct IGD_CP*)device->CP;
	struct UPnPDevice *d = device;
	sem_wait(&(CP->DeviceLock));
	while (d->Parent != NULL) {d = d->Parent;}
	++d->ReferenceCount;
	sem_post(&(CP->DeviceLock));
}

void IGD_CheckfpDestroy(struct UPnPDevice *device)
{
	struct UPnPDevice *ed = device->EmbeddedDevices;
	if (device->fpDestroy != NULL) {device->fpDestroy(device);}
	while (ed != NULL)
	{
		IGD_CheckfpDestroy(ed);
		ed = ed->Next;
	}
}
/*! \fn IGD_Release(struct UPnPDevice *device)
\brief Decrements the reference counter for a UPnP device.
\para Device will be disposed when the counter becomes zero.
\param device UPnP device
*/
void IGD_Release(struct UPnPDevice *device)
{
	struct IGD_CP *CP = (struct IGD_CP*)device->CP;
	struct UPnPDevice *d = device;
	sem_wait(&(CP->DeviceLock));
	while (d->Parent != NULL) {d = d->Parent;}
	--d->ReferenceCount;
	if (d->ReferenceCount<=0)
	{
		IGD_CheckfpDestroy(device);
		IGD_DestructUPnPDevice(d);
	}
	sem_post(&(CP->DeviceLock));
}

//void UPnPDeviceDescriptionInterruptSink(void *sender, void *user1, void *user2)
//{
	//	struct CustomUserData *cd = (struct CustomUserData*)user1;
	//	free(cd->buffer);
	//	free(user1);
	//}

int IGD_IsLegacyDevice(struct packetheader *header)
{
	struct packetheader_field_node *f;
	struct parser_result_field *prf;
	struct parser_result *r, *r2;
	int Legacy = 1;
	
	// Check version of Device
	f = header->FirstField;
	while (f != NULL)
	{
		if (f->FieldLength == 6 && strncasecmp(f->Field, "SERVER", 6) == 0)
		{
			// Check UPnP version of the Control Point which invoked us
			r = IGD_ParseString(f->FieldData, 0, f->FieldDataLength, " ", 1);
			prf = r->FirstResult;
			while (prf != NULL)
			{
				if (prf->datalength > 5 && memcmp(prf->data, "UPnP/", 5) == 0)
				{
					r2 = IGD_ParseString(prf->data+5, 0, prf->datalength-5, ".", 1);
					r2->FirstResult->data[r2->FirstResult->datalength] = 0;
					r2->LastResult->data[r2->LastResult->datalength] = 0;
					if (atoi(r2->FirstResult->data) == 1 && atoi(r2->LastResult->data) > 0)
					{
						Legacy = 0;
					}
					IGD_DestructParserResults(r2);
				}
				prf = prf->NextResult;
			}
			IGD_DestructParserResults(r);
		}
		if (Legacy)
		{
			f = f->NextField;
		}
		else
		{
			break;
		}
	}
	return Legacy;
}
void IGD_Push(struct IGD_Stack **pp_Top, void *data)
{
	struct IGD_Stack *frame;
	if ((frame = (struct IGD_Stack*)malloc(sizeof(struct IGD_Stack))) == NULL) ILIBCRITICALEXIT(254);
	frame->data = data;
	frame->next = *pp_Top;
	*pp_Top = frame;
}

void *IGD_Pop(struct IGD_Stack **pp_Top)
{
	struct IGD_Stack *frame = *pp_Top;
	void *RetVal = NULL;
	
	if (frame != NULL)
	{
		*pp_Top = frame->next;
		RetVal = frame->data;
		free(frame);
	}
	return RetVal;
}

void *IGD_Peek(struct IGD_Stack **pp_Top)
{
	struct IGD_Stack *frame = *pp_Top;
	void *RetVal = NULL;
	
	if (frame != NULL)
	{
		RetVal = (*pp_Top)->data;
	}
	return RetVal;
}

void IGD_Flush(struct IGD_Stack **pp_Top)
{
	while (IGD_Pop(pp_Top) != NULL) {}
	*pp_Top = NULL;
}

/*! \fn IGD_GetDeviceAtUDN(void *v_CP, char* UDN)
\brief Fetches a device with a particular UDN
\param v_CP Control Point Token
\param UDN Unique Device Name
\returns UPnP device
*/
struct UPnPDevice* IGD_GetDeviceAtUDN(void *v_CP, char* UDN)
{
	struct UPnPDevice *RetVal = NULL;
	struct IGD_CP* CP = (struct IGD_CP*)v_CP;
	char uuid[1000]={0};

	if (v_CP == NULL || UDN == NULL)
	{
		printf("[%s:%d] Invalid args\n", __FUNCTION__, __LINE__);
		return NULL;
	}

	if (strcasestr("uuid:", UDN) != NULL)
	{
		sscanf(UDN, "uuid:%999[^\n]",  uuid);
	}
	else
	{
		sprintf(uuid, UDN);
	}
	
	IGD_HashTree_Lock(CP->DeviceTable_UDN);
	RetVal = (struct UPnPDevice*)IGD_GetEntry(CP->DeviceTable_UDN, uuid, (int)strlen(uuid));
	IGD_HashTree_UnLock(CP->DeviceTable_UDN);

	return(RetVal);
}
struct packetheader *IGD_BuildPacket(char* IP, int Port, char* Path, char* cmd)
{
	char* HostLine;
	int HostLineLength = (int)strlen(IP) + 7;
	struct packetheader *RetVal = IGD_CreateEmptyPacket();
	if ((HostLine = (char*)malloc(HostLineLength)) == NULL) ILIBCRITICALEXIT(254);
	HostLineLength = snprintf(HostLine, HostLineLength, "%s:%d", IP, Port);
	
	IGD_SetVersion(RetVal, "1.1", 3);
	IGD_SetDirective(RetVal, cmd, (int)strlen(cmd), Path, (int)strlen(Path));
	IGD_AddHeaderLine(RetVal, "Host", 4, HostLine, HostLineLength);
	IGD_AddHeaderLine(RetVal, "User-Agent", 10, IGD_PLATFORM, (int)strlen(IGD_PLATFORM));
	free(HostLine);
	return(RetVal);
}

void IGD_RemoveServiceFromDevice(struct UPnPDevice *device, struct UPnPService *service)
{
	struct UPnPService *s = device->Services;
	
	if (s == service)
	{
		device->Services = s->Next;
		IGD_DestructUPnPService(service);
		return;
	}
	while (s->Next != NULL)
	{
		if (s->Next == service)
		{
			s->Next = s->Next->Next;
			IGD_DestructUPnPService(service);
			return;
		}
		s = s->Next;
	}
}

void IGD_ProcessDevice(void *v_cp, struct UPnPDevice *device)
{
	struct IGD_CP* cp = (struct IGD_CP*)v_cp;
	struct UPnPDevice *EmbeddedDevice = device->EmbeddedDevices;
	size_t len;
	
	// Copy the LocationURL if necessary
	if (device->LocationURL == NULL && device->Parent != NULL && device->Parent->LocationURL != NULL)
	{
		len = strlen(device->Parent->LocationURL) + 1;
		if ((device->LocationURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
		memcpy(device->LocationURL, device->Parent->LocationURL, len);
	}
	while (EmbeddedDevice != NULL)
	{
		IGD_ProcessDevice(v_cp, EmbeddedDevice);
		EmbeddedDevice = EmbeddedDevice->Next;
	}
	
	// Create a table entry for each Unique Device Name, for easy mapping
	// of ssdp:byebye packets. This way any byebye packet from the device
	// heirarchy will result in the device being removed.
	
	IGD_HashTree_Lock(cp->DeviceTable_UDN);
	IGD_AddEntry(cp->DeviceTable_UDN, device->UDN, (int)strlen(device->UDN), device);
	IGD_HashTree_UnLock(cp->DeviceTable_UDN);
}

/*! fn IGD_PrintUPnPDevice(int indents, struct UPnPDevice *device)
\brief Debug method that displays the device object structure
\param indents The number of spaces to indent each sub-entry
\param device The device to display
*/
void IGD_PrintUPnPDevice(int indents, struct UPnPDevice *device)
{
	struct UPnPService *s;
	struct UPnPDevice *d;
	struct UPnPAction *a;
	int x = 0;
	
	for(x=0; x<indents; ++x) { printf(" "); }
	printf("Device: %s\r\n", device->DeviceType);
	
	for(x=0; x<indents; ++x) { printf(" "); }
	printf("Friendly: %s\r\n", device->FriendlyName);
	
	s = device->Services;
	while (s != NULL)
	{
		for(x=0; x<indents; ++x) { printf(" "); }
		printf("   Service: %s\r\n", s->ServiceType);
		a = s->Actions;
		while (a != NULL)
		{
			for(x=0;x<indents;++x) { printf(" "); }
			printf("      Action: %s\r\n", a->Name);
			a = a->Next;
		}
		s = s->Next;
	}
	
	d = device->EmbeddedDevices;
	while (d != NULL)
	{
		IGD_PrintUPnPDevice(indents+5, d);
		d = d->Next;
	}
}

/*! \fn IGD_GetService(struct UPnPDevice *device, char* ServiceName, int length)
\brief Obtains a specific UPnP service instance of appropriate version, from within a device
\para
This method returns services who's version is greater than or equal to that specified within \a ServiceName
\param device UPnP device
\param ServiceName Service Type
\param length Length of \a ServiceName
\returns UPnP service
*/
struct UPnPService *IGD_GetService(struct UPnPDevice *device, char* ServiceName, int length)
{
	struct UPnPService *RetService = NULL;
	struct UPnPService *s = device->Services;
	struct parser_result *pr, *pr2;
	
	pr = IGD_ParseString(ServiceName, 0, length, ":", 1);
	while (s != NULL)
	{
		pr2 = IGD_ParseString(s->ServiceType, 0, (int)strlen(s->ServiceType), ":", 1);
		if (length-pr->LastResult->datalength >= (int)strlen(s->ServiceType)-pr2->LastResult->datalength && memcmp(ServiceName, s->ServiceType, length-pr->LastResult->datalength) == 0)
		{
			if (atoi(pr->LastResult->data) <= atoi(pr2->LastResult->data))
			{
				RetService = s;
				IGD_DestructParserResults(pr2);
				break;
			}
		}
		IGD_DestructParserResults(pr2);
		s = s->Next;
	}
	IGD_DestructParserResults(pr);
	
	return RetService;
}

/*! \fn IGD_GetService_WANIPConnection(struct UPnPDevice *device)
\brief Returns the WANIPConnection service from the specified device	\par
Service Type = urn:schemas-upnp-org:service:WANIPConnection<br>
Version >= 1
\param device The device object to query
\returns A pointer to the service object
*/
struct UPnPService *IGD_GetService_WANIPConnection(struct UPnPDevice *device)
{
	return(IGD_GetService(device,"urn:schemas-upnp-org:service:WANIPConnection:1",46));
}


struct UPnPDevice *IGD_GetDevice2(struct UPnPDevice *device, int index, int *c_Index)
{
	struct UPnPDevice *RetVal = NULL;
	struct UPnPDevice *e_Device = NULL;
	int currentIndex = *c_Index;
	
	if (strncmp(device->DeviceType,"urn:schemas-upnp-org:device:WANConnectionDevice:",48)==0 && atoi(device->DeviceType+48)>=1)
	
	{
		++currentIndex;
		if (currentIndex == index)
		{
			*c_Index = currentIndex;
			return(device);
		}
	}
	
	e_Device = device->EmbeddedDevices;
	while (e_Device != NULL)
	{
		RetVal = IGD_GetDevice2(e_Device, index, &currentIndex);
		if (RetVal != NULL) break;
		e_Device = e_Device->Next;
	}
	
	*c_Index = currentIndex;
	return RetVal;
}

struct UPnPDevice* IGD_GetDevice1(struct UPnPDevice *device, int index)
{
	int c_Index = -1;
	return IGD_GetDevice2(device, index, &c_Index);
}

int IGD_GetDeviceCount(struct UPnPDevice *device)
{
	int RetVal = 0;
	struct UPnPDevice *e_Device = device->EmbeddedDevices;
	
	while (e_Device != NULL)
	{
		RetVal += IGD_GetDeviceCount(e_Device);
		e_Device = e_Device->Next;
	}
	if (strncmp(device->DeviceType,"urn:schemas-upnp-org:device:WANConnectionDevice:1",49)==0)
	{
		++RetVal;
	}
	
	return RetVal;
}

//
// Internal method to parse the SOAP Fault
//
int IGD_GetErrorCode(char *buffer, int length)
{
	int RetVal = 500;
	struct IGD_XMLNode *xml, *rootXML;
	
	char *temp;
	int tempLength;
	
	if (buffer == NULL) { return(RetVal); }
	
	rootXML = xml = IGD_ParseXML(buffer, 0, length);
	if (IGD_ProcessXMLNodeList(xml) == 0)
	{
		while (xml != NULL)
		{
			if (xml->NameLength == 8 && memcmp(xml->Name, "Envelope", 8) == 0)
			{
				xml = xml->Next;
				while (xml != NULL)
				{
					if (xml->NameLength == 4 && memcmp(xml->Name, "Body", 4) == 0)
					{
						xml = xml->Next;
						while (xml != NULL)
						{
							if (xml->NameLength == 5 && memcmp(xml->Name, "Fault", 5) == 0)
							{
								xml = xml->Next;
								while (xml != NULL)
								{
									if (xml->NameLength == 6 && memcmp(xml->Name, "detail", 6) == 0)
									{
										xml = xml->Next;
										while (xml != NULL)
										{
											if (xml->NameLength == 9 && memcmp(xml->Name, "UPnPError", 9) == 0)
											{
												xml = xml->Next;
												while (xml != NULL)
												{
													if (xml->NameLength == 9 && memcmp(xml->Name, "errorCode", 9) == 0)
													{
														tempLength = IGD_ReadInnerXML(xml, &temp);
														temp[tempLength] = 0;
														RetVal =atoi(temp);
														xml = NULL;
													}
													if (xml != NULL) {xml = xml->Peer;}
												}
											}
											if (xml != NULL) {xml = xml->Peer;}
										}
									}
									if (xml != NULL) {xml = xml->Peer;}
								}
							}
							if (xml != NULL) {xml = xml->Peer;}
						}
					}
					if (xml != NULL) {xml = xml->Peer;}
				}
			}
			if (xml != NULL) {xml = xml->Peer;}
		}
	}
	IGD_DestructXMLNodeList(rootXML);
	return(RetVal);
}

//
// Internal method to parse the SCPD document
//
void IGD_ProcessSCPD(char* buffer, int length, struct UPnPService *service)
{
	struct UPnPAction *action;
	struct UPnPStateVariable *sv = NULL;
	struct UPnPAllowedValue *av = NULL;
	struct UPnPAllowedValue *avs = NULL;
	
	struct IGD_XMLNode *xml, *rootXML;
	int flg2, flg3, flg4;
	
	char* tempString;
	int tempStringLength;
	
	struct UPnPDevice *root = service->Parent;
	while (root->Parent != NULL) {root = root->Parent;}
	
	rootXML = xml = IGD_ParseXML(buffer, 0, length);
	if (IGD_ProcessXMLNodeList(xml) != 0)
	{
		// The XML Document was not well formed
		root->SCPDError=UPNP_ERROR_SCPD_NOT_WELL_FORMED;
		IGD_DestructXMLNodeList(rootXML);
		return;
	}
	
	while (xml != NULL && strncmp(xml->Name, "!", 1) == 0)
	{
		xml = xml->Next;
	}
	xml = xml->Next;
	while (xml != NULL)
	{
		// Iterate all the actions in the actionList element
		if (xml->NameLength == 10 && memcmp(xml->Name, "actionList", 10) == 0)
		{
			xml = xml->Next;
			flg2 = 0;
			while (flg2 == 0)
			{
				if (xml->NameLength == 6 && memcmp(xml->Name, "action", 6) == 0)
				{
					// Action element
					if ((action = (struct UPnPAction*)malloc(sizeof(struct UPnPAction))) == NULL) ILIBCRITICALEXIT(254);
					action->Name = NULL;
					action->Next = service->Actions;
					service->Actions = action;
					
					xml = xml->Next;
					flg3 = 0;
					while (flg3 == 0)
					{
						if (xml->NameLength == 4 && memcmp(xml->Name, "name", 4) == 0)
						{
							tempStringLength = IGD_ReadInnerXML(xml, &tempString);
							if ((action->Name = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(action->Name, tempString, tempStringLength);
							action->Name[tempStringLength] = '\0';
						}
						if (xml->Peer == NULL)
						{
							flg3 = -1;
							xml = xml->Parent;
						}
						else
						{
							xml = xml->Peer;
						}
					}
				}
				if (xml->Peer == NULL)
				{
					flg2 = -1;
					xml = xml->Parent;
				}
				else
				{
					xml = xml->Peer;
				}
			}
		}
		
		// Iterate all the StateVariables in the state table
		if (xml->NameLength == 17 && memcmp(xml->Name, "serviceStateTable", 17) == 0)
		{
			if (xml->Next->StartTag != 0)
			{
				xml = xml->Next;
				flg2 = 0;
				while (flg2 == 0)
				{
					if (xml->NameLength == 13 && memcmp(xml->Name, "stateVariable", 13) == 0)
					{
						// Initialize a new state variable
						if ((sv = (struct UPnPStateVariable*)malloc(sizeof(struct UPnPStateVariable))) == NULL) ILIBCRITICALEXIT(254);
						sv->AllowedValues = NULL;
						sv->NumAllowedValues = 0;
						sv->Max = NULL;
						sv->Min = NULL;
						sv->Step = NULL;
						sv->Name = NULL;
						sv->Next = service->Variables;
						service->Variables = sv;
						sv->Parent = service;
						
						xml = xml->Next;
						flg3 = 0;
						while (flg3 == 0)
						{
							if (xml->NameLength == 4 && memcmp(xml->Name, "name", 4) == 0)
							{
								// Populate the name
								tempStringLength = IGD_ReadInnerXML(xml, &tempString);
								if ((sv->Name = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
								memcpy(sv->Name, tempString, tempStringLength);
								sv->Name[tempStringLength] = '\0';
							}
							if (xml->NameLength == 16 && memcmp(xml->Name, "allowedValueList", 16) == 0)
							{
								// This state variable defines an allowed value list
								if (xml->Next->StartTag != 0)
								{
									avs = NULL;
									xml = xml->Next;
									flg4 = 0;
									while (flg4 == 0)
									{
										// Iterate through all the allowed values, and reference them
										if (xml->NameLength == 12 && memcmp(xml->Name, "allowedValue", 12) == 0)
										{
											if ((av = (struct UPnPAllowedValue*)malloc(sizeof(struct UPnPAllowedValue))) == NULL) ILIBCRITICALEXIT(254);
											av->Next = avs;
											avs = av;
											
											tempStringLength = IGD_ReadInnerXML(xml, &tempString);
											if ((av->Value = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
											memcpy(av->Value, tempString, tempStringLength);
											av->Value[tempStringLength] = '\0';
										}
										if (xml->Peer != NULL)
										{
											xml = xml->Peer;
										}
										else
										{
											xml = xml->Parent;
											flg4 = -1;
										}
									}
									av = avs;
									while (av != NULL)
									{
										++sv->NumAllowedValues;
										av = av->Next;
									}
									av = avs;
									if ((sv->AllowedValues = (char**)malloc(sv->NumAllowedValues*sizeof(char*))) == NULL) ILIBCRITICALEXIT(254);
									for(flg4=0;flg4<sv->NumAllowedValues;++flg4)
									{
										sv->AllowedValues[flg4] = av->Value;
										av = av->Next;
									}
									av = avs;
									while (av != NULL)
									{
										avs = av->Next;
										free(av);
										av = avs;
									}
								}
							}
							if (xml->NameLength == 17 && memcmp(xml->Name, "allowedValueRange", 17) == 0)
							{
								// The state variable defines a range
								if (xml->Next->StartTag != 0)
								{
									xml = xml->Next;
									flg4 = 0;
									while (flg4 == 0)
									{
										if (xml->NameLength == 7)
										{
											if (memcmp(xml->Name, "minimum", 7) == 0)
											{
												// Set the minimum range
												tempStringLength = IGD_ReadInnerXML(xml, &tempString);
												if ((sv->Min = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
												memcpy(sv->Min, tempString, tempStringLength);
												sv->Min[tempStringLength] = '\0';
											}
											else if (memcmp(xml->Name, "maximum", 7) == 0)
											{
												// Set the maximum range
												tempStringLength = IGD_ReadInnerXML(xml, &tempString);
												if ((sv->Max = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
												memcpy(sv->Max, tempString, tempStringLength);
												sv->Max[tempStringLength] = '\0';
											}
										}
										if (xml->NameLength == 4 && memcmp(xml->Name, "step", 4) == 0)
										{
											// Set the stepping
											tempStringLength = IGD_ReadInnerXML(xml, &tempString);
											if ((sv->Step = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
											memcpy(sv->Step, tempString, tempStringLength);
											sv->Step[tempStringLength] = '\0';
										}
										if (xml->Peer != NULL)
										{
											xml = xml->Peer;
										}
										else
										{
											xml = xml->Parent;
											flg4 = -1;
										}
									}
								}
							}
							if (xml->Peer != NULL)
							{
								xml = xml->Peer;
							}
							else
							{
								flg3 = -1;
								xml = xml->Parent;
							}
						}
					}
					if (xml->Peer != NULL)
					{
						xml = xml->Peer;
					}
					else
					{
						xml = xml->Parent;
						flg2 = -1;
					}
				}
			}
		}
		xml = xml->Peer;
	}
	
	IGD_DestructXMLNodeList(rootXML);
}


//
// Internal method called from SSDP dispatch, when the
// IP Address for a device has changed
//
void IGD_InterfaceChanged(struct UPnPDevice *device)
{
	void *cp = device->CP;
	char *UDN;
	char *LocationURL;
	int Timeout;
	size_t len;
	
	Timeout = device->CacheTime;
	len = strlen(device->UDN) + 1;
	if ((UDN = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	memcpy(UDN, device->UDN, len);
	LocationURL = device->Reserved3;
	device->Reserved3 = NULL;
	
	IGD_SSDP_Sink(NULL, device->UDN, 0, NULL, NULL, 0, UPnPSSDP_NOTIFY, device->CP);
	IGD_SSDP_Sink(NULL, UDN, -1, LocationURL, (struct sockaddr*)&(device->LocationAddr), Timeout, UPnPSSDP_NOTIFY, cp);
	
	free(UDN);
	free(LocationURL);
}

//
// Internal Timed Event Sink, that is called when a device
// has failed to refresh it's NOTIFY packets. So
// we'll assume the device is no longer available
//
void IGD_ExpiredDevice(struct UPnPDevice *device)
{
	LVL3DEBUG(printf("Device[%s] failed to re-advertise in a timely manner\r\n", device->FriendlyName);)
	while (device->Parent != NULL) {device = device->Parent;}
	IGD_SSDP_Sink(NULL, device->UDN, 0, NULL, NULL, 0, UPnPSSDP_NOTIFY, device->CP);
}

//
// The discovery process for the device is complete. Just need to 
// set the reference counters, and notify the layers above us
//
void IGD_FinishProcessingDevice(struct IGD_CP* CP, struct UPnPDevice *RootDevice)
{
	int Timeout = RootDevice->CacheTime;
	struct UPnPDevice *RetDevice;
	int i = 0;
	
	RootDevice->ReferenceCount = 1;
	do
	{
		RetDevice = IGD_GetDevice1(RootDevice, i++);
		if (RetDevice != NULL)
		{
			// Set Reserved to non-zero to indicate that we are passing
			// this instance up the stack to the app layer above. Add a reference
			// to the device while we're at it.
			RetDevice->Reserved=1;
			IGD_AddRef(RetDevice);
			if (CP->DiscoverSink != NULL)
			{
				// Notify the app layer above
				CP->DiscoverSink(RetDevice);
			}
		}
	} while (RetDevice != NULL);
	//
	// Set a timed callback for the refresh cycle of the device. If the
	// device doesn't refresh by then, we'll remove this device.
	//
	IGD_LifeTime_Add(CP->LifeTimeMonitor, RootDevice, Timeout, (IGD_LifeTime_OnCallback)&IGD_ExpiredDevice, NULL);
}

//
// Internal HTTP Sink for fetching the SCPD document
//
void IGD_SCPD_Sink(
void *WebReaderToken, 
int IsInterrupt, 
struct packetheader *header, 
char *buffer, 
int *p_BeginPointer, 
int EndPointer, 
int done, 
void *dv, 
void *sv, 
int *PAUSE)
{
	struct UPnPDevice *device;
	struct UPnPService *service = (struct UPnPService*)sv;
	struct IGD_CP *CP = (struct IGD_CP*)service->Parent->CP;
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( p_BeginPointer );
	UNREFERENCED_PARAMETER( dv );
	UNREFERENCED_PARAMETER( PAUSE );
	
	//
	// header == NULL if there was an error connecting to the device
	// StatusCode != 200 if there was an HTTP error in fetching the SCPD
	// done != 0 when the GET is complete
	//
	if (!(header == NULL || !IGD_WebClientIsStatusOk(header->StatusCode)) && done != 0)
	{
		IGD_ProcessSCPD(buffer, EndPointer, service);
		
		//
		// Fetch the root device
		//
		device = service->Parent;
		while (device->Parent != NULL)
		{
			device = device->Parent;
		}
		//
		// Decrement the counter indicating how many
		// SCPD documents are left to fetch
		//
		--device->SCPDLeft;
		
		//
		// Check to see that we have all the SCPD documents we were
		// looking for
		//
		if (device->SCPDLeft == 0)
		{
			if (device->SCPDError == 0)
			{
				//
				// No errors, complete processing
				//
				IGD_FinishProcessingDevice(CP, device);
			}
			else if (IsInterrupt == 0)
			{
				//
				// Errors occured, so free all the resources, of this 
				// stale device
				//
				IGD_CP_ProcessDeviceRemoval(CP, device);
				IGD_DestructUPnPDevice(device);
			}
		}
	}
	else
	{
		//
		// Errors happened while trying to fetch the SCPD
		//
		if (done != 0 && (header == NULL || !IGD_WebClientIsStatusOk(header->StatusCode)))
		{
			//
			// Get the root device
			//
			device = service->Parent;
			while (device->Parent != NULL)
			{
				device = device->Parent;
			}
			
			//
			// Decrement the counter indicating how many
			// SCPD documents are left to fetch
			//
			--device->SCPDLeft;
			
			//
			// Set the flag indicating that an error has occured
			//
			device->SCPDError = 1;
			if (device->SCPDLeft == 0 && IsInterrupt == 0)
			{
				//
				// If all the SCPD requests have been attempted, free
				// all the resources of this stale device
				//
				IGD_CP_ProcessDeviceRemoval(CP, device);
				IGD_DestructUPnPDevice(device);
			}
		}
	}
}

//
// Internal method used to calculate how many SCPD
// documents will need to be fetched
//
void IGD_CalculateSCPD_FetchCount(struct UPnPDevice *device)
{
	int count = 0;
	struct UPnPDevice *root;
	struct UPnPDevice *e_Device = device->EmbeddedDevices;
	struct UPnPService *s;
	
	while (e_Device != NULL)
	{
		IGD_CalculateSCPD_FetchCount(e_Device);
		e_Device = e_Device->Next;
	}
	
	s = device->Services;
	while (s != NULL)
	{
		++count;
		s = s->Next;
	}
	
	root = device;
	while (root->Parent != NULL)
	{
		root = root->Parent;
	}
	root->SCPDLeft += count;
}

//
// Internal method used to actually make the HTTP
// requests to obtain all the Service Description Documents.
//
void IGD_SCPD_Fetch(struct UPnPDevice *device, struct sockaddr *LocationAddr)
{
	struct UPnPDevice *e_Device = device->EmbeddedDevices;
	struct UPnPService *s;
	char *IP, *Path;
	unsigned short Port;
	struct packetheader *p;
	
	while (e_Device != NULL)
	{
		//
		// We need to recursively call this on all the embedded devices, 
		// to insure all of those services are accounted for
		//
		IGD_SCPD_Fetch(e_Device, LocationAddr);
		e_Device = e_Device->Next;
	}
	
	//
	// Initialize address information to device
	//
	memcpy(&(device->LocationAddr), LocationAddr, INET_SOCKADDR_LENGTH(LocationAddr->sa_family));
	
	//
	// Iterate through all of the services contained in this device
	//
	s = device->Services;
	while (s != NULL)
	{
		//
		// Parse the SCPD URL, and then build the request packet
		//
		IGD_ParseUri(s->SCPDURL, &IP, &Port, &Path, NULL);
		DEBUGSTATEMENT(printf("SCPD: %s Port: %d Path: %s\r\n", IP, Port, Path));
		p = IGD_BuildPacket(IP, Port, Path, "GET");
		
		//
		// Actually make the HTTP Request
		//
		IGD_WebClient_PipelineRequest(
		((struct IGD_CP*)device->CP)->HTTP, 
		(struct sockaddr*)&(s->Parent->LocationAddr), 
		p, 
		&IGD_SCPD_Sink, 
		device, 
		s);
		
		//
		// Free the resources from our IGD_ParseURI() method call
		//
		free(IP);
		free(Path);
		s = s->Next;
	}
}


void IGD_ProcessDeviceXML_iconList(struct UPnPDevice *device, const char *BaseURL, struct IGD_XMLNode *xml)
{
	struct IGD_XMLNode *x2;
	int tempStringLength;
	char *tempString;
	struct parser_result *tpr;
	int numIcons = 0;
	struct UPnPIcon *Icons = NULL;
	char *iconURL=NULL;
	size_t len;
	
	//
	// Count how many icons we have
	//
	x2 = xml;
	while (x2 != NULL)
	{
		if (x2->NameLength == 4 && memcmp(x2->Name, "icon", 4) == 0)
		{
			++numIcons;
		}
		x2 = x2->Peer;
	}
	if ((Icons = (struct UPnPIcon*)malloc(numIcons*sizeof(struct UPnPIcon))) == NULL) ILIBCRITICALEXIT(254);
	memset(Icons, 0, numIcons*sizeof(struct UPnPIcon));
	device->IconsLength = numIcons;
	device->Icons = Icons;
	numIcons = 0;
	
	while (xml != NULL)
	{
		if (xml->NameLength == 4 && memcmp(xml->Name, "icon", 4) == 0)
		{
			x2 = xml->Next;
			while (x2 != NULL)
			{
				if (x2->NameLength == 5 && memcmp(x2->Name, "width", 5) == 0)
				{
					tempStringLength = IGD_ReadInnerXML(x2, &tempString);
					tempString[tempStringLength] = 0;
					Icons[numIcons].width = atoi(tempString);
				}
				if (x2->NameLength == 6 && memcmp(x2->Name, "height", 6) == 0)
				{
					tempStringLength = IGD_ReadInnerXML(x2, &tempString);
					tempString[tempStringLength] = 0;
					Icons[numIcons].height = atoi(tempString);
				}
				if (x2->NameLength == 5 && memcmp(x2->Name, "depth", 5) == 0)
				{
					tempStringLength = IGD_ReadInnerXML(x2, &tempString);
					tempString[tempStringLength] = 0;
					Icons[numIcons].colorDepth = atoi(tempString);
				}
				if (x2->NameLength == 8 && memcmp(x2->Name, "mimetype", 8) == 0)
				{
					tempStringLength = IGD_ReadInnerXML(x2, &tempString);
					Icons[numIcons].mimeType = IGD_String_Copy(tempString, tempStringLength);
				}
				if (x2->NameLength == 3 && memcmp(x2->Name, "url", 3) == 0)
				{
					tempStringLength = IGD_ReadInnerXML(x2, &tempString);
					tempString[tempStringLength] = 0;
					tpr = IGD_ParseString(tempString, 0, tempStringLength, "://", 3);
					if (tpr->NumResults == 1)
					{
						// RelativeURL
						len = strlen(BaseURL);
						if (tempString[0] == '/')
						{
							if ((iconURL = (char*)malloc(1 + len + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(iconURL, BaseURL, len);
							memcpy(iconURL + len, tempString + 1, tempStringLength);
						}
						else
						{
							if ((iconURL = (char*)malloc(2 + len + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(iconURL, BaseURL, len);
							memcpy(iconURL + len, tempString, tempStringLength + 1);
						}
					}
					else
					{
						// AbsoluteURL
						if ((iconURL = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(iconURL, tempString, tempStringLength);
						iconURL[tempStringLength] = '\0';
					}
					IGD_DestructParserResults(tpr);
					Icons[numIcons].url = iconURL;
				}
				x2 = x2->Peer;
			}
			++numIcons;
		}
		xml = xml->Peer;
	}
}
struct UPnPDevice* IGD_ProcessDeviceXML_device(struct IGD_XMLNode *xml, void *v_CP, const char *BaseURL, struct sockaddr *BaseAddr, int Timeout, struct sockaddr* RecvAddr)
{
	struct IGD_XMLNode *tempNode;
	struct IGD_XMLAttribute *a, *root_a;
	int flg, flg2;
	char *tempString;
	int tempStringLength;
	struct parser_result *tpr;
	char *ServiceId = NULL;
	int ServiceIdLength = 0;
	char* ServiceType = NULL;
	int ServiceTypeLength = 0;
	char* SCPDURL = NULL;
	int SCPDURLLength = 0;
	char* EventSubURL = NULL;
	int EventSubURLLength = 0;
	char* ControlURL = NULL;
	int ControlURLLength = 0;
	int ServiceMaxVersion;
	struct UPnPDevice *tempDevice;
	struct UPnPService *TempService;
	struct UPnPDevice *device;
	size_t len;
	
	UNREFERENCED_PARAMETER( BaseAddr );
	
	if ((device = (struct UPnPDevice*)malloc(sizeof(struct UPnPDevice))) == NULL) ILIBCRITICALEXIT(254);
	memset(device, 0, sizeof(struct UPnPDevice));
	
	device->MaxVersion=1;
	device->CP = v_CP;
	device->CacheTime = Timeout;
	memcpy(&(device->InterfaceToHostAddr), RecvAddr, INET_SOCKADDR_LENGTH(RecvAddr->sa_family));
	
	// Create a human readable verion
	if ((device->InterfaceToHost = (char*)malloc(64)) == NULL) ILIBCRITICALEXIT(254);
	if (RecvAddr->sa_family == AF_INET)
	{
		// IPv4
		IGD_Inet_ntop(AF_INET, &(((struct sockaddr_in*)RecvAddr)->sin_addr), device->InterfaceToHost, 64);
	}
	else
	{
		// IPv6
		size_t len;
		device->InterfaceToHost[0] = '[';
		IGD_Inet_ntop(AF_INET6, &(((struct sockaddr_in6*)RecvAddr)->sin6_addr), device->InterfaceToHost + 1, 62);
		len = strlen(device->InterfaceToHost);
		device->InterfaceToHost[len] = ']';
		device->InterfaceToHost[len+1] = 0;
	}
	
	root_a = a = IGD_GetXMLAttributes(xml);
	if (a != NULL)
	{
		while (a != NULL)
		{
			a->Name[a->NameLength]=0;
			if (strcasecmp(a->Name, "MaxVersion") == 0)
			{
				a->Value[a->ValueLength]=0;
				device->MaxVersion = atoi(a->Value);
				break;
			}
			a = a->Next;
		}
		IGD_DestructXMLAttributeList(root_a);
	}
	
	xml = xml->Next;
	while (xml != NULL)
	{
		if (xml->NameLength == 10 && memcmp(xml->Name, "deviceList", 10) == 0)
		{
			if (xml->Next->StartTag != 0)
			{
				//
				// Iterate through all the device elements contained in the
				// deviceList element
				//
				xml = xml->Next;
				flg2 = 0;
				while (flg2 == 0)
				{
					if (xml->NameLength == 6 && memcmp(xml->Name, "device", 6) == 0)
					{
						//
						// If this device contains other devices, then we can recursively call ourselves
						//
						tempDevice = IGD_ProcessDeviceXML_device(xml, v_CP, BaseURL, (struct sockaddr*)&(device->LocationAddr), Timeout, RecvAddr);
						tempDevice->Parent = device;
						tempDevice->Next = device->EmbeddedDevices;
						device->EmbeddedDevices = tempDevice;
					}
					if (xml->Peer == NULL)
					{
						flg2 = 1;
						xml = xml->Parent;
					}
					else
					{
						xml = xml->Peer;
					}
				}
			}
		}
		else if (xml->NameLength == 3 && memcmp(xml->Name, "UDN", 3) == 0)
		{
			//
			// Copy the UDN out of the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if (tempStringLength>5)
			{
				if (memcmp(tempString, "uuid:", 5) == 0)
				{
					tempString += 5;
					tempStringLength -= 5;
				}
				if ((device->UDN = (char*)malloc(tempStringLength + 1)) == NULL) ILIBCRITICALEXIT(254);
				memcpy(device->UDN, tempString, tempStringLength);
				device->UDN[tempStringLength] = '\0';
			}
		} 
		else if (xml->NameLength == 10 && memcmp(xml->Name, "deviceType", 10) == 0)
		{
			//
			// Copy the device type out of the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			
			if ((device->DeviceType = (char*)malloc(tempStringLength + 1)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->DeviceType, tempString, tempStringLength);
			device->DeviceType[tempStringLength] = '\0';
		}
		else if (xml->NameLength == 12 && memcmp(xml->Name, "friendlyName", 12) == 0)
		{
			//
			// Copy the friendly name out of the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->FriendlyName = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->FriendlyName, tempString, tempStringLength);
			device->FriendlyName[tempStringLength] = '\0';
		} 
		else if (xml->NameLength == 12 && memcmp(xml->Name, "manufacturer", 12) == 0)
		{
			//
			// Copy the Manufacturer from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->ManufacturerName = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->ManufacturerName, tempString, tempStringLength);
			device->ManufacturerName[tempStringLength] = '\0';
		} 
		else if (xml->NameLength == 15 && memcmp(xml->Name, "manufacturerURL", 15) == 0)
		{
			//
			// Copy the Manufacturer's URL from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->ManufacturerURL = (char*)malloc(1+tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->ManufacturerURL, tempString, tempStringLength);
			device->ManufacturerURL[tempStringLength] = '\0';
		} 
		else if (xml->NameLength == 16 && memcmp(xml->Name, "modelDescription", 16) == 0)
		{
			//
			// Copy the model meta data from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->ModelDescription = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->ModelDescription, tempString, tempStringLength);
			device->ModelDescription[tempStringLength] = '\0';
		} 
		else if (xml->NameLength == 9 && memcmp(xml->Name, "modelName", 9) == 0)
		{
			//
			// Copy the model meta data from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->ModelName = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->ModelName, tempString, tempStringLength);
			device->ModelName[tempStringLength] = '\0';
		} 
		else if (xml->NameLength == 11 && memcmp(xml->Name, "modelNumber", 11) == 0)
		{
			//
			// Copy the model meta data from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->ModelNumber = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->ModelNumber, tempString, tempStringLength);
			device->ModelNumber[tempStringLength] = '\0';
		} 
		else if (xml->NameLength == 8 && memcmp(xml->Name, "modelURL", 8) == 0)
		{
			//
			// Copy the model meta data from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if ((device->ModelURL = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(device->ModelURL, tempString, tempStringLength);
			device->ModelURL[tempStringLength] = '\0';
		}
		else if (xml->NameLength == 8 && memcmp(xml->Name, "iconList", 8) == 0)
		{
			IGD_ProcessDeviceXML_iconList(device, BaseURL, xml->Next);
		} 
		else if (xml->NameLength == 15 && memcmp(xml->Name, "presentationURL", 15) == 0)
		{
			//
			// Copy the presentation URL from the description document
			//
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			tempString[tempStringLength] = 0;
			tpr = IGD_ParseString(tempString, 0, tempStringLength, "://", 3);
			if (tpr->NumResults == 1)
			{
				// RelativeURL
				len = strlen(BaseURL);
				if (tempString[0] == '/')
				{
					if ((device->PresentationURL = (char*)malloc(1 + len + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
					memcpy(device->PresentationURL, BaseURL, len);
					memcpy(device->PresentationURL + len, tempString + 1, tempStringLength);
				}
				else
				{
					if ((device->PresentationURL = (char*)malloc(2 + len + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
					memcpy(device->PresentationURL, BaseURL, len);
					memcpy(device->PresentationURL + len, tempString, tempStringLength + 1);
				}
			}
			else
			{
				// AbsoluteURL
				if ((device->PresentationURL = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
				memcpy(device->PresentationURL, tempString, tempStringLength);
				device->PresentationURL[tempStringLength] = '\0';
			}
			IGD_DestructParserResults(tpr);
		} else
		
		if (xml->NameLength == 11 && memcmp(xml->Name, "serviceList", 11) == 0)
		{
			// Iterate through all the services contained in the serviceList element
			tempNode = xml;
			xml = xml->Next;
			while (xml != NULL)
			{
				if (xml->NameLength == 7 && memcmp(xml->Name, "service", 7) == 0)
				{
					ServiceType = NULL;
					ServiceTypeLength = 0;
					SCPDURL = NULL;
					SCPDURLLength = 0;
					EventSubURL = NULL;
					EventSubURLLength = 0;
					ControlURL = NULL;
					ControlURLLength = 0;
					ServiceMaxVersion = 1;
					
					root_a = a = IGD_GetXMLAttributes(xml);
					if (a != NULL)
					{
						while (a != NULL)
						{
							a->Name[a->NameLength]=0;
							if (strcasecmp(a->Name, "MaxVersion") == 0)
							{
								a->Value[a->ValueLength]=0;
								ServiceMaxVersion = atoi(a->Value);
								break;
							}
							a = a->Next;
						}
						IGD_DestructXMLAttributeList(root_a);
					}
					
					xml = xml->Next;
					flg = 0;
					while (flg == 0)
					{
						//
						// Fetch the URIs associated with this service
						//
						if (xml->NameLength == 11 && memcmp(xml->Name, "serviceType", 11) == 0)
						{
							ServiceTypeLength = IGD_ReadInnerXML(xml, &ServiceType);
						}
						else if (xml->NameLength == 9 && memcmp(xml->Name, "serviceId", 9) == 0)
						{
							ServiceIdLength = IGD_ReadInnerXML(xml, &ServiceId);
						}
						else if (xml->NameLength == 7 && memcmp(xml->Name, "SCPDURL", 7) == 0)
						{
							SCPDURLLength = IGD_ReadInnerXML(xml, &SCPDURL);
						}
						else if (xml->NameLength == 10 && memcmp(xml->Name, "controlURL", 10) == 0)
						{
							ControlURLLength = IGD_ReadInnerXML(xml, &ControlURL);
						}
						else if (xml->NameLength == 11 && memcmp(xml->Name, "eventSubURL", 11) == 0)
						{
							EventSubURLLength = IGD_ReadInnerXML(xml, &EventSubURL);
						}
						
						if (xml->Peer != NULL)
						{
							xml = xml->Peer;
						}
						else
						{
							flg = 1;
							xml = xml->Parent;
						}
					}
					
					// Finished Parsing the ServiceSection, build the Service
					ServiceType[ServiceTypeLength] = '\0';
					SCPDURL[SCPDURLLength] = '\0';
					EventSubURL[EventSubURLLength] = '\0';
					ControlURL[ControlURLLength] = '\0';
					
					if ((TempService = (struct UPnPService*)malloc(sizeof(struct UPnPService))) == NULL) ILIBCRITICALEXIT(254);
					TempService->SubscriptionID = NULL;
					if ((TempService->ServiceId = (char*)malloc(ServiceIdLength + 1)) == NULL) ILIBCRITICALEXIT(254);
					TempService->ServiceId[ServiceIdLength] = 0;
					memcpy(TempService->ServiceId, ServiceId, ServiceIdLength);
					
					TempService->Actions = NULL;
					TempService->Variables = NULL;
					TempService->Next = NULL;
					TempService->Parent = device;
					TempService->MaxVersion = ServiceMaxVersion;
					if (EventSubURLLength >= 7 && memcmp(EventSubURL, "http://", 6) == 0)
					{
						// Explicit
						if ((TempService->SubscriptionURL = (char*)malloc(EventSubURLLength + 1)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(TempService->SubscriptionURL, EventSubURL, EventSubURLLength);
						TempService->SubscriptionURL[EventSubURLLength] = '\0';
					}
					else
					{
						// Relative
						if (memcmp(EventSubURL, "/", 1) != 0)
						{
							if ((TempService->SubscriptionURL = (char*)malloc(EventSubURLLength + (int)strlen(BaseURL) + 1)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(TempService->SubscriptionURL, BaseURL, (int)strlen(BaseURL));
							memcpy(TempService->SubscriptionURL+(int)strlen(BaseURL), EventSubURL, EventSubURLLength);
							TempService->SubscriptionURL[EventSubURLLength+(int)strlen(BaseURL)] = '\0';
						}
						else
						{
							if ((TempService->SubscriptionURL = (char*)malloc(EventSubURLLength + (int)strlen(BaseURL) + 1)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(TempService->SubscriptionURL, BaseURL, (int)strlen(BaseURL));
							memcpy(TempService->SubscriptionURL+(int)strlen(BaseURL), EventSubURL+1, EventSubURLLength-1);
							TempService->SubscriptionURL[EventSubURLLength+(int)strlen(BaseURL) - 1] = '\0';
						}
					}
					if (ControlURLLength>=7 && memcmp(ControlURL, "http://", 6) == 0)
					{
						// Explicit
						if ((TempService->ControlURL = (char*)malloc(ControlURLLength + 1)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(TempService->ControlURL, ControlURL, ControlURLLength);
						TempService->ControlURL[ControlURLLength] = '\0';
					}
					else
					{
						// Relative
						if (memcmp(ControlURL, "/", 1) != 0)
						{
							if ((TempService->ControlURL = (char*)malloc(ControlURLLength + (int)strlen(BaseURL)+1)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(TempService->ControlURL, BaseURL, (int)strlen(BaseURL));
							memcpy(TempService->ControlURL + (int)strlen(BaseURL), ControlURL, ControlURLLength);
							TempService->ControlURL[ControlURLLength+(int)strlen(BaseURL)] = '\0';
						}
						else
						{
							if ((TempService->ControlURL = (char*)malloc(ControlURLLength + (int)strlen(BaseURL)+1)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(TempService->ControlURL, BaseURL, (int)strlen(BaseURL));
							memcpy(TempService->ControlURL + (int)strlen(BaseURL), ControlURL + 1, ControlURLLength-1);
							TempService->ControlURL[ControlURLLength+(int)strlen(BaseURL) - 1] = '\0';
						}
					}
					if (SCPDURLLength >= 7 && memcmp(SCPDURL, "http://", 6) == 0)
					{
						// Explicit
						if ((TempService->SCPDURL = (char*)malloc(SCPDURLLength+1)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(TempService->SCPDURL, SCPDURL, SCPDURLLength);
						TempService->SCPDURL[SCPDURLLength] = '\0';
					}
					else
					{
						// Relative
						if (memcmp(SCPDURL, "/", 1) != 0)
						{
							if ((TempService->SCPDURL = (char*)malloc(SCPDURLLength + (int)strlen(BaseURL) + 1)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(TempService->SCPDURL, BaseURL, (int)strlen(BaseURL));
							memcpy(TempService->SCPDURL + (int)strlen(BaseURL), SCPDURL, SCPDURLLength);
							TempService->SCPDURL[SCPDURLLength+(int)strlen(BaseURL)] = '\0';
						}
						else
						{
							if ((TempService->SCPDURL = (char*)malloc(SCPDURLLength + (int)strlen(BaseURL) + 1)) == NULL) ILIBCRITICALEXIT(254);
							memcpy(TempService->SCPDURL, BaseURL, (int)strlen(BaseURL));
							memcpy(TempService->SCPDURL + (int)strlen(BaseURL), SCPDURL + 1, SCPDURLLength - 1);
							TempService->SCPDURL[SCPDURLLength+(int)strlen(BaseURL) - 1] = '\0';
						}
					}
					
					if ((TempService->ServiceType = (char*)malloc(ServiceTypeLength + 1)) == NULL) ILIBCRITICALEXIT(254);
					snprintf(TempService->ServiceType, ServiceTypeLength + 1, ServiceType, ServiceTypeLength);
					TempService->Next = device->Services;
					device->Services = TempService;
					
					DEBUGSTATEMENT(printf("ServiceType: %s\r\nSCPDURL: %s\r\nEventSubURL: %s\r\nControl URL: %s\r\n", ServiceType, SCPDURL, EventSubURL, ControlURL));
				}
				xml = xml->Peer;
			}
			xml = tempNode;
		} // End of ServiceList
		xml = xml->Peer;
	} // End of While
	
	return(device);
}

//
// Internal method used to parse the Device Description XML Document
//
int IGD_ProcessDeviceXML(void *v_CP, char* buffer, int BufferSize, char* LocationURL, struct sockaddr *LocationAddr, struct sockaddr *RecvAddr, int Timeout)
{
	struct UPnPDevice *RootDevice = NULL;
	char* IP;
	unsigned short Port;
	char* BaseURL = NULL;
	struct IGD_XMLNode *rootXML;
	struct IGD_XMLNode *xml;
	char* tempString;
	int tempStringLength;
	size_t len;
	
	//
	// Parse the XML, check that it's wellformed, and build the namespace lookup table
	//
	rootXML = IGD_ParseXML(buffer, 0, BufferSize);
	if (IGD_ProcessXMLNodeList(rootXML) != 0)
	{
		IGD_DestructXMLNodeList(rootXML);
		return(1);
	}
	IGD_XML_BuildNamespaceLookupTable(rootXML);
	
	//
	// We need to figure out if this particular device uses
	// relative URLs using the URLBase element.
	//
	xml = rootXML;
	xml = xml->Next;
	while (xml != NULL)
	{
		if (xml->NameLength == 7 && memcmp(xml->Name, "URLBase", 7) == 0)
		{
			tempStringLength = IGD_ReadInnerXML(xml, &tempString);
			if (tempString[tempStringLength-1] != '/')
			{
				if ((BaseURL = (char*)malloc(2 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
				memcpy(BaseURL, tempString, tempStringLength);
				BaseURL[tempStringLength] = '/';
				BaseURL[tempStringLength+1] = '\0';
			}
			else
			{
				if ((BaseURL = (char*)malloc(1 + tempStringLength)) == NULL) ILIBCRITICALEXIT(254);
				memcpy(BaseURL, tempString, tempStringLength);
				BaseURL[tempStringLength] = '\0';
			}
			break;
		}
		xml = xml->Peer;
	}
	
	//
	// If the URLBase was not specified, then we need force the
	// base url to be that of the base path that we used to fetch
	// the description document from.
	//
	
	if (BaseURL == NULL)
	{
		size_t len;
		IGD_ParseUri(LocationURL, &IP, &Port, NULL, NULL);
		len = 18 + strlen(IP);
		if ((BaseURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
		snprintf(BaseURL, len, "http://%s:%d/", IP, Port);
		free(IP);
	}
	
	DEBUGSTATEMENT(printf("BaseURL: %s\r\n", BaseURL));
	
	//
	// Now that we have the base path squared away, we can actually parse this thing!
	// Let's start by looking for the device element
	//
	xml = rootXML;
	xml = xml->Next;
	while (xml != NULL && xml->NameLength != 6 && memcmp(xml->Name, "device", 6) != 0)
	{
		xml = xml->Peer;
	}
	if (xml == NULL)
	{
		// Error
		IGD_DestructXMLNodeList(rootXML);
		return(1);
	}
	
	//
	// Process the Device Element. If the device element contains other devices, 
	// it will be recursively called, so we don't need to worry
	//
	RootDevice = IGD_ProcessDeviceXML_device(xml, v_CP, BaseURL, LocationAddr, Timeout, RecvAddr);
	free(BaseURL);
	IGD_DestructXMLNodeList(rootXML);
	
	// Save reference to LocationURL in the RootDevice
	len = strlen(LocationURL) + 1;
	if ((RootDevice->LocationURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	snprintf(RootDevice->LocationURL, len, "%s", LocationURL);
	memcpy(&(RootDevice->LocationAddr), LocationAddr, INET_SOCKADDR_LENGTH(LocationAddr->sa_family));
	
	//
	// Now that we processed the device XML document, we need to fetch
	// and parse the service description documents.
	//
	IGD_ProcessDevice(v_CP, RootDevice);
	RootDevice->SCPDLeft = 0;
	IGD_CalculateSCPD_FetchCount(RootDevice);
	if (RootDevice->SCPDLeft == 0)
	{
		//
		// If this device doesn't contain any services, than we don't
		// need to bother with fetching any SCPD documents
		//
		IGD_FinishProcessingDevice((struct IGD_CP*)v_CP, RootDevice);
	}
	else
	{
		//
		// Fetch the SCPD documents
		//
		IGD_SCPD_Fetch(RootDevice, LocationAddr);
	}
	return 0;
}


//
// The internal sink for obtaining the Device Description Document
//
void IGD_HTTP_Sink_DeviceDescription(
void *WebReaderToken, 
int IsInterrupt, 
struct packetheader *header, 
char *buffer, 
int *p_BeginPointer, 
int EndPointer, 
int done, 
void *user, 
void *cp, 
int *PAUSE)
{
	struct CustomUserData *customData = (struct CustomUserData*)user;
	struct IGD_CP* CP = (struct IGD_CP*)cp;
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done != 0)
	{
		IGD_DeleteEntry(CP->DeviceTable_Tokens, customData->UDN, (int)strlen(customData->UDN));
		IGD_DeleteEntry(CP->DeviceTable_URI, customData->LocationURL, (int)strlen(customData->LocationURL));
	}
	
	if (header != NULL && IGD_WebClientIsStatusOk(header->StatusCode) && done != 0 && EndPointer > 0)
	{
		if (IGD_ProcessDeviceXML(cp, buffer, EndPointer-(*p_BeginPointer), customData->buffer, (struct sockaddr*)&(customData->LocationAddr), (struct sockaddr*)(header->ReceivingAddress), customData->Timeout) != 0)
		{
			IGD_DeleteEntry(CP->DeviceTable_UDN, customData->UDN, (int)strlen(customData->UDN));
			IGD_DeleteEntry(CP->DeviceTable_URI, customData->LocationURL, (int)strlen(customData->LocationURL));
		}
	}
	else if ((header == NULL) || (header != NULL && !IGD_WebClientIsStatusOk(header->StatusCode)))
	{
		if (done != 0 && CP->ErrorDispatch != NULL)
		{
			CP->ErrorDispatch(customData->UDN, customData->LocationURL, header != NULL?header->StatusCode:-1);
		}
		IGD_DeleteEntry(CP->DeviceTable_UDN, customData->UDN, (int)strlen(customData->UDN));
		IGD_DeleteEntry(CP->DeviceTable_URI, customData->LocationURL, (int)strlen(customData->LocationURL));
	}
	
	if (done != 0)
	{
		free(customData->buffer);
		free(customData->UDN);
		free(customData->LocationURL);
		free(user);
	}
}

void IGD_FlushRequest(struct UPnPDevice *device)
{
	struct UPnPDevice *ed = device->EmbeddedDevices;
	struct UPnPService *s = device->Services;
	
	while (ed != NULL)
	{
		IGD_FlushRequest(ed);
		ed = ed->Next;
	}
	while (s != NULL)
	{
		s = s->Next;
	}
}

//
// An internal method used to recursively release all the references to a device.
// While doing this, we'll also check to see if we gave any of these to the app layer
// above, and if so, trigger a removal event.
//
void IGD_CP_RecursiveReleaseAndEventDevice(struct IGD_CP* CP, struct UPnPDevice *device)
{
	struct UPnPDevice *temp = device->EmbeddedDevices;
	
	IGD_DeleteEntry(CP->DeviceTable_URI, device->LocationURL, (int)strlen(device->LocationURL));
	IGD_AddEntry(CP->DeviceTable_UDN, device->UDN, (int)strlen(device->UDN), NULL);
	
	while (temp != NULL)
	{
		IGD_CP_RecursiveReleaseAndEventDevice(CP, temp);
		temp = temp->Next;
	}
	
	if (device->Reserved != 0)
	{
		//
		// We gave this to the app layer above
		//
		if (CP->RemoveSink != NULL)
		{
			CP->RemoveSink(device);
		}
		IGD_Release(device);
	}
}
void IGD_CP_ProcessDeviceRemoval(struct IGD_CP* CP, struct UPnPDevice *device)
{
	struct UPnPDevice *temp = device->EmbeddedDevices;
	struct UPnPService *s;
	struct UPnPDevice *dTemp = device;
	
	if (dTemp->Parent != NULL)
	{
		dTemp = dTemp->Parent;
	}
	IGD_LifeTime_Remove(CP->LifeTimeMonitor, dTemp);
	
	while (temp != NULL)
	{
		IGD_CP_ProcessDeviceRemoval(CP, temp);
		temp = temp->Next;
	}
	
	s = device->Services;
	while (s != NULL)
	{
		// Remove all the pending requests
		IGD_WebClient_DeleteRequests(((struct IGD_CP*)device->CP)->HTTP, (struct sockaddr*)&(s->Parent->LocationAddr));
		
		IGD_LifeTime_Remove(CP->LifeTimeMonitor, s);
		s = s->Next;
	}
	
	if (device->Reserved != 0)
	{
		// Device was flagged, and given to the user
		if (CP->RemoveSink != NULL) CP->RemoveSink(device);
		IGD_Release(device);
	}
	
	IGD_HashTree_Lock(CP->DeviceTable_UDN);
	IGD_DeleteEntry(CP->DeviceTable_UDN, device->UDN, (int)strlen(device->UDN));
	if (device->LocationURL != NULL)
	{
		IGD_DeleteEntry(CP->DeviceTable_URI, device->LocationURL, (int)strlen(device->LocationURL));
	}
	IGD_HashTree_UnLock(CP->DeviceTable_UDN);
}

//
// The internal sink called by our SSDP Module
//
void IGD_SSDP_Sink(void *sender, char* UDN, int Alive, char* LocationURL, struct sockaddr* LocationAddr, int Timeout, UPnPSSDP_MESSAGE m, void *cp)
{
	int i = 0;
	char* buffer;
	char* IP;
	char* Path;
	unsigned short Port;
	struct packetheader *p;
	struct UPnPDevice *device;
	struct CustomUserData *customData;
	struct IGD_CP *CP = (struct IGD_CP*)cp;
	struct timeval t;
	size_t len;
	IGD_WebClient_RequestToken RT;
	void *v;
	
	UNREFERENCED_PARAMETER( sender );
	
	if (Alive != 0)
	{
		// Hello
		
		// A device should never advertise it's timeout value being zero. But if
		// it does, let's not waste time processing stuff
		if (Timeout == 0) {return;}
		DEBUGSTATEMENT(printf("Hello, LocationURL: %s\r\n", LocationURL));
		
		// Lock the table
		IGD_HashTree_Lock(CP->DeviceTable_UDN);
		
		// We have never seen this location URL, nor have we ever seen this UDN before, 
		// so let's try and find it
		if (IGD_HasEntry(CP->DeviceTable_URI, LocationURL, (int)strlen(LocationURL)) == 0 && IGD_HasEntry(CP->DeviceTable_UDN, UDN, (int)strlen(UDN)) == 0)
		{
			// Parse the location uri of the device description document, 
			// and build the request packet
			IGD_ParseUri(LocationURL, &IP, &Port, &Path, NULL);
			DEBUGSTATEMENT(printf("IP: %s Port: %d Path: %s\r\n", IP, Port, Path));
			p = IGD_BuildPacket(IP, Port, Path, "GET");
			
			len = strlen(LocationURL) + 1;
			if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(buffer, LocationURL, len);
			
			if ((customData = (struct CustomUserData*)malloc(sizeof(struct CustomUserData))) == NULL) ILIBCRITICALEXIT(254);
			customData->Timeout = Timeout;
			customData->buffer = buffer;
			memcpy(&(customData->LocationAddr), LocationAddr, INET_SOCKADDR_LENGTH(LocationAddr->sa_family));
			if (customData->LocationAddr.sin6_family == AF_INET6) customData->LocationAddr.sin6_port = htons(Port); else ((struct sockaddr_in*)&(customData->LocationAddr))->sin_port = htons(Port);
			
			len = strlen(LocationURL) + 1;
			if ((customData->LocationURL = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(customData->LocationURL, LocationURL, len);
			len = strlen(UDN) + 1;
			if ((customData->UDN = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
			memcpy(customData->UDN, UDN, len);
			
			// Add these items into our table, so we don't try to find it multiple times
			IGD_AddEntry(CP->DeviceTable_URI, LocationURL, (int)strlen(LocationURL), customData->UDN);
			IGD_AddEntry(CP->DeviceTable_UDN, UDN, (int)strlen(UDN), NULL);
			
			// Make the HTTP request to fetch the Device Description Document
			RT = IGD_WebClient_PipelineRequest(
			((struct IGD_CP*)cp)->HTTP,
			(struct sockaddr*)&(customData->LocationAddr),
			p,
			&IGD_HTTP_Sink_DeviceDescription,
			customData,
			cp);
			
			free(IP);
			free(Path);
			
			IGD_AddEntry(CP->DeviceTable_Tokens, UDN, (int)strlen(UDN), RT);
		}
		else
		{
			// We have seen this device before, so thse packets are
			// Periodic Notify Packets
			
			// Fetch the device, this packet is advertising
			device = (struct UPnPDevice*)IGD_GetEntry(CP->DeviceTable_UDN, UDN, (int)strlen(UDN));
			if (device != NULL && device->ReservedID == 0 && m == UPnPSSDP_NOTIFY)
			{
				// Get the root device
				while (device->Parent != NULL)
				{
					device = device->Parent;
				}
				
				// Is this device on the same IP Address?
				if (strcmp(device->LocationURL, LocationURL) == 0)
				{
					// Extend LifetimeMonitor duration
					gettimeofday(&t, NULL);
					device->Reserved2 = t.tv_sec;
					IGD_LifeTime_Remove(((struct IGD_CP*)cp)->LifeTimeMonitor, device);
					IGD_LifeTime_Add(((struct IGD_CP*)cp)->LifeTimeMonitor, device, Timeout, (IGD_LifeTime_OnCallback)&IGD_ExpiredDevice, NULL);
				}
				else
				{
					// Same device, different Interface
					// Wait up to 7 seconds to see if the old interface is still valid.
					gettimeofday(&t, NULL);
					if (t.tv_sec-device->Reserved2>10)
					{
						device->Reserved2 = t.tv_sec;
						if (device->Reserved3 != NULL) { free(device->Reserved3); }
						len = strlen(LocationURL) + 1;
						if ((device->Reserved3 = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(device->Reserved3, LocationURL, len);
						
						IGD_ParseUri(LocationURL, NULL, &Port, NULL, NULL);
						memcpy(&(device->LocationAddr), LocationAddr, INET_SOCKADDR_LENGTH(LocationAddr->sa_family));
						if (device->LocationAddr.sin6_family == AF_INET6) device->LocationAddr.sin6_port = htons(Port); else ((struct sockaddr_in*)&(device->LocationAddr))->sin_port = htons(Port);
						
						IGD_LifeTime_Remove(((struct IGD_CP*)cp)->LifeTimeMonitor, device);
						IGD_LifeTime_Add(((struct IGD_CP*)cp)->LifeTimeMonitor, device, 7, (IGD_LifeTime_OnCallback)&IGD_InterfaceChanged, NULL);
					}
				}
			}
		}
		IGD_HashTree_UnLock(CP->DeviceTable_UDN);
	}
	else
	{
		// Bye Bye
		IGD_HashTree_Lock(CP->DeviceTable_UDN);
		
		v = IGD_GetEntry(CP->DeviceTable_Tokens, UDN, (int)strlen(UDN));
		if (v != NULL)
		{
			IGD_WebClient_CancelRequest((IGD_WebClient_RequestToken)v);
			IGD_DeleteEntry(CP->DeviceTable_Tokens, UDN, (int)strlen(UDN));
			IGD_DeleteEntry(CP->DeviceTable_UDN, UDN, (int)strlen(UDN));
		}
		device = (struct UPnPDevice*)IGD_GetEntry(CP->DeviceTable_UDN, UDN, (int)strlen(UDN));
		
		// Find the device that is going away
		IGD_HashTree_UnLock(CP->DeviceTable_UDN);
		
		if (device != NULL)
		{
			// Get the root device
			while (device->Parent != NULL) device = device->Parent;
			
			// Remove the timed event, checking the refreshing of notify packets
			IGD_LifeTime_Remove(((struct IGD_CP*)cp)->LifeTimeMonitor, device);
			IGD_CP_ProcessDeviceRemoval(CP, device);
			
			// If the app above subscribed to events, there will be extra references
			// that we can delete, otherwise, the device ain't ever going away
			i = device->ReferenceTiedToEvents;
			while (i != 0)
			{
				IGD_Release(device);
				--i;
			}
			IGD_Release(device);
		}
	}
}

void IGD_WANIPConnection_EventSink(char* buffer, int bufferlength, struct UPnPService *service)
{
	struct IGD_XMLNode *xml,*rootXML;
	char *tempString;
	int tempStringLength;
	int flg,flg2;
	
	unsigned short PortMappingNumberOfEntries = 0;
	char* ExternalIPAddress = 0;
	char* ConnectionStatus = 0;
	char* PossibleConnectionTypes = 0;
	unsigned long TempULong;
	
	/* Parse SOAP */
	rootXML = xml = IGD_ParseXML(buffer, 0, bufferlength);
	IGD_ProcessXMLNodeList(xml);
	
	while(xml != NULL)
	{
		if (xml->NameLength == 11 && memcmp(xml->Name, "propertyset", 11)==0)
		{
			if (xml->Next->StartTag != 0)
			{
				flg = 0;
				xml = xml->Next;
				while(flg==0)
				{
					if (xml->NameLength == 8 && memcmp(xml->Name, "property", 8)==0)
					{
						xml = xml->Next;
						flg2 = 0;
						while(flg2==0)
						{
							if (xml->NameLength == 26 && memcmp(xml->Name, "PortMappingNumberOfEntries", 26) == 0)
							{
								tempStringLength = IGD_ReadInnerXML(xml,&tempString);
								if (IGD_GetULong(tempString, tempStringLength, &TempULong)==0)
								{
									PortMappingNumberOfEntries = (unsigned short) TempULong;
								}
								if (IGD_EventCallback_WANIPConnection_PortMappingNumberOfEntries != NULL)
								{
									IGD_EventCallback_WANIPConnection_PortMappingNumberOfEntries(service,PortMappingNumberOfEntries);
								}
							}
							if (xml->NameLength == 17 && memcmp(xml->Name, "ExternalIPAddress", 17) == 0)
							{
								tempStringLength = IGD_ReadInnerXML(xml,&tempString);
								tempString[tempStringLength] = '\0';
								ExternalIPAddress = tempString;
								if (IGD_EventCallback_WANIPConnection_ExternalIPAddress != NULL)
								{
									IGD_EventCallback_WANIPConnection_ExternalIPAddress(service,ExternalIPAddress);
								}
							}
							if (xml->NameLength == 16 && memcmp(xml->Name, "ConnectionStatus", 16) == 0)
							{
								tempStringLength = IGD_ReadInnerXML(xml,&tempString);
								tempString[tempStringLength] = '\0';
								ConnectionStatus = tempString;
								if (IGD_EventCallback_WANIPConnection_ConnectionStatus != NULL)
								{
									IGD_EventCallback_WANIPConnection_ConnectionStatus(service,ConnectionStatus);
								}
							}
							if (xml->NameLength == 23 && memcmp(xml->Name, "PossibleConnectionTypes", 23) == 0)
							{
								tempStringLength = IGD_ReadInnerXML(xml,&tempString);
								tempString[tempStringLength] = '\0';
								PossibleConnectionTypes = tempString;
								if (IGD_EventCallback_WANIPConnection_PossibleConnectionTypes != NULL)
								{
									IGD_EventCallback_WANIPConnection_PossibleConnectionTypes(service,PossibleConnectionTypes);
								}
							}
							if (xml->Peer!=NULL)
							{
								xml = xml->Peer;
							}
							else
							{
								flg2 = -1;
								xml = xml->Parent;
							}
						}
					}
					if (xml->Peer!=NULL)
					{
						xml = xml->Peer;
					}
					else
					{
						flg = -1;
						xml = xml->Parent;
					}
				}
			}
		}
		xml = xml->Peer;
	}
	
	IGD_DestructXMLNodeList(rootXML);
}


//
// Internal HTTP Sink, called when an event is delivered
//
void IGD_OnEventSink(
struct IGD_WebServer_Session *sender, 
int InterruptFlag, 
struct packetheader *header, 
char *buffer, 
int *BeginPointer, 
int BufferSize, 
int done)
{
	int type_length;
	char* sid = NULL;
	void* value = NULL;
	struct UPnPService *service = NULL;
	struct packetheader_field_node *field = NULL;
	struct packetheader *resp;
	
	char *txt;
	if (header != NULL && sender->User3 == NULL && done == 0)
	{
		sender->User3 = (void*)~0;
		txt = IGD_GetHeaderLine(header, "Expect", 6);
		if (txt != NULL)
		{
			if (strcasecmp(txt, "100-Continue") == 0)
			{
				// Expect Continue
				IGD_WebServer_Send_Raw(sender, "HTTP/1.1 100 Continue\r\n\r\n", 25, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
			else
			{
				// Don't understand
				IGD_WebServer_Send_Raw(sender, "HTTP/1.1 417 Expectation Failed\r\n\r\n", 35, IGD_AsyncSocket_MemoryOwnership_STATIC, 1);
				IGD_WebServer_DisconnectSession(sender);
				return;
			}
		}
	}
	
	UNREFERENCED_PARAMETER( InterruptFlag );
	UNREFERENCED_PARAMETER( BeginPointer );
	
	if (done != 0)
	{
		// We recieved the event, let's prepare the response
		resp = IGD_CreateEmptyPacket();
		IGD_SetVersion(resp, "1.1", 3);
		IGD_AddHeaderLine(resp, "Server", 6, IGD_PLATFORM, (int)strlen(IGD_PLATFORM));
		IGD_AddHeaderLine(resp, "Content-Length", 14, "0", 1);
		field = header->FirstField;
		while (field != NULL)
		{
			if (field->FieldLength == 3)
			{
				if (strncasecmp(field->Field, "SID", 3) == 0)
				{
					// We need to determine who this event is for, by looking at the subscription id
					if ((sid = (char*)malloc(field->FieldDataLength + 1)) == NULL) ILIBCRITICALEXIT(254);
					snprintf(sid, field->FieldDataLength + 1, "%s", field->FieldData);
					
					// Do we know about this SID?
					value = IGD_GetEntry(((struct IGD_CP*)sender->User)->SIDTable, field->FieldData, field->FieldDataLength);
					break;
				}
			}
			field = field->NextField;
		}
		
		if (value == NULL)
		{
			// Not a valid SID
			IGD_SetStatusCode(resp, 412, "Failed", 6);
		}
		else
		{
			IGD_SetStatusCode(resp, 200, "OK", 2);
			service = (struct UPnPService*)value;
			
			type_length = (int)strlen(service->ServiceType);
			if (type_length>45 && strncmp("urn:schemas-upnp-org:service:WANIPConnection:",service->ServiceType,45)==0)
			{
				IGD_WANIPConnection_EventSink(buffer, BufferSize, service);
			}
			
		}
		IGD_WebServer_Send(sender, resp);
		if (sid != NULL){free(sid);}
	}
}

//
// Internal sink called when our attempt to unregister for events
// has gone through
//
void IGD_OnUnSubscribeSink(
void *WebReaderToken, 
int IsInterrupt, 
struct packetheader *header, 
char *buffer, 
int *BeginPointer, 
int BufferSize, 
int done, 
void *user, 
void *vcp, 
int *PAUSE)
{
	struct UPnPService *s;
	//struct IGD_CP *cp = (struct IGD_CP*)vcp;
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( buffer );
	UNREFERENCED_PARAMETER( BeginPointer );
	UNREFERENCED_PARAMETER( BufferSize );
	UNREFERENCED_PARAMETER( vcp );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done != 0)
	{
		s = (struct UPnPService*)user;
		if (header != NULL)
		{
			if (IGD_WebClientIsStatusOk(header->StatusCode))
			{
				// Successful
			}
		}
		IGD_Release(s->Parent);
	}
}


//
// Internal sink called when our attempt to register for events
// has gone through
//
void IGD_OnSubscribeSink(
void *WebReaderToken, 
int IsInterrupt, 
struct packetheader *header, 
char *buffer, 
int *BeginPointer, 
int BufferSize, 
int done, 
void *user, 
void *vcp, 
int *PAUSE)
{
	struct UPnPService *s;
	struct UPnPDevice *d;
	struct packetheader_field_node *field;
	struct parser_result *p;
	struct IGD_CP *cp = (struct IGD_CP*)vcp;
	size_t len;
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( buffer );
	UNREFERENCED_PARAMETER( BeginPointer );
	UNREFERENCED_PARAMETER( BufferSize );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done != 0)
	{
		s = (struct UPnPService*)user;
		if (header != NULL)
		{
			if (IGD_WebClientIsStatusOk(header->StatusCode))
			{
				// Successful
				field = header->FirstField;
				while (field != NULL)
				{
					if (field->FieldLength == 3 && strncasecmp(field->Field, "SID", 3) == 0 && s->SubscriptionID == NULL)
					{
						//
						// Determine what subscription id was assigned to us
						//
						len = 1 + field->FieldDataLength;
						if ((s->SubscriptionID = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
						memcpy(s->SubscriptionID, field->FieldData, len);
						//
						// Make a mapping from this SID to our service, to make our lives easier
						//
						IGD_AddEntry(cp->SIDTable, field->FieldData, field->FieldDataLength, s);
					}
					else if (field->FieldLength == 7 && strncasecmp(field->Field, "TIMEOUT", 7) == 0)
					{
						//
						// Determine what refresh cycle the device wants us to enforce
						//
						p = IGD_ParseString(field->FieldData, 0, field->FieldDataLength, "-", 1);
						p->LastResult->data[p->LastResult->datalength] = '\0';
						IGD_AddRef(s->Parent);
						d = s->Parent;
						while (d->Parent != NULL) {d = d->Parent;}
						++d->ReferenceTiedToEvents;
						IGD_LifeTime_Add(cp->LifeTimeMonitor, s, atoi(p->LastResult->data)/2, (IGD_LifeTime_OnCallback)&IGD_Renew, NULL);
						IGD_DestructParserResults(p);
					}
					field = field->NextField;
				}
			}
		}
		IGD_Release(s->Parent);
	}
}

//
// Internal Method used to renew our event subscription with a device
//
void IGD_Renew(void *state)
{
	struct UPnPService *service = (struct UPnPService*)state;
	struct UPnPDevice *d = service->Parent;
	char *IP;
	char *Path;
	unsigned short Port;
	struct packetheader *p;
	char* TempString;
	size_t len;
	
	//
	// Determine where this renewal should go
	//
	IGD_ParseUri(service->SubscriptionURL, &IP, &Port, &Path, NULL);
	p = IGD_CreateEmptyPacket();
	IGD_SetVersion(p, "1.1", 3);
	
	IGD_SetDirective(p, "SUBSCRIBE", 9, Path, (int)strlen(Path));
	
	len = (int)strlen(IP)+7;
	if ((TempString = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	snprintf(TempString, len, "%s:%d", IP, Port);
	
	IGD_AddHeaderLine(p, "HOST", 4, TempString, (int)strlen(TempString));
	free(TempString);
	
	IGD_AddHeaderLine(p, "SID", 3, service->SubscriptionID, (int)strlen(service->SubscriptionID));
	IGD_AddHeaderLine(p, "TIMEOUT", 7, "Second-180", 10);
	IGD_AddHeaderLine(p, "User-Agent", 10, IGD_PLATFORM, (int)strlen(IGD_PLATFORM));
	
	//
	// Try to refresh our subscription
	//
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequest(
	((struct IGD_CP*)service->Parent->CP)->HTTP, 
	(struct sockaddr*)&(service->Parent->LocationAddr), 
	p, 
	&IGD_OnSubscribeSink, 
	(void*)service, service->Parent->CP);
	
	while (d->Parent != NULL) {d = d->Parent;}
	--d->ReferenceTiedToEvents;
	free(IP);
	free(Path);
}

struct UPnPDevice* IGD_GetDeviceEx(struct UPnPDevice *device, char* DeviceType, int counter, int number)
{
	struct UPnPDevice *RetVal = NULL;
	struct UPnPDevice *d = device->EmbeddedDevices;
	struct parser_result *pr, *pr2;
	int DeviceTypeLength = (int)strlen(DeviceType);
	int TempLength = (int)strlen(device->DeviceType);
	
	while (d != NULL && RetVal == NULL)
	{
		RetVal = IGD_GetDeviceEx(d, DeviceType, counter, number);
		d = d->Next;
	}
	
	if (RetVal == NULL)
	{
		pr = IGD_ParseString(DeviceType, 0, DeviceTypeLength, ":", 1);
		pr2 = IGD_ParseString(device->DeviceType, 0, TempLength, ":", 1);
		
		if (DeviceTypeLength-pr->LastResult->datalength == TempLength - pr2->LastResult->datalength && atoi(pr->LastResult->data) >= atoi(pr2->LastResult->data) && memcmp(DeviceType, device->DeviceType, DeviceTypeLength-pr->LastResult->datalength) == 0)
		{
			IGD_DestructParserResults(pr);
			IGD_DestructParserResults(pr2);
			if (number == (++counter)) return(device);
		}
		IGD_DestructParserResults(pr);
		IGD_DestructParserResults(pr2);
		return(NULL);
	}
	else
	{
		return(RetVal);
	}
}

/*! \fn IGD_HasAction(struct UPnPService *s, char* action)
\brief Determines if an action exists on a service
\param s UPnP service to query
\param action action name
\returns Non-zero if it exists
*/
int IGD_HasAction(struct UPnPService *s, char* action)
{
	struct UPnPAction *a = s->Actions;
	
	while (a != NULL)
	{
		if (strcmp(action, a->Name) == 0) return(-1);
		a = a->Next;
	}
	return(0);
}

//
// Internal Trigger called when the chain is cleaning up
//
void IGD_StopCP(void *v_CP)
{
	int i;
	struct UPnPDevice *Device;
	struct IGD_CP *CP= (struct IGD_CP*)v_CP;
	void *en;
	char *key;
	int keyLength;
	void *data;
	
	
	en = IGD_HashTree_GetEnumerator(CP->DeviceTable_UDN);
	while (IGD_HashTree_MoveNext(en) == 0)
	{
		IGD_HashTree_GetValue(en, &key, &keyLength, &data);
		if (data != NULL)
		{
			// This is a UPnPDevice
			Device = (struct UPnPDevice*)data;
			if (Device->ReservedID == 0 && Device->Parent == NULL) // This is a UPnPDevice if ReservedID == 0
			{
				// This is the Root Device (Which is only in the table once)
				IGD_CP_RecursiveReleaseAndEventDevice(CP, Device);
				i = Device->ReferenceTiedToEvents;
				while (i != 0)
				{
					IGD_Release(Device);
					--i;
				}
				IGD_Release(Device);
			}
		}
	}
	IGD_HashTree_DestroyEnumerator(en);
	IGD_DestroyHashTree(CP->SIDTable);
	IGD_DestroyHashTree(CP->DeviceTable_UDN);
	IGD_DestroyHashTree(CP->DeviceTable_URI);
	IGD_DestroyHashTree(CP->DeviceTable_Tokens);
	
	if (CP->IPAddressListV4 != NULL) { free(CP->IPAddressListV4); CP->IPAddressListV4 = NULL; }
	if (CP->IPAddressListV6 != NULL) { free(CP->IPAddressListV6); CP->IPAddressListV6 = NULL; }
	
	sem_destroy(&(CP->DeviceLock));
}

/*! \fn IGD_CP_IPAddressListChanged(void *CPToken)
\brief Notifies the underlying microstack that one of the ip addresses may have changed
\param CPToken Control Point Token
*/
void IGD_CP_IPAddressListChanged(void *CPToken)
{
	if (IGD_IsChainBeingDestroyed(((struct IGD_CP*)CPToken)->Chain) == 0)
	{
		((struct IGD_CP*)CPToken)->RecheckFlag = 1;
		IGD_ForceUnBlockChain(((struct IGD_CP*)CPToken)->Chain);
	}
}

void IGD_CP_PreSelect(void *CPToken, fd_set *readset, fd_set *writeset, fd_set *errorset, int *blocktime)
{
	struct IGD_CP *CP = (struct IGD_CP*)CPToken;
	void *en;
	
	struct UPnPDevice *device;
	char *key;
	int keyLength;
	void *data;
	void *q;
	int found;
	
	// Local Address Lists
	struct sockaddr_in *IPAddressListV4 = NULL;
	int IPAddressListV4Length;
	struct sockaddr_in6 *IPAddressListV6 = NULL;
	int IPAddressListV6Length;
	
	UNREFERENCED_PARAMETER( readset );
	UNREFERENCED_PARAMETER( writeset );
	UNREFERENCED_PARAMETER( errorset );
	UNREFERENCED_PARAMETER( blocktime );
	
	//
	// Do we need to recheck IP Addresses?
	//
	if (CP->RecheckFlag != 0)
	{
		CP->RecheckFlag = 0;
		
		//
		// Get the current IP Address list
		//
		IPAddressListV4Length = IGD_GetLocalIPv4AddressList(&(IPAddressListV4), 1);
		IPAddressListV6Length = IGD_GetLocalIPv6List(&(IPAddressListV6));
		
		//
		// Create a Queue, to add devices that need to be removed
		//
		q = IGD_Queue_Create();
		
		//
		// Iterate through all the devices we are aware of
		//
		IGD_HashTree_Lock(CP->DeviceTable_UDN);
		en = IGD_HashTree_GetEnumerator(CP->DeviceTable_UDN);
		while (IGD_HashTree_MoveNext(en) == 0)
		{
			IGD_HashTree_GetValue(en, &key, &keyLength, &data);
			if (data != NULL)
			{
				// This is a UPnP Device
				device = (struct UPnPDevice*)data;
				if (device->ReservedID == 0 && device->Parent == NULL)
				{
					int i;
					// This is the root device, which is in the table exactly once
					
					// Iterate through all the current IP addresses, and check to 
					// see if there are any devices that aren't on one of these
					found = 0;
					if (device->InterfaceToHostAddr.sin6_family == AF_INET6)
					{
						// IPv6 Search
						for (i = 0; i < IPAddressListV6Length; ++i)
						{
							if (memcmp(&(IPAddressListV6[i]), device->InterfaceToHost, INET_SOCKADDR_LENGTH(AF_INET6)) == 0) { found = 1; break; }
						}
					}
					else
					{
						// IPv4 Search
						for (i = 0; i < IPAddressListV4Length; ++i)
						{
							if (memcmp(&(IPAddressListV4[i]), device->InterfaceToHost, INET_SOCKADDR_LENGTH(AF_INET)) == 0) { found = 1; break; }
						}
					}
					
					// If the device wasn't bound to any of the current IP addresses, than
					// it is no longer reachable, so we should get rid of it, and hope
					// to find it later
					if (found == 0)
					{
						// Queue Device to be removed, so we can process it outside of the lock
						IGD_Queue_EnQueue(q, device);
					}
				}
			}
		}
		
		IGD_HashTree_DestroyEnumerator(en);
		IGD_HashTree_UnLock(CP->DeviceTable_UDN);
		
		//
		// Get rid of the devices that are no longer reachable
		//
		while (IGD_Queue_PeekQueue(q) != NULL)
		{
			device = (struct UPnPDevice*)IGD_Queue_DeQueue(q);
			IGD_CP_ProcessDeviceRemoval(CP, device);
			IGD_Release(device);
		}
		IGD_Queue_Destroy(q);
		IGD_SSDP_IPAddressListChanged(CP->SSDP);
		
		if (CP->IPAddressListV4 != NULL) { free(CP->IPAddressListV4); CP->IPAddressListV4 = NULL; }
		if (CP->IPAddressListV6 != NULL) { free(CP->IPAddressListV6); CP->IPAddressListV6 = NULL; }
		
		CP->IPAddressListV4Length = IPAddressListV4Length;
		CP->IPAddressListV4 = IPAddressListV4;
		CP->IPAddressListV6Length = IPAddressListV6Length;
		CP->IPAddressListV6 = IPAddressListV6;
	}
}
void IGD_OnSessionSink(struct IGD_WebServer_Session *session, void *User)
{
	session->OnReceive = &IGD_OnEventSink;
	session->User = User;
}


#ifdef UPNP_DEBUG

void IGD_Debug(struct IGD_WebServer_Session *sender, struct UPnPDevice *dv)
{
	char tmp[25];
	
	IGD_WebServer_StreamBody(sender, "<b>", 3, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	IGD_WebServer_StreamBody(sender, dv->FriendlyName, (int)strlen(dv->FriendlyName), IGD_AsyncSocket_MemoryOwnership_USER, 0);
	IGD_WebServer_StreamBody(sender, "</b>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	IGD_WebServer_StreamBody(sender, "<br>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	IGD_WebServer_StreamBody(sender, "  LocationURL: <A HREF=\"", 24, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	IGD_WebServer_StreamBody(sender, dv->LocationURL, (int)strlen(dv->LocationURL), IGD_AsyncSocket_MemoryOwnership_USER, 0);
	IGD_WebServer_StreamBody(sender, "\">", 2, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	IGD_WebServer_StreamBody(sender, dv->LocationURL, (int)strlen(dv->LocationURL), IGD_AsyncSocket_MemoryOwnership_USER, 0);
	IGD_WebServer_StreamBody(sender, "</A>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	IGD_WebServer_StreamBody(sender, "<br>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	IGD_WebServer_StreamBody(sender, "  UDN: <A HREF=\"/UDN/", 21, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	IGD_WebServer_StreamBody(sender, dv->UDN, (int)strlen(dv->UDN), IGD_AsyncSocket_MemoryOwnership_USER, 0);
	IGD_WebServer_StreamBody(sender, "\">", 2, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	IGD_WebServer_StreamBody(sender, dv->UDN, (int)strlen(dv->UDN), IGD_AsyncSocket_MemoryOwnership_USER, 0);
	IGD_WebServer_StreamBody(sender, "</A>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	
	while (dv->Parent != NULL)
	{
		dv = dv->Parent;
	}
	IGD_WebServer_StreamBody(sender, "<br><i>  Reference Counter: ", 28, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	sprintf(tmp, "%d", dv->ReferenceCount);
	IGD_WebServer_StreamBody(sender, tmp, (int)strlen(tmp), IGD_AsyncSocket_MemoryOwnership_USER, 0);
	IGD_WebServer_StreamBody(sender, "</i>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	
	
	IGD_WebServer_StreamBody(sender, "<br><br>", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
}
void IGD_DebugOnQueryWCDO(struct IGD_WebServer_Session *sender, char *w)
{
	struct IGD_CP *cp = (struct IGD_CP*)sender->User3;
	struct packetheader *p = IGD_CreateEmptyPacket();
	char *t;
	
	IGD_SetStatusCode(p, 200, "OK", 2);
	IGD_SetVersion(p, "1.1", 3);
	IGD_AddHeaderLine(p, "Content-Type", 12, "text/html", 9);
	IGD_WebServer_StreamHeader(sender, p);
	
	IGD_WebServer_StreamBody(sender, "<HTML>", 6, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	if (w != NULL)
	{
		t = IGD_WebClient_QueryWCDO(cp->HTTP, w);
		IGD_WebServer_StreamBody(sender, t, (int)strlen(t), IGD_AsyncSocket_MemoryOwnership_CHAIN, 0);
	}
	
	IGD_WebServer_StreamBody(sender, "</HTML>", 7, IGD_AsyncSocket_MemoryOwnership_STATIC, 1);
}
void IGD_DebugOnQuery(struct IGD_WebServer_Session *sender, char *UDN, char *URI, char *TOK)
{
	struct IGD_CP *cp = (struct IGD_CP*)sender->User3;
	struct packetheader *p = IGD_CreateEmptyPacket();
	void *en;
	char *key;
	int keyLen;
	void *data;
	int rmv = 0;
	
	char *tmp;
	
	struct UPnPDevice *dv;
	
	IGD_SetStatusCode(p, 200, "OK", 2);
	IGD_SetVersion(p, "1.1", 3);
	IGD_AddHeaderLine(p, "Content-Type", 12, "text/html", 9);
	IGD_WebServer_StreamHeader(sender, p);
	
	IGD_WebServer_StreamBody(sender, "<HTML>", 6, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
	
	IGD_HashTree_Lock(cp->DeviceTable_UDN);
	
	if (UDN != NULL && stricmp(UDN, "*") == 0)
	{
		// Look in the DeviceTable_UDN
		
		en = IGD_HashTree_GetEnumerator(cp->DeviceTable_UDN);
		while (IGD_HashTree_MoveNext(en) == 0)
		{
			IGD_HashTree_GetValue(en, &key, &keyLen, &data);
			if (data != NULL)
			{
				dv = (struct UPnPDevice*)data;
				IGD_Debug(sender, dv);
			}
			else
			{
				if ((tmp = (char*)malloc(keyLen+1)) == NULL) ILIBCRITICALEXIT(254);
				tmp[keyLen]=0;
				memcpy(tmp, key, keyLen);
				IGD_WebServer_StreamBody(sender, "<b>UDN: ", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				IGD_WebServer_StreamBody(sender, tmp, keyLen, IGD_AsyncSocket_MemoryOwnership_CHAIN, 0);
				IGD_WebServer_StreamBody(sender, "</b><br><i>Not created yet</i><br>", 34, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
		}
		
		IGD_HashTree_DestroyEnumerator(en);
	}
	else if (UDN != NULL)
	{
		if (memcmp(UDN, "K", 1) == 0)
		{
			rmv=1;
			UDN = UDN+1;
		}
		dv = (struct UPnPDevice*)IGD_GetEntry(cp->DeviceTable_UDN, UDN, (int)strlen(UDN));
		if (IGD_HasEntry(cp->DeviceTable_UDN, UDN, (int)strlen(UDN)) == 0)
		{
			IGD_WebServer_StreamBody(sender, "<b>NOT FOUND</b>", 16, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
		}
		else
		{
			if (dv == NULL)
			{
				IGD_WebServer_StreamBody(sender, "<b>UDN exists, but device not created yet.</b><br>", 50, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
			else
			{
				IGD_Debug(sender, dv);
			}
			if (rmv)
			{
				IGD_DeleteEntry(cp->DeviceTable_UDN, UDN, (int)strlen(UDN));
				IGD_WebServer_StreamBody(sender, "<i>DELETED</i>", 14, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
		}
	}
	else if (URI != NULL)
	{
		if (stricmp(URI, "*") != 0)
		{
			if (memcmp(URI, "K", 1) == 0)
			{
				rmv = 1;
				URI = URI+1;
			}
			key = (char*)IGD_GetEntry(cp->DeviceTable_URI, URI, (int)strlen(URI));
			if (key == NULL)
			{
				IGD_WebServer_StreamBody(sender, "<b>NOT FOUND</b>", 16, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
			else
			{
				IGD_WebServer_StreamBody(sender, "<b>UDN: ", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				IGD_WebServer_StreamBody(sender, key, (int)strlen(key), IGD_AsyncSocket_MemoryOwnership_USER, 0);
				IGD_WebServer_StreamBody(sender, "</b><br>", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				if (rmv)
				{
					IGD_DeleteEntry(cp->DeviceTable_URI, URI, (int)strlen(URI));
					IGD_WebServer_StreamBody(sender, "<i>DELETED</i>", 14, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				}
			}
		}
		else
		{
			en = IGD_HashTree_GetEnumerator(cp->DeviceTable_URI);
			while (IGD_HashTree_MoveNext(en) == 0)
			{
				IGD_HashTree_GetValue(en, &key, &keyLen, &data);
				IGD_WebServer_StreamBody(sender, "<b>URI: ", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				IGD_WebServer_StreamBody(sender, key, (int)strlen(key), IGD_AsyncSocket_MemoryOwnership_USER, 0);
				IGD_WebServer_StreamBody(sender, "</b><br>", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				if (data == NULL)
				{
					IGD_WebServer_StreamBody(sender, "<i>No UDN</i><br>", 17, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				}
				else
				{
					IGD_WebServer_StreamBody(sender, "UDN: ", 5, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
					IGD_WebServer_StreamBody(sender, (char*)data, (int)strlen((char*)data), IGD_AsyncSocket_MemoryOwnership_USER, 0);
					IGD_WebServer_StreamBody(sender, "<br>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				}
				IGD_WebServer_StreamBody(sender, "<br>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
			IGD_HashTree_DestroyEnumerator(en);
		}
	}
	else if (TOK != NULL)
	{
		if (stricmp(TOK, "*") != 0)
		{
			key = (char*)IGD_GetEntry(cp->DeviceTable_Tokens, TOK, (int)strlen(TOK));
			if (key == NULL)
			{
				IGD_WebServer_StreamBody(sender, "<b>NOT FOUND</b>", 16, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
			else
			{
				IGD_WebServer_StreamBody(sender, "<i>Outstanding Requests</i><br>", 31, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
		}
		else
		{
			en = IGD_HashTree_GetEnumerator(cp->DeviceTable_URI);
			while (IGD_HashTree_MoveNext(en) == 0)
			{
				IGD_HashTree_GetValue(en, &key, &keyLen, &data);
				IGD_WebServer_StreamBody(sender, "<b>UDN: ", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				IGD_WebServer_StreamBody(sender, key, (int)strlen(key), IGD_AsyncSocket_MemoryOwnership_USER, 0);
				IGD_WebServer_StreamBody(sender, "</b><br>", 8, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				if (data == NULL)
				{
					IGD_WebServer_StreamBody(sender, "<i>No Tokens?</i><br>", 21, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				}
				else
				{
					IGD_WebServer_StreamBody(sender, "<i>Outstanding Requests</i><br>", 31, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
				}
				IGD_WebServer_StreamBody(sender, "<br>", 4, IGD_AsyncSocket_MemoryOwnership_STATIC, 0);
			}
		}
	}
	
	
	IGD_HashTree_UnLock(cp->DeviceTable_UDN);
	IGD_WebServer_StreamBody(sender, "</HTML>", 7, IGD_AsyncSocket_MemoryOwnership_STATIC, 1);
}
void IGD_OnDebugReceive(struct IGD_WebServer_Session *sender, int InterruptFlag, struct packetheader *header, char *bodyBuffer, int *beginPointer, int endPointer, int done)
{
	struct packetheader *r = NULL;
	if (!done){return;}
	
	header->DirectiveObj[header->DirectiveObjLength]=0;
	header->Directive[header->DirectiveLength]=0;
	
	if (stricmp(header->Directive, "GET") == 0)
	{
		if (memcmp(header->DirectiveObj, "/UDN/", 5) == 0)
		{
			IGD_DebugOnQuery(sender, header->DirectiveObj+5, NULL, NULL);
		}
		else if (memcmp(header->DirectiveObj, "/URI/", 5) == 0)
		{
			IGD_DebugOnQuery(sender, NULL, header->DirectiveObj+5, NULL);
		}
		else if (memcmp(header->DirectiveObj, "/TOK/", 5) == 0)
		{
			IGD_DebugOnQuery(sender, NULL, NULL, header->DirectiveObj+5);
		}
		else if (memcmp(header->DirectiveObj, "/WCDO/", 6) == 0)
		{
			IGD_DebugOnQueryWCDO(sender, header->DirectiveObj+6);
		}
		else
		{
			r = IGD_CreateEmptyPacket();
			IGD_SetStatusCode(r, 404, "Bad Request", 11);
			IGD_WebServer_Send(sender, r);
		}
	}
}

void IGD_OnDebugSessionSink(struct IGD_WebServer_Session *sender, void *user)
{
	sender->OnReceive = &IGD_OnDebugReceive;
	sender->User3 = user;
}
#endif
void IGD_ControlPoint_AddDiscoveryErrorHandler(void *cpToken, UPnPDeviceDiscoveryErrorHandler callback)
{
	struct IGD_CP *cp = (struct IGD_CP*)cpToken;
	cp->ErrorDispatch = callback;
}
/*! \fn IGD_CreateControlPoint(void *Chain, void(*A)(struct UPnPDevice*), void(*R)(struct UPnPDevice*))
\brief Initalizes the control point
\param Chain The chain to attach this CP to
\param A AddSink Function Pointer
\param R RemoveSink Function Pointer
\returns ControlPoint Token
*/
void *IGD_CreateControlPoint(void *Chain, void(*A)(struct UPnPDevice*), void(*R)(struct UPnPDevice*))
{
	struct IGD_CP *cp;
	if ((cp = (struct IGD_CP*)malloc(sizeof(struct IGD_CP))) == NULL) ILIBCRITICALEXIT(254);
	
	memset(cp, 0, sizeof(struct IGD_CP));
	cp->Destroy = &IGD_StopCP;
	cp->PostSelect = NULL;
	cp->PreSelect = &IGD_CP_PreSelect;
	cp->DiscoverSink = A;
	cp->RemoveSink = R;
	
	sem_init(&(cp->DeviceLock), 0, 1);
	cp->WebServer = IGD_WebServer_Create(Chain, 5, 0, &IGD_OnSessionSink, cp);
	cp->SIDTable = IGD_InitHashTree();
	cp->DeviceTable_UDN = IGD_InitHashTree();
	cp->DeviceTable_URI = IGD_InitHashTree();
	cp->DeviceTable_Tokens = IGD_InitHashTree();
	#ifdef UPNP_DEBUG
	IGD_WebServer_Create(Chain, 2, 7575, &IGD_OnDebugSessionSink, cp);
	#endif
	
	cp->SSDP = IGD_CreateSSDPClientModule(Chain,"urn:schemas-upnp-org:device:WANConnectionDevice:1", 49, &IGD_SSDP_Sink,cp);
	
	cp->HTTP = IGD_CreateWebClient(5, Chain);
	IGD_AddToChain(Chain, cp);
	cp->LifeTimeMonitor = (struct LifeTimeMonitorStruct*)IGD_GetBaseTimer(Chain);
	
	cp->Chain = Chain;
	cp->RecheckFlag = 0;
	cp->IPAddressListV4Length = IGD_GetLocalIPv4AddressList(&(cp->IPAddressListV4), 1);
	cp->IPAddressListV6Length = IGD_GetLocalIPv6List(&(cp->IPAddressListV6));
	
	return((void*)cp);
}

void IGD_Invoke_WANIPConnection_AddPortMapping_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	UNREFERENCED_PARAMETER(WebReaderToken);
	UNREFERENCED_PARAMETER(IsInterrupt);
	UNREFERENCED_PARAMETER(PAUSE);
	if (done == 0) return;
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,-1,_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,IGD_GetErrorCode(buffer,EndPointer-(*p_BeginPointer)),_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User);
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_DeletePortMapping_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	UNREFERENCED_PARAMETER(WebReaderToken);
	UNREFERENCED_PARAMETER(IsInterrupt);
	UNREFERENCED_PARAMETER(PAUSE);
	if (done == 0) return;
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,-1,_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,IGD_GetErrorCode(buffer,EndPointer-(*p_BeginPointer)),_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User);
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_ForceTermination_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	UNREFERENCED_PARAMETER(WebReaderToken);
	UNREFERENCED_PARAMETER(IsInterrupt);
	UNREFERENCED_PARAMETER(PAUSE);
	if (done == 0) return;
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,-1,_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,IGD_GetErrorCode(buffer,EndPointer-(*p_BeginPointer)),_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User);
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_GetConnectionTypeInfo_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	int ArgLeft = 2;
	struct IGD_XMLNode *xml;
	struct IGD_XMLNode *__xml;
	char *tempBuffer;
	int tempBufferLength;
	char* NewConnectionType = NULL;
	char* NewPossibleConnectionTypes = NULL;
	LVL3DEBUG(char *DebugBuffer;)
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done == 0) return;
	LVL3DEBUG(if ((DebugBuffer = (char*)malloc(EndPointer + 1)) == NULL) ILIBCRITICALEXIT(254);)
	LVL3DEBUG(memcpy(DebugBuffer,buffer,EndPointer);)
	LVL3DEBUG(DebugBuffer[EndPointer]=0;)
	LVL3DEBUG(printf("\r\n SOAP Recieved:\r\n%s\r\n",DebugBuffer);)
	LVL3DEBUG(free(DebugBuffer);)
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*,char*,char*))_InvokeData->CallbackPtr)(Service,IsInterrupt==0?-1:IsInterrupt,_InvokeData->User,INVALID_DATA,INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*, char*, char*))_InvokeData->CallbackPtr)(Service, IGD_GetErrorCode(buffer, EndPointer-(*p_BeginPointer)), _InvokeData->User, INVALID_DATA, INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	__xml = xml = IGD_ParseXML(buffer,0,EndPointer-(*p_BeginPointer));
	if (IGD_ProcessXMLNodeList(xml)==0)
	{
		while(xml != NULL)
		{
			if (xml->NameLength == 29 && memcmp(xml->Name, "GetConnectionTypeInfoResponse", 29) == 0)
			{
				xml = xml->Next;
				while(xml != NULL)
				{
					if (xml->NameLength == 17 && memcmp(xml->Name, "NewConnectionType", 17) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewConnectionType = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewConnectionType);
						}
					}
					else 
					if (xml->NameLength == 26 && memcmp(xml->Name, "NewPossibleConnectionTypes", 26) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewPossibleConnectionTypes = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewPossibleConnectionTypes);
						}
					}
					xml = xml->Peer;
				}
			}
			if (xml!=NULL) {xml = xml->Next;}
		}
		IGD_DestructXMLNodeList(__xml);
	}
	
	if (ArgLeft!=0)
	{
		((void (*)(struct UPnPService*,int,void*,char*,char*))_InvokeData->CallbackPtr)(Service,-2,_InvokeData->User,INVALID_DATA,INVALID_DATA);
	}
	else
	{
		((void (*)(struct UPnPService*,int,void*,char*,char*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User,NewConnectionType,NewPossibleConnectionTypes);
	}
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_GetExternalIPAddress_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	int ArgLeft = 1;
	struct IGD_XMLNode *xml;
	struct IGD_XMLNode *__xml;
	char *tempBuffer;
	int tempBufferLength;
	char* NewExternalIPAddress = NULL;
	LVL3DEBUG(char *DebugBuffer;)
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done == 0) return;
	LVL3DEBUG(if ((DebugBuffer = (char*)malloc(EndPointer + 1)) == NULL) ILIBCRITICALEXIT(254);)
	LVL3DEBUG(memcpy(DebugBuffer,buffer,EndPointer);)
	LVL3DEBUG(DebugBuffer[EndPointer]=0;)
	LVL3DEBUG(printf("\r\n SOAP Recieved:\r\n%s\r\n",DebugBuffer);)
	LVL3DEBUG(free(DebugBuffer);)
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*,char*))_InvokeData->CallbackPtr)(Service,IsInterrupt==0?-1:IsInterrupt,_InvokeData->User,INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*, char*))_InvokeData->CallbackPtr)(Service, IGD_GetErrorCode(buffer, EndPointer-(*p_BeginPointer)), _InvokeData->User, INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	__xml = xml = IGD_ParseXML(buffer,0,EndPointer-(*p_BeginPointer));
	if (IGD_ProcessXMLNodeList(xml)==0)
	{
		while(xml != NULL)
		{
			if (xml->NameLength == 28 && memcmp(xml->Name, "GetExternalIPAddressResponse", 28) == 0)
			{
				xml = xml->Next;
				while(xml != NULL)
				{
					if (xml->NameLength == 20 && memcmp(xml->Name, "NewExternalIPAddress", 20) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewExternalIPAddress = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewExternalIPAddress);
						}
					}
					xml = xml->Peer;
				}
			}
			if (xml!=NULL) {xml = xml->Next;}
		}
		IGD_DestructXMLNodeList(__xml);
	}
	
	if (ArgLeft!=0)
	{
		((void (*)(struct UPnPService*,int,void*,char*))_InvokeData->CallbackPtr)(Service,-2,_InvokeData->User,INVALID_DATA);
	}
	else
	{
		((void (*)(struct UPnPService*,int,void*,char*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User,NewExternalIPAddress);
	}
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_GetGenericPortMappingEntry_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	int ArgLeft = 8;
	struct IGD_XMLNode *xml;
	struct IGD_XMLNode *__xml;
	char *tempBuffer;
	int tempBufferLength;
	unsigned long TempULong;
	char* NewRemoteHost = NULL;
	unsigned short NewExternalPort = 0;
	char* NewProtocol = NULL;
	unsigned short NewInternalPort = 0;
	char* NewInternalClient = NULL;
	int NewEnabled = 0;
	char* NewPortMappingDescription = NULL;
	unsigned int NewLeaseDuration = 0;
	LVL3DEBUG(char *DebugBuffer;)
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done == 0) return;
	LVL3DEBUG(if ((DebugBuffer = (char*)malloc(EndPointer + 1)) == NULL) ILIBCRITICALEXIT(254);)
	LVL3DEBUG(memcpy(DebugBuffer,buffer,EndPointer);)
	LVL3DEBUG(DebugBuffer[EndPointer]=0;)
	LVL3DEBUG(printf("\r\n SOAP Recieved:\r\n%s\r\n",DebugBuffer);)
	LVL3DEBUG(free(DebugBuffer);)
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*,char*,unsigned short,char*,unsigned short,char*,int,char*,unsigned int))_InvokeData->CallbackPtr)(Service,IsInterrupt==0?-1:IsInterrupt,_InvokeData->User,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*, char*, unsigned short, char*, unsigned short, char*, int, char*, unsigned int))_InvokeData->CallbackPtr)(Service, IGD_GetErrorCode(buffer, EndPointer-(*p_BeginPointer)), _InvokeData->User, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	__xml = xml = IGD_ParseXML(buffer,0,EndPointer-(*p_BeginPointer));
	if (IGD_ProcessXMLNodeList(xml)==0)
	{
		while(xml != NULL)
		{
			if (xml->NameLength == 34 && memcmp(xml->Name, "GetGenericPortMappingEntryResponse", 34) == 0)
			{
				xml = xml->Next;
				while(xml != NULL)
				{
					if (xml->NameLength == 13 && memcmp(xml->Name, "NewRemoteHost", 13) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewRemoteHost = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewRemoteHost);
						}
					}
					else 
					if (xml->NameLength == 15 && memcmp(xml->Name, "NewExternalPort", 15) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (IGD_GetULong(tempBuffer,tempBufferLength,&TempULong)==0)
						{
							NewExternalPort = (unsigned short) TempULong;
						}
					}
					else 
					if (xml->NameLength == 11 && memcmp(xml->Name, "NewProtocol", 11) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewProtocol = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewProtocol);
						}
					}
					else 
					if (xml->NameLength == 15 && memcmp(xml->Name, "NewInternalPort", 15) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (IGD_GetULong(tempBuffer,tempBufferLength,&TempULong)==0)
						{
							NewInternalPort = (unsigned short) TempULong;
						}
					}
					else 
					if (xml->NameLength == 17 && memcmp(xml->Name, "NewInternalClient", 17) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewInternalClient = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewInternalClient);
						}
					}
					else 
					if (xml->NameLength == 10 && memcmp(xml->Name, "NewEnabled", 10) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						NewEnabled = 1;
						if (strncasecmp(tempBuffer, "false", 5)==0 || strncmp(tempBuffer, "0", 1)==0 || strncasecmp(tempBuffer, "no", 2) == 0)
						{
							NewEnabled = 0;
						}
					}
					else 
					if (xml->NameLength == 25 && memcmp(xml->Name, "NewPortMappingDescription", 25) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewPortMappingDescription = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewPortMappingDescription);
						}
					}
					else 
					if (xml->NameLength == 16 && memcmp(xml->Name, "NewLeaseDuration", 16) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (IGD_GetULong(tempBuffer,tempBufferLength,&TempULong)==0)
						{
							NewLeaseDuration = (unsigned int) TempULong;
						}
					}
					xml = xml->Peer;
				}
			}
			if (xml!=NULL) {xml = xml->Next;}
		}
		IGD_DestructXMLNodeList(__xml);
	}
	
	if (ArgLeft!=0)
	{
		((void (*)(struct UPnPService*,int,void*,char*,unsigned short,char*,unsigned short,char*,int,char*,unsigned int))_InvokeData->CallbackPtr)(Service,-2,_InvokeData->User,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA);
	}
	else
	{
		((void (*)(struct UPnPService*,int,void*,char*,unsigned short,char*,unsigned short,char*,int,char*,unsigned int))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User,NewRemoteHost,NewExternalPort,NewProtocol,NewInternalPort,NewInternalClient,NewEnabled,NewPortMappingDescription,NewLeaseDuration);
	}
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_GetNATRSIPStatus_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	int ArgLeft = 2;
	struct IGD_XMLNode *xml;
	struct IGD_XMLNode *__xml;
	char *tempBuffer;
	int tempBufferLength;
	int NewRSIPAvailable = 0;
	int NewNATEnabled = 0;
	LVL3DEBUG(char *DebugBuffer;)
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done == 0) return;
	LVL3DEBUG(if ((DebugBuffer = (char*)malloc(EndPointer + 1)) == NULL) ILIBCRITICALEXIT(254);)
	LVL3DEBUG(memcpy(DebugBuffer,buffer,EndPointer);)
	LVL3DEBUG(DebugBuffer[EndPointer]=0;)
	LVL3DEBUG(printf("\r\n SOAP Recieved:\r\n%s\r\n",DebugBuffer);)
	LVL3DEBUG(free(DebugBuffer);)
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*,int,int))_InvokeData->CallbackPtr)(Service,IsInterrupt==0?-1:IsInterrupt,_InvokeData->User,INVALID_DATA,INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*, int, int))_InvokeData->CallbackPtr)(Service, IGD_GetErrorCode(buffer, EndPointer-(*p_BeginPointer)), _InvokeData->User, INVALID_DATA, INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	__xml = xml = IGD_ParseXML(buffer,0,EndPointer-(*p_BeginPointer));
	if (IGD_ProcessXMLNodeList(xml)==0)
	{
		while(xml != NULL)
		{
			if (xml->NameLength == 24 && memcmp(xml->Name, "GetNATRSIPStatusResponse", 24) == 0)
			{
				xml = xml->Next;
				while(xml != NULL)
				{
					if (xml->NameLength == 16 && memcmp(xml->Name, "NewRSIPAvailable", 16) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						NewRSIPAvailable = 1;
						if (strncasecmp(tempBuffer, "false", 5)==0 || strncmp(tempBuffer, "0", 1)==0 || strncasecmp(tempBuffer, "no", 2) == 0)
						{
							NewRSIPAvailable = 0;
						}
					}
					else 
					if (xml->NameLength == 13 && memcmp(xml->Name, "NewNATEnabled", 13) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						NewNATEnabled = 1;
						if (strncasecmp(tempBuffer, "false", 5)==0 || strncmp(tempBuffer, "0", 1)==0 || strncasecmp(tempBuffer, "no", 2) == 0)
						{
							NewNATEnabled = 0;
						}
					}
					xml = xml->Peer;
				}
			}
			if (xml!=NULL) {xml = xml->Next;}
		}
		IGD_DestructXMLNodeList(__xml);
	}
	
	if (ArgLeft!=0)
	{
		((void (*)(struct UPnPService*,int,void*,int,int))_InvokeData->CallbackPtr)(Service,-2,_InvokeData->User,INVALID_DATA,INVALID_DATA);
	}
	else
	{
		((void (*)(struct UPnPService*,int,void*,int,int))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User,NewRSIPAvailable,NewNATEnabled);
	}
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_GetSpecificPortMappingEntry_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	int ArgLeft = 5;
	struct IGD_XMLNode *xml;
	struct IGD_XMLNode *__xml;
	char *tempBuffer;
	int tempBufferLength;
	unsigned long TempULong;
	unsigned short NewInternalPort = 0;
	char* NewInternalClient = NULL;
	int NewEnabled = 0;
	char* NewPortMappingDescription = NULL;
	unsigned int NewLeaseDuration = 0;
	LVL3DEBUG(char *DebugBuffer;)
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done == 0) return;
	LVL3DEBUG(if ((DebugBuffer = (char*)malloc(EndPointer + 1)) == NULL) ILIBCRITICALEXIT(254);)
	LVL3DEBUG(memcpy(DebugBuffer,buffer,EndPointer);)
	LVL3DEBUG(DebugBuffer[EndPointer]=0;)
	LVL3DEBUG(printf("\r\n SOAP Recieved:\r\n%s\r\n",DebugBuffer);)
	LVL3DEBUG(free(DebugBuffer);)
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*,unsigned short,char*,int,char*,unsigned int))_InvokeData->CallbackPtr)(Service,IsInterrupt==0?-1:IsInterrupt,_InvokeData->User,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*, unsigned short, char*, int, char*, unsigned int))_InvokeData->CallbackPtr)(Service, IGD_GetErrorCode(buffer, EndPointer-(*p_BeginPointer)), _InvokeData->User, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA, INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	__xml = xml = IGD_ParseXML(buffer,0,EndPointer-(*p_BeginPointer));
	if (IGD_ProcessXMLNodeList(xml)==0)
	{
		while(xml != NULL)
		{
			if (xml->NameLength == 35 && memcmp(xml->Name, "GetSpecificPortMappingEntryResponse", 35) == 0)
			{
				xml = xml->Next;
				while(xml != NULL)
				{
					if (xml->NameLength == 15 && memcmp(xml->Name, "NewInternalPort", 15) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (IGD_GetULong(tempBuffer,tempBufferLength,&TempULong)==0)
						{
							NewInternalPort = (unsigned short) TempULong;
						}
					}
					else 
					if (xml->NameLength == 17 && memcmp(xml->Name, "NewInternalClient", 17) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewInternalClient = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewInternalClient);
						}
					}
					else 
					if (xml->NameLength == 10 && memcmp(xml->Name, "NewEnabled", 10) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						NewEnabled = 1;
						if (strncasecmp(tempBuffer, "false", 5)==0 || strncmp(tempBuffer, "0", 1)==0 || strncasecmp(tempBuffer, "no", 2) == 0)
						{
							NewEnabled = 0;
						}
					}
					else 
					if (xml->NameLength == 25 && memcmp(xml->Name, "NewPortMappingDescription", 25) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewPortMappingDescription = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewPortMappingDescription);
						}
					}
					else 
					if (xml->NameLength == 16 && memcmp(xml->Name, "NewLeaseDuration", 16) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (IGD_GetULong(tempBuffer,tempBufferLength,&TempULong)==0)
						{
							NewLeaseDuration = (unsigned int) TempULong;
						}
					}
					xml = xml->Peer;
				}
			}
			if (xml!=NULL) {xml = xml->Next;}
		}
		IGD_DestructXMLNodeList(__xml);
	}
	
	if (ArgLeft!=0)
	{
		((void (*)(struct UPnPService*,int,void*,unsigned short,char*,int,char*,unsigned int))_InvokeData->CallbackPtr)(Service,-2,_InvokeData->User,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA,INVALID_DATA);
	}
	else
	{
		((void (*)(struct UPnPService*,int,void*,unsigned short,char*,int,char*,unsigned int))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User,NewInternalPort,NewInternalClient,NewEnabled,NewPortMappingDescription,NewLeaseDuration);
	}
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_GetStatusInfo_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	int ArgLeft = 3;
	struct IGD_XMLNode *xml;
	struct IGD_XMLNode *__xml;
	char *tempBuffer;
	int tempBufferLength;
	unsigned long TempULong;
	char* NewConnectionStatus = NULL;
	char* NewLastConnectionError = NULL;
	unsigned int NewUptime = 0;
	LVL3DEBUG(char *DebugBuffer;)
	
	UNREFERENCED_PARAMETER( WebReaderToken );
	UNREFERENCED_PARAMETER( IsInterrupt );
	UNREFERENCED_PARAMETER( PAUSE );
	
	if (done == 0) return;
	LVL3DEBUG(if ((DebugBuffer = (char*)malloc(EndPointer + 1)) == NULL) ILIBCRITICALEXIT(254);)
	LVL3DEBUG(memcpy(DebugBuffer,buffer,EndPointer);)
	LVL3DEBUG(DebugBuffer[EndPointer]=0;)
	LVL3DEBUG(printf("\r\n SOAP Recieved:\r\n%s\r\n",DebugBuffer);)
	LVL3DEBUG(free(DebugBuffer);)
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*,char*,char*,unsigned int))_InvokeData->CallbackPtr)(Service,IsInterrupt==0?-1:IsInterrupt,_InvokeData->User,INVALID_DATA,INVALID_DATA,INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*, char*, char*, unsigned int))_InvokeData->CallbackPtr)(Service, IGD_GetErrorCode(buffer, EndPointer-(*p_BeginPointer)), _InvokeData->User, INVALID_DATA, INVALID_DATA, INVALID_DATA);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	__xml = xml = IGD_ParseXML(buffer,0,EndPointer-(*p_BeginPointer));
	if (IGD_ProcessXMLNodeList(xml)==0)
	{
		while(xml != NULL)
		{
			if (xml->NameLength == 21 && memcmp(xml->Name, "GetStatusInfoResponse", 21) == 0)
			{
				xml = xml->Next;
				while(xml != NULL)
				{
					if (xml->NameLength == 19 && memcmp(xml->Name, "NewConnectionStatus", 19) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewConnectionStatus = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewConnectionStatus);
						}
					}
					else 
					if (xml->NameLength == 22 && memcmp(xml->Name, "NewLastConnectionError", 22) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (tempBufferLength != 0)
						{
							tempBuffer[tempBufferLength] = '\0';
							NewLastConnectionError = tempBuffer;
							IGD_InPlaceXmlUnEscape(NewLastConnectionError);
						}
					}
					else 
					if (xml->NameLength == 9 && memcmp(xml->Name, "NewUptime", 9) == 0)
					{
						--ArgLeft;
						tempBufferLength = IGD_ReadInnerXML(xml, &tempBuffer);
						if (IGD_GetULong(tempBuffer,tempBufferLength,&TempULong)==0)
						{
							NewUptime = (unsigned int) TempULong;
						}
					}
					xml = xml->Peer;
				}
			}
			if (xml!=NULL) {xml = xml->Next;}
		}
		IGD_DestructXMLNodeList(__xml);
	}
	
	if (ArgLeft!=0)
	{
		((void (*)(struct UPnPService*,int,void*,char*,char*,unsigned int))_InvokeData->CallbackPtr)(Service,-2,_InvokeData->User,INVALID_DATA,INVALID_DATA,INVALID_DATA);
	}
	else
	{
		((void (*)(struct UPnPService*,int,void*,char*,char*,unsigned int))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User,NewConnectionStatus,NewLastConnectionError,NewUptime);
	}
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_RequestConnection_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	UNREFERENCED_PARAMETER(WebReaderToken);
	UNREFERENCED_PARAMETER(IsInterrupt);
	UNREFERENCED_PARAMETER(PAUSE);
	if (done == 0) return;
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,-1,_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,IGD_GetErrorCode(buffer,EndPointer-(*p_BeginPointer)),_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User);
	IGD_Release(Service->Parent);
	free(_InvokeData);
}
void IGD_Invoke_WANIPConnection_SetConnectionType_Sink(
void *WebReaderToken,
int IsInterrupt,
struct packetheader *header,
char *buffer,
int *p_BeginPointer,
int EndPointer,
int done,
void *_service,
void *state,
int *PAUSE)
{
	struct UPnPService *Service = (struct UPnPService*)_service;
	struct InvokeStruct *_InvokeData = (struct InvokeStruct*)state;
	UNREFERENCED_PARAMETER(WebReaderToken);
	UNREFERENCED_PARAMETER(IsInterrupt);
	UNREFERENCED_PARAMETER(PAUSE);
	if (done == 0) return;
	if (_InvokeData->CallbackPtr == NULL)
	{
		IGD_Release(Service->Parent);
		free(_InvokeData);
		return;
	}
	else
	{
		if (header == NULL)
		{
			/* Connection Failed */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,-1,_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
		else if (!IGD_WebClientIsStatusOk(header->StatusCode))
		{
			/* SOAP Fault */
			((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,IGD_GetErrorCode(buffer,EndPointer-(*p_BeginPointer)),_InvokeData->User);
			IGD_Release(Service->Parent);
			free(_InvokeData);
			return;
		}
	}
	
	((void (*)(struct UPnPService*,int,void*))_InvokeData->CallbackPtr)(Service,0,_InvokeData->User);
	IGD_Release(Service->Parent);
	free(_InvokeData);
}

/*! \fn IGD_Invoke_WANIPConnection_AddPortMapping(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void *_user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol, unsigned short NewInternalPort, char* unescaped_NewInternalClient, int NewEnabled, char* unescaped_NewPortMappingDescription, unsigned int NewLeaseDuration)
\brief Invokes the AddPortMapping action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
\param unescaped_NewRemoteHost Value of the NewRemoteHost parameter.  <b>Automatically</b> escaped
\param NewExternalPort Value of the NewExternalPort parameter. 	\param unescaped_NewProtocol Value of the NewProtocol parameter.  <b>Automatically</b> escaped
\param NewInternalPort Value of the NewInternalPort parameter. 	\param unescaped_NewInternalClient Value of the NewInternalClient parameter.  <b>Automatically</b> escaped
\param NewEnabled Value of the NewEnabled parameter. 	\param unescaped_NewPortMappingDescription Value of the NewPortMappingDescription parameter.  <b>Automatically</b> escaped
\param NewLeaseDuration Value of the NewLeaseDuration parameter. */
void IGD_Invoke_WANIPConnection_AddPortMapping(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void* user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol, unsigned short NewInternalPort, char* unescaped_NewInternalClient, int NewEnabled, char* unescaped_NewPortMappingDescription, unsigned int NewLeaseDuration)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	char* NewRemoteHost;
	char* NewProtocol;
	char* NewInternalClient;
	int __NewEnabled;
	char* NewPortMappingDescription;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	(NewEnabled!=0)?(__NewEnabled=1):(__NewEnabled=0);
	if ((NewRemoteHost = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewRemoteHost))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewRemoteHost,unescaped_NewRemoteHost);
	if ((NewProtocol = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewProtocol))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewProtocol,unescaped_NewProtocol);
	if ((NewInternalClient = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewInternalClient))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewInternalClient,unescaped_NewInternalClient);
	if ((NewPortMappingDescription = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewPortMappingDescription))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewPortMappingDescription,unescaped_NewPortMappingDescription);
	len = (int)strlen(service->ServiceType)+(int)strlen(NewRemoteHost)+(int)strlen(NewProtocol)+(int)strlen(NewInternalClient)+(int)strlen(NewPortMappingDescription)+569;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sAddPortMapping xmlns:u=\"%s\"><NewRemoteHost>%s</NewRemoteHost><NewExternalPort>%u</NewExternalPort><NewProtocol>%s</NewProtocol><NewInternalPort>%u</NewInternalPort><NewInternalClient>%s</NewInternalClient><NewEnabled>%d</NewEnabled><NewPortMappingDescription>%s</NewPortMappingDescription><NewLeaseDuration>%u</NewLeaseDuration></u:AddPortMapping%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, NewRemoteHost, NewExternalPort, NewProtocol, NewInternalPort, NewInternalClient, __NewEnabled, NewPortMappingDescription, NewLeaseDuration, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	free(NewRemoteHost);
	free(NewProtocol);
	free(NewInternalClient);
	free(NewPortMappingDescription);
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 171 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "AddPortMapping", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
		((struct IGD_CP*)service->Parent->CP)->HTTP,
		(struct sockaddr*)&(service->Parent->LocationAddr),
		headerBuffer,
		headerLength,
		0,
		buffer,
		bufferLength,
		0,
		&IGD_Invoke_WANIPConnection_AddPortMapping_Sink,
		service,
		invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_DeletePortMapping(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void *_user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol)
\brief Invokes the DeletePortMapping action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
\param unescaped_NewRemoteHost Value of the NewRemoteHost parameter.  <b>Automatically</b> escaped
\param NewExternalPort Value of the NewExternalPort parameter. 	\param unescaped_NewProtocol Value of the NewProtocol parameter.  <b>Automatically</b> escaped
*/
void IGD_Invoke_WANIPConnection_DeletePortMapping(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void* user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	char* NewRemoteHost;
	char* NewProtocol;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	if ((NewRemoteHost = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewRemoteHost))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewRemoteHost,unescaped_NewRemoteHost);
	if ((NewProtocol = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewProtocol))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewProtocol,unescaped_NewProtocol);
	len = (int)strlen(service->ServiceType)+(int)strlen(NewRemoteHost)+(int)strlen(NewProtocol)+364;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sDeletePortMapping xmlns:u=\"%s\"><NewRemoteHost>%s</NewRemoteHost><NewExternalPort>%u</NewExternalPort><NewProtocol>%s</NewProtocol></u:DeletePortMapping%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, NewRemoteHost, NewExternalPort, NewProtocol, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	free(NewRemoteHost);
	free(NewProtocol);
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 174 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "DeletePortMapping", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_DeletePortMapping_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_ForceTermination(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void *_user)
\brief Invokes the ForceTermination action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
*/
void IGD_Invoke_WANIPConnection_ForceTermination(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void* user)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+264;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sForceTermination xmlns:u=\"%s\"></u:ForceTermination%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 173 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "ForceTermination", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_ForceTermination_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_GetConnectionTypeInfo(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionType,char* NewPossibleConnectionTypes), void *_user)
\brief Invokes the GetConnectionTypeInfo action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
*/
void IGD_Invoke_WANIPConnection_GetConnectionTypeInfo(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionType,char* NewPossibleConnectionTypes), void* user)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+274;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sGetConnectionTypeInfo xmlns:u=\"%s\"></u:GetConnectionTypeInfo%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 178 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "GetConnectionTypeInfo", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_GetConnectionTypeInfo_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_GetExternalIPAddress(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewExternalIPAddress), void *_user)
\brief Invokes the GetExternalIPAddress action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
*/
void IGD_Invoke_WANIPConnection_GetExternalIPAddress(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewExternalIPAddress), void* user)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+272;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sGetExternalIPAddress xmlns:u=\"%s\"></u:GetExternalIPAddress%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 177 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "GetExternalIPAddress", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_GetExternalIPAddress_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_GetGenericPortMappingEntry(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewRemoteHost,unsigned short NewExternalPort,char* NewProtocol,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration), void *_user, unsigned short NewPortMappingIndex)
\brief Invokes the GetGenericPortMappingEntry action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
\param NewPortMappingIndex Value of the NewPortMappingIndex parameter. */
void IGD_Invoke_WANIPConnection_GetGenericPortMappingEntry(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewRemoteHost,unsigned short NewExternalPort,char* NewProtocol,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration), void* user, unsigned short NewPortMappingIndex)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+332;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sGetGenericPortMappingEntry xmlns:u=\"%s\"><NewPortMappingIndex>%u</NewPortMappingIndex></u:GetGenericPortMappingEntry%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, NewPortMappingIndex, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 183 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "GetGenericPortMappingEntry", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_GetGenericPortMappingEntry_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_GetNATRSIPStatus(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,int NewRSIPAvailable,int NewNATEnabled), void *_user)
\brief Invokes the GetNATRSIPStatus action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
*/
void IGD_Invoke_WANIPConnection_GetNATRSIPStatus(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,int NewRSIPAvailable,int NewNATEnabled), void* user)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+264;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sGetNATRSIPStatus xmlns:u=\"%s\"></u:GetNATRSIPStatus%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 173 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "GetNATRSIPStatus", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_GetNATRSIPStatus_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_GetSpecificPortMappingEntry(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration), void *_user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol)
\brief Invokes the GetSpecificPortMappingEntry action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
\param unescaped_NewRemoteHost Value of the NewRemoteHost parameter.  <b>Automatically</b> escaped
\param NewExternalPort Value of the NewExternalPort parameter. 	\param unescaped_NewProtocol Value of the NewProtocol parameter.  <b>Automatically</b> escaped
*/
void IGD_Invoke_WANIPConnection_GetSpecificPortMappingEntry(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration), void* user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	char* NewRemoteHost;
	char* NewProtocol;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	if ((NewRemoteHost = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewRemoteHost))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewRemoteHost,unescaped_NewRemoteHost);
	if ((NewProtocol = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewProtocol))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewProtocol,unescaped_NewProtocol);
	len = (int)strlen(service->ServiceType)+(int)strlen(NewRemoteHost)+(int)strlen(NewProtocol)+384;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sGetSpecificPortMappingEntry xmlns:u=\"%s\"><NewRemoteHost>%s</NewRemoteHost><NewExternalPort>%u</NewExternalPort><NewProtocol>%s</NewProtocol></u:GetSpecificPortMappingEntry%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, NewRemoteHost, NewExternalPort, NewProtocol, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	free(NewRemoteHost);
	free(NewProtocol);
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 184 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "GetSpecificPortMappingEntry", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_GetSpecificPortMappingEntry_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_GetStatusInfo(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionStatus,char* NewLastConnectionError,unsigned int NewUptime), void *_user)
\brief Invokes the GetStatusInfo action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
*/
void IGD_Invoke_WANIPConnection_GetStatusInfo(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionStatus,char* NewLastConnectionError,unsigned int NewUptime), void* user)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+258;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sGetStatusInfo xmlns:u=\"%s\"></u:GetStatusInfo%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 170 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "GetStatusInfo", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_GetStatusInfo_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_RequestConnection(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void *_user)
\brief Invokes the RequestConnection action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
*/
void IGD_Invoke_WANIPConnection_RequestConnection(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void* user)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	len = (int)strlen(service->ServiceType)+266;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sRequestConnection xmlns:u=\"%s\"></u:RequestConnection%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 174 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "RequestConnection", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_RequestConnection_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}
/*! \fn IGD_Invoke_WANIPConnection_SetConnectionType(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void *_user, char* unescaped_NewConnectionType)
\brief Invokes the SetConnectionType action in the WANIPConnection service
\param service The UPnPService instance to invoke the action on
\param CallbackPtr The function pointer to be called when the invocation returns
\param unescaped_NewConnectionType Value of the NewConnectionType parameter.  <b>Automatically</b> escaped
*/
void IGD_Invoke_WANIPConnection_SetConnectionType(struct UPnPService *service,void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user), void* user, char* unescaped_NewConnectionType)
{
	int headerLength;
	char *headerBuffer;
	char *SoapBodyTemplate;
	char* buffer;
	int bufferLength;
	char* IP;
	unsigned short Port;
	char* Path;
	size_t len;
	struct InvokeStruct *invoke_data;
	char* NewConnectionType;
	if ((invoke_data = (struct InvokeStruct*)malloc(sizeof(struct InvokeStruct))) == NULL) ILIBCRITICALEXIT(254);
	
	if (service == NULL)
	{
		free(invoke_data);
		return;
	}
	
	
	if ((NewConnectionType = (char*)malloc(1 + IGD_XmlEscapeLength(unescaped_NewConnectionType))) == NULL) ILIBCRITICALEXIT(254);
	IGD_XmlEscape(NewConnectionType,unescaped_NewConnectionType);
	len = (int)strlen(service->ServiceType)+(int)strlen(NewConnectionType)+305;
	if ((buffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	SoapBodyTemplate = "%sSetConnectionType xmlns:u=\"%s\"><NewConnectionType>%s</NewConnectionType></u:SetConnectionType%s";
	bufferLength = snprintf(buffer, len, SoapBodyTemplate, UPNPCP_SOAP_BodyHead, service->ServiceType, NewConnectionType, UPNPCP_SOAP_BodyTail);
	LVL3DEBUG(printf("\r\n SOAP Sent: \r\n%s\r\n",buffer);)
	free(NewConnectionType);
	
	IGD_AddRef(service->Parent);
	IGD_ParseUri(service->ControlURL, &IP, &Port, &Path, NULL);
	
	len = 174 + (int)strlen(IGD_PLATFORM) + (int)strlen(Path) + (int)strlen(IP) + (int)strlen(service->ServiceType);
	if ((headerBuffer = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	headerLength = snprintf(headerBuffer, len, UPNPCP_SOAP_Header, Path, IP, Port, IGD_PLATFORM, service->ServiceType, "SetConnectionType", bufferLength);
	
	invoke_data->CallbackPtr = (voidfp)CallbackPtr;
	invoke_data->User = user;
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequestEx(
	((struct IGD_CP*)service->Parent->CP)->HTTP,
	(struct sockaddr*)&(service->Parent->LocationAddr),
	headerBuffer,
	headerLength,
	0,
	buffer,
	bufferLength,
	0,
	&IGD_Invoke_WANIPConnection_SetConnectionType_Sink,
	service,
	invoke_data);
	
	free(IP);
	free(Path);
}


/*! \fn IGD_UnSubscribeUPnPEvents(struct UPnPService *service)
\brief Unregisters for UPnP events
\param service UPnP service to unregister from
*/
void IGD_UnSubscribeUPnPEvents(struct UPnPService *service)
{
	char *IP;
	char *Path;
	unsigned short Port;
	struct packetheader *p;
	char* TempString;
	size_t len;
	
	if (service->SubscriptionID == NULL) {return;}
	IGD_ParseUri(service->SubscriptionURL, &IP, &Port, &Path, NULL);
	p = IGD_CreateEmptyPacket();
	IGD_SetVersion(p, "1.1", 3);
	
	IGD_SetDirective(p, "UNSUBSCRIBE", 11, Path, (int)strlen(Path));
	
	len = (int)strlen(IP) + 7;
	if ((TempString = (char*)malloc(len)) == NULL) ILIBCRITICALEXIT(254);
	snprintf(TempString, len, "%s:%d", IP, Port);
	
	IGD_AddHeaderLine(p, "HOST", 4, TempString, (int)strlen(TempString));
	free(TempString);
	
	IGD_AddHeaderLine(p, "User-Agent", 10, IGD_PLATFORM, (int)strlen(IGD_PLATFORM));
	IGD_AddHeaderLine(p, "SID", 3, service->SubscriptionID, (int)strlen(service->SubscriptionID));
	
	IGD_AddRef(service->Parent);
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequest(
	((struct IGD_CP*)service->Parent->CP)->HTTP, 
	(struct sockaddr*)&(service->Parent->LocationAddr), 
	p, 
	&IGD_OnUnSubscribeSink, 
	(void*)service, 
	service->Parent->CP);
	
	free(IP);
	free(Path);
}

/*! \fn IGD_SubscribeForUPnPEvents(struct UPnPService *service, void(*callbackPtr)(struct UPnPService* service, int OK))
\brief Registers for UPnP events
\param service UPnP service to register with
\param callbackPtr Function Pointer triggered on completion
*/
void IGD_SubscribeForUPnPEvents(struct UPnPService *service, void(*callbackPtr)(struct UPnPService* service, int OK))
{
	char *IP;
	char *Path;
	unsigned short Port;
	struct packetheader *p;
	int len;
	
	UNREFERENCED_PARAMETER( callbackPtr );
	
	IGD_ParseUri(service->SubscriptionURL, &IP, &Port, &Path, NULL);
	p = IGD_CreateEmptyPacket();
	IGD_SetVersion(p, "1.1", 3);
	
	IGD_SetDirective(p, "SUBSCRIBE", 9, Path, (int)strlen(Path));
	
	len = snprintf(IGD_ScratchPad, sizeof(IGD_ScratchPad), "%s:%d", IP, Port);
	IGD_AddHeaderLine(p, "HOST", 4, IGD_ScratchPad, len);
	IGD_AddHeaderLine(p, "NT", 2, "upnp:event", 10);
	IGD_AddHeaderLine(p, "TIMEOUT", 7, "Second-180", 10);
	IGD_AddHeaderLine(p, "User-Agent", 10, IGD_PLATFORM, (int)strlen(IGD_PLATFORM));
	len = snprintf(IGD_ScratchPad, sizeof(IGD_ScratchPad), "<http://%s:%d%s>", service->Parent->InterfaceToHost, IGD_WebServer_GetPortNumber(((struct IGD_CP*)service->Parent->CP)->WebServer), Path);
	IGD_AddHeaderLine(p, "CALLBACK", 8, IGD_ScratchPad, len);
	
	IGD_AddRef(service->Parent);
	//IGD_WebClient_SetQosForNextRequest(((struct IGD_CP*)service->Parent->CP)->HTTP, IGD_InvocationPriorityLevel);
	IGD_WebClient_PipelineRequest(
	((struct IGD_CP*)service->Parent->CP)->HTTP, 
	(struct sockaddr*)&(service->Parent->LocationAddr), 
	p, 
	&IGD_OnSubscribeSink, 
	(void*)service, 
	service->Parent->CP);
	
	free(IP);
	free(Path);
}


void IGD_IPAddressMonitor(void *data)
{
	int length;
	int *list;
	IGD_HANDLE hIGD = (IGD_HANDLE)(data);

	length = IGD_GetLocalIPAddressList(&list);
	if (length != hIGD->ipCount || memcmp((void*)list, (void*)hIGD->ipAddresses, sizeof(int)*length) != 0)
	{
		IGD_CP_IPAddressListChanged(hIGD->igd_cp);
		free(hIGD->ipAddresses);
		hIGD->ipAddresses	= list;
		hIGD->ipCount		= length;
	}
	else
	{
		free(list);
	}

	IGD_LifeTime_Add(hIGD->monitor, hIGD, 4, (void*)&IGD_IPAddressMonitor, NULL);
}


void IGD_DeviceDiscoverSink(struct UPnPDevice *device)
{
	struct UPnPDevice *tempDevice = device;
	struct UPnPService *tempService;

	printf("UPnPDevice Added: %s\r\n", device->FriendlyName);

	// This call will print the device, all embedded devices and service to the console. It is just used for debugging.
	// IGD_PrintUPnPDevice(0, device);

	// The following subscribes for events on all services
	while(tempDevice != NULL)
	{
		tempService = tempDevice->Services;
		while(tempService != NULL)
		{
			IGD_SubscribeForUPnPEvents(tempService, NULL);
			tempService = tempService->Next;
		}
		tempDevice = tempDevice->Next;
	}

	// The following will call every method of every service in the device with sample values
	// You can cut & paste these lines where needed. The user value is NULL, it can be freely used
	// to pass state information.
	// The IGD_GetService call can return NULL, a correct application must check this since a device
	// can be implemented without some services.

	// You can check for the existence of an action by calling: IGD_HasAction(serviceStruct,serviceType)
	// where serviceStruct is the struct like tempService, and serviceType, is a null terminated string representing
	// the service urn.

	if (IGD_EventCallback_WANIPConnectionDevice_DiscoverSink != NULL)
	{
		IGD_EventCallback_WANIPConnectionDevice_DiscoverSink(device);
	}
}

// Called whenever a discovered device was removed from the network
void IGD_DeviceRemoveSink(struct UPnPDevice *device)
{
	printf("UPnPDevice Removed: %s\r\n", device->FriendlyName);

	if (IGD_EventCallback_WANIPConnectionDevice_RemoveSink != NULL)
	{
		IGD_EventCallback_WANIPConnectionDevice_RemoveSink(device);
	}
}

void* IGD_ThreadProc(void* param)
{
	IGD_HANDLE hIGD = (IGD_HANDLE)(param);

	printf("[%s:%d] (+)\n", __FUNCTION__, __LINE__);

	IGD_StartChain(hIGD->chain);

	printf("[%s:%d] (-)\n", __FUNCTION__, __LINE__);

	return 0;
}

struct UPnPService* igd_GetWANIPConnection(IGD_HANDLE hIGD)
{
	struct UPnPDevice* IgdDevice;

	if (hIGD == NULL)
	{
		printf("[%s:%d] Invalid args\n", __FUNCTION__, __LINE__);
		return NULL;
	}

	IgdDevice = IGD_GetDeviceAtUDN(hIGD->igd_cp, hIGD->udn);

	if (IgdDevice == NULL)
	{
		printf("[%s:%d] Not found device:%s\n", __FUNCTION__, __LINE__, hIGD->udn);
		return NULL;
	}

	return IGD_GetService_WANIPConnection(IgdDevice);
}

// Interfaces =======================
IGD_HANDLE IGD_CreateClient(void)
{
	IGD_HANDLE hIGD = malloc(sizeof(struct igd_handle_tag));;
	memset(hIGD, 0, sizeof(struct igd_handle_tag));

	hIGD->chain		= IGD_CreateChain();
	hIGD->igd_cp	= IGD_CreateControlPoint(hIGD->chain, &IGD_DeviceDiscoverSink, &IGD_DeviceRemoveSink);

	hIGD->monitor	= IGD_CreateLifeTime(hIGD->chain);
	hIGD->ipCount	= IGD_GetLocalIPAddressList(&hIGD->ipAddresses);
	IGD_LifeTime_Add(hIGD->monitor, hIGD, 4, (void*)&IGD_IPAddressMonitor, NULL);

	pthread_create(&hIGD->threadid, NULL, IGD_ThreadProc, hIGD);

	return hIGD;
}

bool IGD_DestroyClient(IGD_HANDLE hIGD)
{
	void* ret = NULL;

	if(hIGD->chain != NULL)
	{
		IGD_StopChain(hIGD->chain);
		hIGD->chain = NULL;
	}

	pthread_join(hIGD->threadid, &ret);

	if(hIGD->ipAddresses != NULL)
	{
		free(hIGD->ipAddresses);
	}

	free(hIGD);

	return true;
}

void IGD_SetDiscoveryCallback(IGD_EventCallback_DiscoverSink cbDiscoverSink, IGD_EventCallback_RemoveSink cbRemoveSink)
{
	IGD_EventCallback_WANIPConnectionDevice_DiscoverSink	= cbDiscoverSink;
	IGD_EventCallback_WANIPConnectionDevice_RemoveSink	= cbRemoveSink;
}

void IGD_SetEventCallback(
		IGD_EventCallback_PortMappingNumberOfEntries	cbPortMappingNumberOfEntries,
		IGD_EventCallback_ExternalIPAddress				cbExternalIPAddress,
		IGD_EventCallback_ConnectionStatus				cbConnectionStatus,
		IGD_EventCallback_PossibleConnectionTypes		cbPossibleConnectionTypes)
{
	IGD_EventCallback_WANIPConnection_PortMappingNumberOfEntries	= cbPortMappingNumberOfEntries;
	IGD_EventCallback_WANIPConnection_ExternalIPAddress				= cbExternalIPAddress;
	IGD_EventCallback_WANIPConnection_ConnectionStatus				= cbConnectionStatus;
	IGD_EventCallback_WANIPConnection_PossibleConnectionTypes		= cbPossibleConnectionTypes;
}

bool IGD_SelectDevice(IGD_HANDLE hIGD, const char* udn)
{
	sprintf(hIGD->udn ,"%s", udn);
}

bool IGD_AddPortMapping(IGD_HANDLE hIGD, IGD_ResponseSink_AddPortMapping CallbackPtr,void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol, unsigned short NewInternalPort, char* unescaped_NewInternalClient, int NewEnabled, char* unescaped_NewPortMappingDescription, unsigned int NewLeaseDuration)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_AddPortMapping(IgdService, CallbackPtr,_user, unescaped_NewRemoteHost, NewExternalPort, unescaped_NewProtocol, NewInternalPort, unescaped_NewInternalClient, NewEnabled, unescaped_NewPortMappingDescription, NewLeaseDuration);

	return true;
}

bool IGD_DeletePortMapping(IGD_HANDLE hIGD, IGD_ResponseSink_DeletePortMapping CallbackPtr,void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_DeletePortMapping(IgdService, CallbackPtr, _user, unescaped_NewRemoteHost, NewExternalPort, unescaped_NewProtocol);

	return true;
}

bool IGD_ForceTermination(IGD_HANDLE hIGD, IGD_ResponseSink_ForceTermination CallbackPtr,void* _user)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_ForceTermination(IgdService, CallbackPtr, _user);

	return true;
}

bool IGD_GetConnectionTypeInfo(IGD_HANDLE hIGD, IGD_ResponseSink_GetConnectionTypeInfo CallbackPtr,void* _user)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_GetConnectionTypeInfo(IgdService, CallbackPtr, _user);

	return true;
}

bool IGD_GetExternalIPAddress(IGD_HANDLE hIGD, IGD_ResponseSink_GetExternalIPAddress CallbackPtr,void* _user)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_GetExternalIPAddress(IgdService, CallbackPtr, _user);

	return true;
}

bool IGD_GetGenericPortMappingEntry(IGD_HANDLE hIGD, IGD_ResponseSink_GetGenericPortMappingEntry CallbackPtr,void* _user, unsigned short NewPortMappingIndex)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_GetGenericPortMappingEntry(IgdService, CallbackPtr, _user,  NewPortMappingIndex);

	return true;
}

bool IGD_GetNATRSIPStatus(IGD_HANDLE hIGD, IGD_ResponseSink_GetNATRSIPStatus CallbackPtr,void* _user)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_GetNATRSIPStatus(IgdService, CallbackPtr, _user);

	return true;
}

bool IGD_GetSpecificPortMappingEntry(IGD_HANDLE hIGD, IGD_ResponseSink_GetSpecificPortMappingEntry CallbackPtr,void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_GetSpecificPortMappingEntry(IgdService, CallbackPtr, _user, unescaped_NewRemoteHost, NewExternalPort, unescaped_NewProtocol);

	return true;
}

bool IGD_GetStatusInfo(IGD_HANDLE hIGD, IGD_ResponseSink_GetStatusInfo CallbackPtr,void* _user)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_GetStatusInfo(IgdService, CallbackPtr, _user);

	return true;
}

bool IGD_RequestConnection(IGD_HANDLE hIGD, IGD_ResponseSink_RequestConnection CallbackPtr,void* _user)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_RequestConnection(IgdService, CallbackPtr, _user);

	return true;
}

bool IGD_SetConnectionType(IGD_HANDLE hIGD, IGD_ResponseSink_SetConnectionType CallbackPtr,void* _user, char* unescaped_NewConnectionType)
{
	struct UPnPService* IgdService;

	IgdService = igd_GetWANIPConnection(hIGD);

	if (IgdService == NULL)
	{
		printf("[%s:%d] Not found service:%s\n", __FUNCTION__, __LINE__, "urn:schemas-upnp-org:service:WANIPConnection:1");
		return false;
	}

	// to do :
	IGD_Invoke_WANIPConnection_SetConnectionType(IgdService, CallbackPtr, _user, unescaped_NewConnectionType);

	return true;
}
