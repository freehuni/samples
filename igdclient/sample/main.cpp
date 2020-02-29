#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <map>
#include "IGD_Client.h"

#define UNUSED(x) (void)(x)

using namespace std;

typedef map<string, string> IGD_DEVICES;
typedef IGD_DEVICES::iterator IGD_DEVICE_IT;

enum eINVOKE_API
{
	eCMD_AddPortMapping = 0,
	eCMD_DeletePortMapping,
	eCMD_ForceTermination,
	eCMD_GetConnectionTypeInfo,
	eCMD_GetExternalIPAddress,
	eCMD_GetGenericPortMappingEntry,
	eCMD_GetNATRSIPStatus,
	eCMD_GetSpecificPortMappingEntry,
	eCMD_GetStatusInfo,
	eCMD_RequestConnection,
	eCMD_SetConnectionType,
	eCMD_Max,
	eCMD_Quit
};

IGD_DEVICES g_devices;

// [Porting Layer] =================================================================
void igd_cbDiscoverSink(struct UPnPDevice* Device)
{
	IGD_DEVICE_IT it;

	printf("[%s:%d] %s %s\n", __FUNCTION__, __LINE__, Device->UDN, Device->FriendlyName);
	it = g_devices.find(Device->UDN);
	if (it == g_devices.end())
	{
		g_devices.insert(make_pair(Device->UDN, Device->FriendlyName));
	}
}

void igd_cbRemoveSink(struct UPnPDevice* Device)
{
	IGD_DEVICE_IT it;

	printf("[%s:%d] %s %s\n", __FUNCTION__, __LINE__, Device->UDN, Device->FriendlyName);
	it = g_devices.find(Device->UDN);
	if (it != g_devices.end())
	{
		g_devices.erase(it);
	}
}

void igd_event_PortMappingNumberOfEntries(struct UPnPService* Service,unsigned short PortMappingNumberOfEntries)
{
	printf("IGD Event from %s/WANIPConnection/PortMappingNumberOfEntries: %u\r\n",Service->Parent->FriendlyName,PortMappingNumberOfEntries);
}

void igd_event_ExternalIPAddress(struct UPnPService* Service,char* ExternalIPAddress)
{
	if (ExternalIPAddress!=NULL)
	{
		printf("IGD Event from %s/WANIPConnection/ExternalIPAddress: %s\r\n", Service->Parent->FriendlyName, ExternalIPAddress);
	}
}

void igd_event_ConnectionStatus(struct UPnPService* Service,char* ConnectionStatus)
{
	if (ConnectionStatus!=NULL)
	{
		printf("IGD Event from %s/WANIPConnection/ConnectionStatus: %s\r\n", Service->Parent->FriendlyName, ConnectionStatus);
	}
}

void igd_event_PossibleConnectionTypes(struct UPnPService* Service,char* PossibleConnectionTypes)
{
	if (PossibleConnectionTypes!=NULL)
	{
		printf("IGD Event from %s/WANIPConnection/PossibleConnectionTypes: %s\r\n", Service->Parent->FriendlyName, PossibleConnectionTypes);
	}
}

void igd_ResponseSink_WANIPConnection_AddPortMapping(struct UPnPService* Service, int ErrorCode, void *User)
{
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/AddPortMapping[ErrorCode:%d]()\r\n",ErrorCode);
}

void igd_ResponseSink_WANIPConnection_DeletePortMapping(struct UPnPService* Service, int ErrorCode, void *User)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/DeletePortMapping[ErrorCode:%d]()\r\n",ErrorCode);
}

void igd_ResponseSink_WANIPConnection_ForceTermination(struct UPnPService* Service, int ErrorCode, void *User)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/ForceTermination[ErrorCode:%d]()\r\n",ErrorCode);
}

void igd_ResponseSink_WANIPConnection_GetConnectionTypeInfo(struct UPnPService* Service, int ErrorCode, void *User,char* NewConnectionType,char* NewPossibleConnectionTypes)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/GetConnectionTypeInfo[ErrorCode:%d](%s,%s)\r\n",ErrorCode,NewConnectionType,NewPossibleConnectionTypes);
}

void igd_ResponseSink_WANIPConnection_GetExternalIPAddress(struct UPnPService* Service, int ErrorCode, void *User,char* NewExternalIPAddress)
{
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/GetExternalIPAddress[ErrorCode:%d](%s)\r\n",ErrorCode,NewExternalIPAddress);
}

