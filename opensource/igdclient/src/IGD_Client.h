/**************************************************************************************/
/**
*
* \file IGD_Client.h
*
* \mainpage
* This document describes WANIPConenction service APIs.\n
* IGD Device includes several internal services such as WANIPConnection service. \n
* If you need more information about the service, please refer to UPnP-gw-WANIPConnection-v1-Service-20011112.pdf.
* \copyright
* Copyright 2006 - 2011 Intel Corporation\n
\n
* Licensed under the Apache License, Version 2.0 (the "License");\n
* you may not use this file except in compliance with the License.\n
* You may obtain a copy of the License at\n
\n
*   http://www.apache.org/licenses/LICENSE-2.0\n
\n
* Unless required by applicable law or agreed to in writing, software\n
* distributed under the License is distributed on an "AS IS" BASIS,\n
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n
* See the License for the specific language governing permissions and\n
* limitations under the License.\n
* \author  mhkang2@humaxdigital.com
* \date    2018/3/12
*
**************************************************************************************
**/

#pragma once

#include <stdbool.h>
#include <pthread.h>
#include "IGD_ControlPointStructs.h"

typedef struct igd_handle_tag
{
	pthread_t	threadid;
	void*		igd_cp;
	void*		chain;
	void*		monitor;
	int *		ipAddresses;
	int			ipCount;
	char		udn[100];
}* IGD_HANDLE;

// Response APIs : refer to UPnP-gw-WANIPConnection-v1-Service-20011112.pdf
typedef void (*IGD_ResponseSink_AddPortMapping)(struct UPnPService *sender,int ErrorCode,void *user);
typedef void (*IGD_ResponseSink_DeletePortMapping)(struct UPnPService *sender,int ErrorCode,void *user);
typedef void (*IGD_ResponseSink_ForceTermination)(struct UPnPService *sender,int ErrorCode,void *user);
typedef void (*IGD_ResponseSink_GetConnectionTypeInfo)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionType,char* NewPossibleConnectionTypes);
typedef void (*IGD_ResponseSink_GetExternalIPAddress)(struct UPnPService *sender,int ErrorCode,void *user,char* NewExternalIPAddress);
typedef void (*IGD_ResponseSink_GetGenericPortMappingEntry)(struct UPnPService *sender,int ErrorCode,void *user,char* NewRemoteHost,unsigned short NewExternalPort,char* NewProtocol,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration);
typedef void (*IGD_ResponseSink_GetNATRSIPStatus)(struct UPnPService *sender,int ErrorCode,void *user,int NewRSIPAvailable,int NewNATEnabled);
typedef void (*IGD_ResponseSink_GetSpecificPortMappingEntry)(struct UPnPService *sender,int ErrorCode,void *user,unsigned short NewInternalPort,char* NewInternalClient,int NewEnabled,char* NewPortMappingDescription,unsigned int NewLeaseDuration);
typedef void (*IGD_ResponseSink_GetStatusInfo)(struct UPnPService *sender,int ErrorCode,void *user,char* NewConnectionStatus,char* NewLastConnectionError,unsigned int NewUptime);
typedef void (*IGD_ResponseSink_RequestConnection)(struct UPnPService *sender,int ErrorCode,void *user);
typedef void (*IGD_ResponseSink_SetConnectionType)(struct UPnPService *sender,int ErrorCode,void *user);

// Event
typedef void (*IGD_EventCallback_PortMappingNumberOfEntries)(struct UPnPService* Service,unsigned short value);
typedef void (*IGD_EventCallback_ExternalIPAddress)(struct UPnPService* Service,char* value);
typedef void (*IGD_EventCallback_ConnectionStatus)(struct UPnPService* Service,char* value);
typedef void (*IGD_EventCallback_PossibleConnectionTypes)(struct UPnPService* Service,char* value);

// Discovery callback
typedef void (*IGD_EventCallback_DiscoverSink)(struct UPnPDevice* Service);
typedef void (*IGD_EventCallback_RemoveSink)(struct UPnPDevice* Service);

extern IGD_EventCallback_DiscoverSink					IGD_EventCallback_WANIPConnectionDevice_DiscoverSink;
extern IGD_EventCallback_RemoveSink						IGD_EventCallback_WANIPConnectionDevice_RemoveSink;

extern IGD_EventCallback_PortMappingNumberOfEntries		IGD_EventCallback_WANIPConnection_PortMappingNumberOfEntries;
extern IGD_EventCallback_ExternalIPAddress				IGD_EventCallback_WANIPConnection_ExternalIPAddress;
extern IGD_EventCallback_ConnectionStatus				IGD_EventCallback_WANIPConnection_ConnectionStatus;
extern IGD_EventCallback_PossibleConnectionTypes		IGD_EventCallback_WANIPConnection_PossibleConnectionTypes;

