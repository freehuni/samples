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

#ifndef __IGD_ControlPoint__
#define __IGD_ControlPoint__

#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "IGD_ControlPointStructs.h"

void IGD_Invoke_WANIPConnection_AddPortMapping(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol, unsigned short NewInternalPort, char* unescaped_NewInternalClient, int NewEnabled, char* unescaped_NewPortMappingDescription, unsigned int NewLeaseDuration);
void IGD_Invoke_WANIPConnection_DeletePortMapping(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol);
void IGD_Invoke_WANIPConnection_ForceTermination(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user);
void IGD_Invoke_WANIPConnection_GetConnectionTypeInfo(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionType,char* NewPossibleConnectionTypes),void* _user);
void IGD_Invoke_WANIPConnection_GetExternalIPAddress(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewExternalIPAddress),void* _user);
void IGD_Invoke_WANIPConnection_GetGenericPortMappingEntry(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewRemoteHost,unsigned short NewExternalPort,char* NewProtocol,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration),void* _user, unsigned short NewPortMappingIndex);
void IGD_Invoke_WANIPConnection_GetNATRSIPStatus(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,int NewRSIPAvailable,int NewNATEnabled),void* _user);
void IGD_Invoke_WANIPConnection_GetSpecificPortMappingEntry(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration),void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol);
void IGD_Invoke_WANIPConnection_GetStatusInfo(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionStatus,char* NewLastConnectionError,unsigned int NewUptime),void* _user);
void IGD_Invoke_WANIPConnection_RequestConnection(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user);
void IGD_Invoke_WANIPConnection_SetConnectionType(struct UPnPService *service, void (*CallbackPtr)(struct UPnPService *sender,int ErrorCode,void *user),void* _user, char* unescaped_NewConnectionType);


/*! \file IGD_ControlPoint.h 
	\brief MicroStack APIs for Control Point Implementation
*/

/*! \defgroup ControlPoint Control Point Module
	\{
*/


/* Complex Type Parsers */


/* Complex Type Serializers */




/*! \defgroup CPReferenceCounter Reference Counter Methods
	\ingroup ControlPoint
	\brief Reference Counting for the UPnPDevice and UPnPService objects.
	\para
	Whenever a user application is going to keep the pointers to the UPnPDevice object that is obtained from
	the add sink (or any pointers inside them), the application <b>must</b> increment the reference counter. Failure to do so
	will lead to references to invalid pointers, when the device leaves the network.
	\{
*/
void IGD_AddRef(struct UPnPDevice *device);
void IGD_Release(struct UPnPDevice *device);
/*! \} */   



struct UPnPDevice* IGD_GetDevice1(struct UPnPDevice *device,int index);
int IGD_GetDeviceCount(struct UPnPDevice *device);
struct UPnPDevice* IGD_GetDeviceEx(struct UPnPDevice *device, char* DeviceType, int start,int number);
void PrintUPnPDevice(int indents, struct UPnPDevice *device);



/*! \defgroup CPAdministration Administrative Methods
	\ingroup ControlPoint
	\brief Basic administrative functions, used to setup/configure the control point application
	\{
*/
void *IGD_CreateControlPoint(void *Chain, void(*A)(struct UPnPDevice*),void(*R)(struct UPnPDevice*));
void IGD_ControlPoint_AddDiscoveryErrorHandler(void *cpToken, UPnPDeviceDiscoveryErrorHandler callback);
struct UPnPDevice* IGD_GetDeviceAtUDN(void *v_CP, char* UDN);
void IGD_CP_IPAddressListChanged(void *CPToken);
int IGD_HasAction(struct UPnPService *s, char* action);
void IGD_UnSubscribeUPnPEvents(struct UPnPService *service);
void IGD_SubscribeForUPnPEvents(struct UPnPService *service, void(*callbackPtr)(struct UPnPService* service, int OK));
struct UPnPService *IGD_GetService(struct UPnPDevice *device, char* ServiceName, int length);

void IGD_SetUser(void *token, void *user);
void* IGD_GetUser(void *token);

struct UPnPService *IGD_GetService_WANIPConnection(struct UPnPDevice *device);

/*! \} */


/*! \defgroup InvocationEventingMethods Invocation/Eventing Methods
	\ingroup ControlPoint
	\brief Methods used to invoke actions and receive events from a UPnPService
	\{
*/



/*! \} */


/*! \} */

#ifdef __cplusplus
}
#endif

#endif