void igd_ResponseSink_WANIPConnection_GetGenericPortMappingEntry(struct UPnPService* Service, int ErrorCode, void *User,char* NewRemoteHost,unsigned short NewExternalPort,char* NewProtocol,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/GetGenericPortMappingEntry[ErrorCode:%d](%s,%u,%s,%u,%s,%d,%s,%u)\r\n",ErrorCode,NewRemoteHost,NewExternalPort,NewProtocol,NewInternalPort,NewInternalClient,NewEnabled,NewPortMappingDescription,NewLeaseDuration);
}

void igd_ResponseSink_WANIPConnection_GetNATRSIPStatus(struct UPnPService* Service, int ErrorCode, void *User,int NewRSIPAvailable,int NewNATEnabled)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/GetNATRSIPStatus[ErrorCode:%d](%d,%d)\r\n",ErrorCode,NewRSIPAvailable,NewNATEnabled);
}

void igd_ResponseSink_WANIPConnection_GetSpecificPortMappingEntry(struct UPnPService* Service, int ErrorCode, void *User,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration)
{
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/GetSpecificPortMappingEntry[ErrorCode:%d](%u,%s,%d,%s,%u)\r\n",ErrorCode,NewInternalPort,NewInternalClient,NewEnabled,NewPortMappingDescription,NewLeaseDuration);
}

void igd_ResponseSink_WANIPConnection_GetStatusInfo(struct UPnPService* Service, int ErrorCode, void *User,char* NewConnectionStatus,char* NewLastConnectionError,unsigned int NewUptime)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/GetStatusInfo[ErrorCode:%d](%s,%s,%u)\r\n",ErrorCode,NewConnectionStatus,NewLastConnectionError,NewUptime);
}

void igd_ResponseSink_WANIPConnection_RequestConnection(struct UPnPService* Service, int ErrorCode, void *User)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/RequestConnection[ErrorCode:%d]()\r\n",ErrorCode);
}

void igd_ResponseSink_WANIPConnection_SetConnectionType(struct UPnPService* Service, int ErrorCode, void *User)
{	
	UNUSED(Service);
	UNUSED(User);

	printf("IGD Invoke Response: WANIPConnection/SetConnectionType[ErrorCode:%d]()\r\n",ErrorCode);
}

// [App Private Layer] ====================================================
static void cli_prompt(const char * message, char * in_buffer)
{
	printf("%s", message);
	gets(in_buffer);
}

void cli_show_igd_devices()
{
	IGD_DEVICE_IT it;
	int i;

	printf("[Device List]============\n");
	for(i = 0, it = g_devices.begin() ; it != g_devices.end() ; it++)
	{
		printf(" [%d] %s : %s\n", i, it->first.c_str(), it->second.c_str());
	}
}

void cli_show_invoke_list()
{
	 const char* cmdInvoke[eCMD_Max]={
		"AddPortMapping",
		"DeletePortMapping",
		"ForceTermination",
		"GetConnectionTypeInfo",
		"GetExternalIPAddress",
		"GetGenericPortMappingEntry",
		"GetNATRSIPStatus",
		"GetSpecificPortMappingEntry",
		"GetStatusInfo",
		"RequestConnection",
		"SetConnectionType"
	};
	int i;

	printf("\n[Invoke List]============\n");
	for ( i = 0 ; i < eCMD_Max ; i++)
	{
		printf(" [%d] %s\n", i, cmdInvoke[i]);
	}
}

const char* cli_select_device()
{
	char in_buffer[1024] = {0,};
	int i, n;
	IGD_DEVICE_IT it;

	while (true)
	{
		cli_show_igd_devices();

		cli_prompt("Select Index of IGD[enter:refresh, q:exit]:", in_buffer);
		if (strlen(in_buffer) == 0)
			continue;

		if (strcmp(in_buffer, "q") == 0 || strcmp(in_buffer, "Q") == 0)
			return NULL;

		n = atoi(in_buffer);

		// select item(n).
		if (n < 0 || n >= g_devices.size())
		{
			continue;
		}

		for (i = 0, it = g_devices.begin() ; it != g_devices.end() ; it++)
		{
			if (i == 0)
			{
				return it->first.c_str();
			}
		}
	}

	return NULL;
}

