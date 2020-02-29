#include <vector>
#include <string>
#include <libgupnp/gupnp.h>
#include <stdlib.h>
#include <gmodule.h>
#include "utility.h"

//#define SUPPORT_ORANGE_IGD

using namespace std;

#ifdef SUPPORT_ORANGE_IGD
	#define IGD_WANDEVICE				"urn:dslforum-org:device:WANDevice:1"
	#define IGD_WANConnectionDevice		"urn:dslforum-org:device:WANConnectionDevice:1"
	#define IGD_WANIPConnection			"urn:dslforum-org:service:WANIPConnection:1"
#else
	#define IGD_WANDEVICE				"urn:schemas-upnp-org:device:WANDevice:1"
	#define IGD_WANConnectionDevice		"urn:schemas-upnp-org:device:WANConnectionDevice:1"
	#define IGD_WANIPConnection			"urn:schemas-upnp-org:service:WANIPConnection:1"
#endif

void cbGetGenericPortMappingEntry(GUPnPService *service, GUPnPServiceAction *action, gpointer user_data)
{
	gint NewPortMappingIndex;

	//input
	gupnp_service_action_get (action, "NewPortMappingIndex"      , G_TYPE_UINT   , &NewPortMappingIndex,  NULL);

	//output
	gupnp_service_action_set (action, "NewRemoteHost"				, G_TYPE_STRING	, ""				,  NULL);
	gupnp_service_action_set (action, "NewExternalPort"				, G_TYPE_UINT	, 8080				,  NULL);
	gupnp_service_action_set (action, "NewProtocol"					, G_TYPE_STRING	,"TCP"				,  NULL);
	gupnp_service_action_set (action, "NewInternalPort"				, G_TYPE_UINT	, 80				, NULL);
	gupnp_service_action_set (action, "NewInternalClient"			, G_TYPE_STRING	, "192.168.1.777"	, NULL);
	gupnp_service_action_set (action, "NewEnabled"					, G_TYPE_BOOLEAN, 1					, NULL);
	gupnp_service_action_set (action, "NewPortMappingDescription"	, G_TYPE_STRING	, "My WebServer"	, NULL);
	gupnp_service_action_set (action, "NewLeaseDuration"			, G_TYPE_UINT	, 3000				, NULL);

	gupnp_service_action_return (action);
}

void on_context_available(GUPnPContextManager *manager, GUPnPContext *context, gpointer user_data)
{
	GUPnPRootDevice *rootDevice = NULL;
	GUPnPDeviceInfo* WanDevice = NULL;
	GUPnPDeviceInfo* WanConnectionDevice = NULL;
	GUPnPServiceInfo *service = NULL;
	char* deviceDescriptionFile = static_cast<char*>(user_data);

	/* Create root device */
	rootDevice = gupnp_root_device_new (context, deviceDescriptionFile, ".");
	if (rootDevice == NULL)
	{
		printf("cannot create device!\n");
		return ;
	}

	gupnp_root_device_set_available (rootDevice, TRUE);
	printf("upnp device(%s) starts!\n", gssdp_client_get_host_ip( (GSSDPClient*)context));

#ifdef SUPPORT_ORANGE_IGD

	WanDevice = gupnp_device_info_get_device(GUPNP_DEVICE_INFO(rootDevice), "urn:dslforum-org:device:WANDevice:1");
	if (WanDevice == NULL)
	{
		printf("Not found WanDevice!\n");
		return;
	}

	WanConnectionDevice = gupnp_device_info_get_device(GUPNP_DEVICE_INFO(WanDevice), "urn:dslforum-org:device:WANConnectionDevice:1");
	if (WanConnectionDevice == NULL)
	{
		printf("Not found WanConnectionDevice!\n");
		return;
	}

	service = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (WanConnectionDevice), "urn:dslforum-org:service:WANIPConnection:1");
	if (service == NULL)
	{
		printf("Not found WANIPConnection!\n");
		return;
	}

	g_signal_connect (service, "action-invoked::GetGenericPortMappingEntry", G_CALLBACK (cbGetGenericPortMappingEntry), NULL);

#endif
}

void on_context_unavailable(GUPnPContextManager *manager, GUPnPContext *context, gpointer user_data)
{
	printf("[%s]\n", __FUNCTION__);
}

int main (int argc, char **argv)
{
    GMainLoop *main_loop;
    GError *error = NULL;
    GUPnPContext *context;
    GUPnPRootDevice *dev;
	GUPnPWhiteList *white_list;
	GUPnPContextManager* _context_manager;
	INTERFACES interfaces;
	int i;

	if (argc < 2)
	{
		printf("Usage: upnp_server <device_description.xml>\n");
		return -1;
	}

    g_type_init ();

	_context_manager = gupnp_context_manager_create (0);

	if (utility::getNetworkInterfaces(&interfaces) == false)
	{
		printf("Unavailable network interface.");
		return -2;
	}

	white_list = gupnp_context_manager_get_white_list(_context_manager);
	for (string interface : interfaces)
	{
		gupnp_white_list_add_entry(white_list, interface.c_str());
	}
	gupnp_white_list_set_enabled (white_list, TRUE);

	g_signal_connect(_context_manager, "context-available"  , G_CALLBACK(on_context_available)   , argv[1]);
	g_signal_connect(_context_manager, "context-unavailable", G_CALLBACK (on_context_unavailable), NULL);

    /* Run the main loop */
    main_loop = g_main_loop_new (NULL, FALSE);

    g_main_loop_run (main_loop);

    /* Cleanup */
    g_main_loop_unref (main_loop);
    g_object_unref (dev);
    g_object_unref (context);

    return EXIT_SUCCESS;
}