#ifdef __cplusplus
extern "C" {
#endif

/*!
****************************************************************************************************
* Create IGD control point
* \param N/A
*
* \return Handle of IGD Control Point
****************************************************************************************************
*/
IGD_HANDLE IGD_CreateClient(void);

/*!
****************************************************************************************************
* Destroy IGD control point
* \param hIGD  handle of IGD Control Point
*
* \return true or false
****************************************************************************************************
*/
bool IGD_DestroyClient(IGD_HANDLE hIGD);

/*!
****************************************************************************************************
* Register callback functions which is called when IGD is discovered or disappeared.
* \param hIGD  handle of IGD Control Point
*
* \return N/A
****************************************************************************************************
*/
void IGD_SetDiscoveryCallback(IGD_EventCallback_DiscoverSink cbDiscoverSink, IGD_EventCallback_RemoveSink cbRemoveSink);

/*!
****************************************************************************************************
* If IGD device's state is changed, IGD device notifies its event to IGD control point.
* When IGD control point receives the event, event callback fucntion is called at that time.
* \param hIGD  handle of IGD Control Point
* \param cbPortMappingNumberOfEntries
* \param cbExternalIPAddress
* \param cbConnectionStatus
* \param cbPossibleConnectionTypes
*
* \return N/A
****************************************************************************************************
*/
void IGD_SetEventCallback(
		IGD_EventCallback_PortMappingNumberOfEntries	cbPortMappingNumberOfEntries,
		IGD_EventCallback_ExternalIPAddress				cbExternalIPAddress,
		IGD_EventCallback_ConnectionStatus				cbConnectionStatus,
		IGD_EventCallback_PossibleConnectionTypes		cbPossibleConnectionTypes);

/*!
****************************************************************************************************
* IGD Device has its unique id(uuid). this uuid is called as UDN.
* After discovering IGD Device, Its control point must select the IGD deivce with its UDN in order to invoke the APIs of IGD.
* \param hIGD  handle of IGD control point
* \param udn  UDN of IGD
*
* \return true or false
****************************************************************************************************
*/
bool IGD_SelectDevice(IGD_HANDLE hIGD, const char* udn);

/*!
****************************************************************************************************
* This action creates a new port mapping or overwrites an existing mapping with the same internal client.\n
* If the ExternalPort and PortMappingProtocol pair is already mapped to another internal client, an error is returned.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of AddPortMapping.
* \param _user This value passes through CallbackPtr.
* \param unescaped_NewRemoteHost
* This variable represents the source of inbound IP packets. This will be a wildcard in most cases
* (i.e. an empty string). NAT vendors are only required to support wildcards. A non-wildcard value
* will allow for “narrow” port mappings, which may be desirable in some usage scenarios.When
* RemoteHost is a wildcard, all traffic sent to the ExternalPort on the WAN interface of the
* gateway is forwarded to the InternalClient on the InternalPort. When RemoteHost is
* specified as one external IP address as opposed to a wildcard, the NAT will only forward inbound
* packets from this RemoteHost to the InternalClient, all other packets will be dropped.
* \param NewExternalPort
* This variable represents the external port that the NAT gateway would “listen” on for connection\n
* requests to a corresponding InternalPort on an InternalClient.. Inbound packets to this\n
* external port on the WAN interface of the gateway should be forwarded to InternalClient on\n
* the InternalPort on which the message was received. If this value is specified as a wildcard\n
* (i.e. 0), connection request on all external ports (that are not otherwise mapped) will be forwarded to InternalClient.
* In the wildcard case, the value(s) of InternalPort on InternalClient are ignored by the IGD for those connections that are forwarded to InternalClient.
* Obviously only one such entry can exist in the NAT at any time and conflicts are handled with a “first write wins” behavior.
* \param unescaped_NewProtocol
* This variable represents the protocol of the port mapping. Possible values are TCP or UDP.
* \param NewInternalPort
* This variable represents the port on InternalClient that the gateway should forward
* connection requests to. A value of 0 is not allowed. NAT implementations that do not permit
* different values for ExternalPort and InternalPort will return an error.
* \param unescaped_NewInternalClient
* This variable represents the IP address or DNS host name of an internal client (on the residential LAN).
* Note that if the gateway does not support DHCP, it does not have to support DNS host names.
* Consequently, support for an IP address is mandatory and support for DNS host names is recommended.
* This value cannot be a wildcard (i.e. empty string).
* It must be possible to set the InternalClient to the broadcast IP address 255.255.255.255 for UDP mappings.
* This is to enable multiple NAT clients to use the same well-known port simultaneously.
* \param NewEnabled
* This variable allows security conscious users to disable and enable dynamic and static NAT port mappings on the IGD.
* \param unescaped_NewPortMappingDescription
* This is a string representation of a port mapping and is applicable for static and dynamic port mappings.
* The format of the description string is not specified and is application dependent.
* If specified, the description string can be displayed to a user via the UI of a control point, enabling easier management of port mappings.
* The description string for a port mapping (or a set of related port mappings) may or may not be unique across multiple instantiations of an application on multiple nodes in the residential LAN.
* \param NewLeaseDuration
* This variable determines the time to live in seconds of a port-mapping lease. A value of 0 means the port mapping is static.
* Non-zero values will allow support for dynamic port mappings.
* Note that static port mappings do not necessarily mean persistence of these mappings across device resets or reboots.
* It is up to a gateway vendor to implement persistence as appropriate for their IGD device.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_AddPortMapping(IGD_HANDLE hIGD, IGD_ResponseSink_AddPortMapping CallbackPtr,void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol, unsigned short NewInternalPort, char* unescaped_NewInternalClient, int NewEnabled, char* unescaped_NewPortMappingDescription, unsigned int NewLeaseDuration);

/*!
****************************************************************************************************
* This action deletes a previously instantiated port mapping. As each entry is deleted, the array is
* compacted, and the evented variable PortMappingNumberOfEntries is decremented.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of DeletePortMapping.
* \param _user This value passes through CallbackPtr.
* \param unescaped_NewRemoteHost
* This variable represents the source of inbound IP packets. This will be a wildcard in most cases
* (i.e. an empty string). NAT vendors are only required to support wildcards. A non-wildcard value
* will allow for “narrow” port mappings, which may be desirable in some usage scenarios.When
* RemoteHost is a wildcard, all traffic sent to the ExternalPort on the WAN interface of the
* gateway is forwarded to the InternalClient on the InternalPort. When RemoteHost is
* specified as one external IP address as opposed to a wildcard, the NAT will only forward inbound
* packets from this RemoteHost to the InternalClient, all other packets will be dropped.
* \param NewExternalPort
* This variable represents the external port that the NAT gateway would “listen” on for connection\n
* requests to a corresponding InternalPort on an InternalClient.. Inbound packets to this\n
* external port on the WAN interface of the gateway should be forwarded to InternalClient on\n
* the InternalPort on which the message was received. If this value is specified as a wildcard\n
* (i.e. 0), connection request on all external ports (that are not otherwise mapped) will be forwarded to InternalClient.
* In the wildcard case, the value(s) of InternalPort on InternalClient are ignored by the IGD for those connections that are forwarded to InternalClient.
* Obviously only one such entry can exist in the NAT at any time and conflicts are handled with a “first write wins” behavior.
* \param unescaped_NewProtocol
* This variable represents the protocol of the port mapping. Possible values are TCP or UDP.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_DeletePortMapping(IGD_HANDLE hIGD, IGD_ResponseSink_DeletePortMapping CallbackPtr,void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol);

/*!
****************************************************************************************************
* A client may send this command to any connection instance in Connected,Connecting,
* PendingDisconnect or Disconnecting state to change ConnectionStatus to Disconnected.
* Connection state immediately transitions to Disconnected irrespective of the setting of
* WarnDisconnectDelay variable. The process of terminating a connection is described in
* Theory of Operation section.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of ForceTermination.
* \param _user This value passes through CallbackPtr.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_ForceTermination(IGD_HANDLE hIGD, IGD_ResponseSink_ForceTermination CallbackPtr,void* _user);

/*!
****************************************************************************************************
* This action retrieves the values of the current connection type and allowable connection types.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of ConnectionTypeInfo.
* \param _user This value passes through CallbackPtr.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_GetConnectionTypeInfo(IGD_HANDLE hIGD, IGD_ResponseSink_GetConnectionTypeInfo CallbackPtr,void* _user);

/*!
****************************************************************************************************
* This action retrieves the value of the external IP address on this connection instance.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of GetExternalIPAddress.
* \param _user This value passes through CallbackPtr.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_GetExternalIPAddress(IGD_HANDLE hIGD, IGD_ResponseSink_GetExternalIPAddress CallbackPtr,void* _user);

/*!
****************************************************************************************************
* This action retrieves NAT port mappings one entry at a time. Control points can call this action
* with an incrementing array index until no more entries are found on the gateway. If
* PortMappingNumberOfEntries is updated during a call, the process may have to start over.
* Entries in the array are contiguous. As entries are deleted, the array is compacted, and the
* evented variable PortMappingNumberOfEntries is decremented. Port mappings are logically
* stored as an array on the IGD and retrieved using an array index ranging from 0 to PortMappingNumberOfEntries-1.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of GetGenericPortMappingEntry.
* \param _user This value passes through CallbackPtr.
* \param NewPortMappingIndex Zero based index of Port Mapping.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_GetGenericPortMappingEntry(IGD_HANDLE hIGD, IGD_ResponseSink_GetGenericPortMappingEntry CallbackPtr,void* _user, unsigned short NewPortMappingIndex);

/*!
****************************************************************************************************
* This action retrieves the current state of NAT and RSIP on the gateway for this connection.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of GetNATRSIPStatus.
* \param _user This value passes through CallbackPtr.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_GetNATRSIPStatus(IGD_HANDLE hIGD, IGD_ResponseSink_GetNATRSIPStatus CallbackPtr,void* _user);

/*!
****************************************************************************************************
* This action reports the Static Port Mapping specified by the unique tuple of RemoteHost,
* ExternalPort and PortMappingProtocol.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of GetSpecificPortMappingEntry.
* \param _user This value passes through CallbackPtr.
* \param unescaped_NewRemoteHost
* This variable represents the source of inbound IP packets. This will be a wildcard in most cases
* (i.e. an empty string). NAT vendors are only required to support wildcards. A non-wildcard value
* will allow for “narrow” port mappings, which may be desirable in some usage scenarios.When
* RemoteHost is a wildcard, all traffic sent to the ExternalPort on the WAN interface of the
* gateway is forwarded to the InternalClient on the InternalPort. When RemoteHost is
* specified as one external IP address as opposed to a wildcard, the NAT will only forward inbound
* packets from this RemoteHost to the InternalClient, all other packets will be dropped.
* \param NewExternalPort
* This variable represents the external port that the NAT gateway would “listen” on for connection\n
* requests to a corresponding InternalPort on an InternalClient.. Inbound packets to this\n
* external port on the WAN interface of the gateway should be forwarded to InternalClient on\n
* the InternalPort on which the message was received. If this value is specified as a wildcard\n
* (i.e. 0), connection request on all external ports (that are not otherwise mapped) will be forwarded to InternalClient.
* In the wildcard case, the value(s) of InternalPort on InternalClient are ignored by the IGD for those connections that are forwarded to InternalClient.
* Obviously only one such entry can exist in the NAT at any time and conflicts are handled with a “first write wins” behavior.
* \param unescaped_NewProtocol
* This variable represents the protocol of the port mapping. Possible values are TCP or UDP.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_GetSpecificPortMappingEntry(IGD_HANDLE hIGD, IGD_ResponseSink_GetSpecificPortMappingEntry CallbackPtr,void* _user, char* unescaped_NewRemoteHost, unsigned short NewExternalPort, char* unescaped_NewProtocol);

/*!
****************************************************************************************************
* This action retrieves the values of state variables pertaining to connection status.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of GetStatusInfo.
* \param _user This value passes through CallbackPtr.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_GetStatusInfo(IGD_HANDLE hIGD, IGD_ResponseSink_GetStatusInfo CallbackPtr,void* _user);

/*!
****************************************************************************************************
* A client sends this action to initiate a connection on an instance of a connection service that has
* a configuration already defined. RequestConnection causes the ConnectionStatus to
* immediately change to Connecting (if implemented) unless the action is not permitted in the
* current state of the IGD or the specific service instance. This change of state will be evented.
* RequestConnection should synchronously return at this time in accordance with UPnP
* architecture requirements that mandate that an action can take no more than 30 seconds to
* respond synchronously. However, the actual connection setup may take several seconds more to
* complete. If the connection setup is successful, ConnectionStatus will change to Connected
* and will be evented. If the connection setup is not successful, ConnectionStatus will
* eventually revert back to Disconnected and will be evented. LastConnectionError will be
* set appropriately in either case. While this may be obvious, it is worth noting that a control point
* must not source packets to the Internet until ConnectionStatus is updated to Connected, or
* the IGD may drop packets until it transitions to the Connected state.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of RequestConnection.
* \param _user This value passes through CallbackPtr.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_RequestConnection(IGD_HANDLE hIGD, IGD_ResponseSink_RequestConnection CallbackPtr,void* _user);

/*!
****************************************************************************************************
* This action sets up a specific connection type. Clients on the LAN may initiate or share
* connection only after this action completes or ConnectionType is set to a value other than
* Unconfigured. ConnectionType can be a read-only variable in cases where some form of auto
* configuration is employed.
* \param hIGD  handle of IGD control point
* \param CallbackPtr Reseponse callback of SetConnectionType.
* \param _user This value passes through CallbackPtr.
* \param unescaped_NewConnectionType This variable is set to specify the connection type(IP_Routed or IP_Bridged) for a specific active connection.
*
* \return true or false
****************************************************************************************************
*/
bool IGD_SetConnectionType(IGD_HANDLE hIGD, IGD_ResponseSink_SetConnectionType CallbackPtr,void* _user, char* unescaped_NewConnectionType);


#ifdef __cplusplus
}
#endif