eINVOKE_API cli_select_invoke_command(void)
{
	char in_buffer[1024] = {0,};
	int i, n;

	while(true)
	{
		cli_show_invoke_list();

		cli_prompt("Select Index of Invoke API[enter:refresh, q:exit]:", in_buffer);
		if (strlen(in_buffer) == 0)
			continue;

		if (strcmp(in_buffer, "q") == 0 || strcmp(in_buffer, "Q") == 0)
			return eCMD_Quit;

		return (eINVOKE_API)atoi(in_buffer);
	}
}

// [MAIN FUNCTION] ============================================
int main(void)
{
	IGD_HANDLE hIGD;
	const char* udn;
	eINVOKE_API invokeCmd;

	printf("[IGD Client] START.\n");

	// Create IGD Control Point
	hIGD = IGD_CreateClient();

	// Register Discovery/Remove Callback
	IGD_SetDiscoveryCallback(igd_cbDiscoverSink, igd_cbRemoveSink);

	// Register Event Callback
	IGD_SetEventCallback(
				igd_event_PortMappingNumberOfEntries,
				igd_event_ExternalIPAddress,
				igd_event_ConnectionStatus,
				igd_event_PossibleConnectionTypes);	

	// Select IGD Device
	udn = cli_select_device();
	if (udn == NULL) goto EXIT;

	IGD_SelectDevice(hIGD, udn);


	// Invoke API TEST
INVOKE_LOOP:
	usleep(500000);
	invokeCmd = cli_select_invoke_command();

	switch(invokeCmd)
	{
		case eCMD_AddPortMapping:
			IGD_AddPortMapping(hIGD, &igd_ResponseSink_WANIPConnection_AddPortMapping,NULL,
							   "",				// NewRemoteHost
							   8080,			// NewExternalPort
							   "TCP",			// NewProtocol
							   80,				// NewInternalPort
							   "192.168.1.100",	// NewInternalClient(IP)
							   1,				// NewEnalbed
							   "WebServer",		// NewPortMappingDescription
							   3600);			// NewLeaseDuration
			goto INVOKE_LOOP;;

		case eCMD_DeletePortMapping:
			IGD_DeletePortMapping(hIGD, &igd_ResponseSink_WANIPConnection_DeletePortMapping,NULL, "", 8080, "TCP");
			goto INVOKE_LOOP;;

		case eCMD_ForceTermination:
			IGD_ForceTermination(hIGD, &igd_ResponseSink_WANIPConnection_ForceTermination,NULL);
			goto INVOKE_LOOP;;

		case eCMD_GetConnectionTypeInfo:
			IGD_GetConnectionTypeInfo(hIGD, &igd_ResponseSink_WANIPConnection_GetConnectionTypeInfo,NULL);
			goto INVOKE_LOOP;;

		case eCMD_GetExternalIPAddress:
			IGD_GetExternalIPAddress(hIGD, &igd_ResponseSink_WANIPConnection_GetExternalIPAddress,NULL);
			goto INVOKE_LOOP;;

		case eCMD_GetGenericPortMappingEntry:
			IGD_GetGenericPortMappingEntry(hIGD, &igd_ResponseSink_WANIPConnection_GetGenericPortMappingEntry,NULL, 0);
			goto INVOKE_LOOP;;

		case eCMD_GetNATRSIPStatus:
			IGD_GetNATRSIPStatus(hIGD, &igd_ResponseSink_WANIPConnection_GetNATRSIPStatus,NULL);
			goto INVOKE_LOOP;;

		case eCMD_GetSpecificPortMappingEntry:
			IGD_GetSpecificPortMappingEntry(hIGD, &igd_ResponseSink_WANIPConnection_GetSpecificPortMappingEntry,NULL, "Sample String", 250, "Sample String");
			goto INVOKE_LOOP;;

		case eCMD_GetStatusInfo:
			IGD_GetStatusInfo(hIGD, &igd_ResponseSink_WANIPConnection_GetStatusInfo,NULL);
			goto INVOKE_LOOP;;

		case eCMD_RequestConnection:
			IGD_RequestConnection(hIGD, &igd_ResponseSink_WANIPConnection_RequestConnection,NULL);
			goto INVOKE_LOOP;;

		case eCMD_SetConnectionType:
			IGD_SetConnectionType(hIGD, &igd_ResponseSink_WANIPConnection_SetConnectionType,NULL, "Sample String");
			goto INVOKE_LOOP;;

		default:
			break;
	}

EXIT:
	// Destroy IGD Control Point
	IGD_DestroyClient(hIGD);

	printf("[IGD Client] END.\n");

	return 0;
}
