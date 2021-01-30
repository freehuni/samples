#include "igddevice.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <libgupnp/gupnp.h>
#include <sys/sysinfo.h>
#include "utility.h"

using namespace std;


class IgdDeviceImpl
{
public:
	IgdDeviceImpl(string deviceDescription) : mDeviceDescription(deviceDescription), mExternalIPAddress("10.10.20.20"), mWanConnected(true)
	{}
	virtual ~IgdDeviceImpl()
	{}

	bool init()
	{
		unique_lock<mutex> lock(mMutex);
		mContextManager = gupnp_context_manager_create(0);

		initNetworkInterface();

		mThread = thread(&IgdDeviceImpl::workerThread, this);

		cv_status status = mCond.wait_for(lock, chrono::seconds(5));
		if (cv_status::timeout == status)
		{
			return false;
		}

		return (mLastError == 0);
	}

	bool final()
	{
		if (mRootDevice != nullptr)
		{
			g_object_unref(mRootDevice);
			mRootDevice = nullptr;
		}

		if (mContextManager != nullptr)
		{
			g_object_unref(mContextManager);
			mContextManager = nullptr;
		}

		if (mMainLoop != nullptr)
		{
			g_main_loop_quit(mMainLoop);

			if (mThread.joinable())
			{
				mThread.join();
			}

			mMainLoop = nullptr;
		}

		return true;
	}

	const string& getExternalIP()
	{
		return mExternalIPAddress;
	}

	bool changeExternalIP(std::string ip)
	{
		if (mWanConnected == false)
		{
			printf("WAN is not connected!\n");
			return false;
		}

		mExternalIPAddress = ip;

		printf("Send Event ExternalIPAddress:%s\n", ip.c_str());
		gupnp_service_notify((GUPnPService*)mService, "ExternalIPAddress", G_TYPE_STRING, mExternalIPAddress.c_str(), nullptr);

		return true;
	}

	void setRootDevice(GUPnPRootDevice* rootDevice)
	{
		mRootDevice = rootDevice;
	}

	void setUpnpService(GUPnPServiceInfo* service)
	{
		mService = service;
	}

	string getExternalIPAddress()
	{
		return mExternalIPAddress;
	}

	const PORT_MAPPING_ENTRIES& getPortEntries()
	{
		return mPortEnties;
	}

	void setWanConnected(bool bConnected)
	{
		mWanConnected = bConnected;
	}

	bool isWanConnected()
	{
		return mWanConnected;
	}

protected:
	const PORT_ENTRY* getEntry(int ExternalPort)
	{
		for(const PORT_ENTRY& entry : mPortEnties)
		{
			if (std::get<eExternalPort>(entry) == ExternalPort)
			{
				return &entry;
			}
		}

		return nullptr;
	}
	const PORT_ENTRY* getEntryByIndex(int index)
	{
		if (index < 0 || mPortEnties.size()<= index)
		{
			return nullptr;
		}

		return &mPortEnties[index];
	}

	void addEntry(const PORT_ENTRY& entry)
	{
		mPortEnties.push_back(entry);
	}

	bool deletgeEntry(int ExternalPort)
	{
		for(ENTRY_IT it = mPortEnties.begin() ; it != mPortEnties.end() ; it++)
		{
			if (std::get<eExternalPort>(*it) == ExternalPort)
			{
				mPortEnties.erase(it);
				return true;
			}
		}

		return false;
	}

	void notifyCondition(int ErrCode)
	{
		mLastError = ErrCode;
		mCond.notify_one();
	}

	void initNetworkInterface()
	{
		INTERFACES interfaces;
		GUPnPWhiteList* white_list = nullptr;

		white_list = gupnp_context_manager_get_white_list(mContextManager);
		for (string interface : interfaces)
		{
			gupnp_white_list_add_entry(white_list, interface.c_str());
		}
		gupnp_white_list_set_enabled(white_list, true);
	}

	void workerThread()
	{
		g_signal_connect(mContextManager, "context-available", G_CALLBACK(IgdDeviceImpl::onContextAvailable), this);
		g_signal_connect(mContextManager, "context-unavailable", G_CALLBACK(IgdDeviceImpl::onContextUnavailable), this);

		mMainLoop = g_main_loop_new(nullptr, false);

		g_main_loop_run(mMainLoop);
	}

	string getDeviceDescription()
	{
		return mDeviceDescription;
	}

	static void onContextAvailable(GUPnPContextManager *manager, GUPnPContext *context, gpointer user_data)
	{
		GUPnPRootDevice *rootDevice = nullptr;
		GUPnPDeviceInfo* WanDevice = nullptr;
		GUPnPDeviceInfo* WanConnectionDevice = nullptr;
		GUPnPServiceInfo *service = nullptr;
		GError* error = nullptr;
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);

		/* Create root device */
		rootDevice = gupnp_root_device_new (context, IgdImpl->getDeviceDescription().c_str(), ".", &error);
		if (rootDevice == nullptr)
		{
			printf("cannot create device!\n");
			IgdImpl->notifyCondition(-1);
			return ;
		}

		IgdImpl->setRootDevice(rootDevice);

		gupnp_root_device_set_available (rootDevice, TRUE);
		printf("upnp device(%s) starts!\n", gssdp_client_get_host_ip( (GSSDPClient*)context));

		// Bind WANIPConn Service APIs
		WanDevice = gupnp_device_info_get_device(GUPNP_DEVICE_INFO(rootDevice), "urn:schemas-upnp-org:device:WANDevice:1");
		if (WanDevice == nullptr)
		{
			printf("Not found WanDevice!\n");
			IgdImpl->notifyCondition(-2);
			return;
		}

		WanConnectionDevice = gupnp_device_info_get_device(GUPNP_DEVICE_INFO(WanDevice), "urn:schemas-upnp-org:device:WANConnectionDevice:1");
		if (WanConnectionDevice == nullptr)
		{
			printf("Not found WanConnectionDevice!\n");
			IgdImpl->notifyCondition(-3);
			return;
		}

		service = gupnp_device_info_get_service (GUPNP_DEVICE_INFO (WanConnectionDevice), "urn:schemas-upnp-org:service:WANIPConnection:1");

		if (service == nullptr)
		{
			printf("Not found WANIPConnection!\n");
			IgdImpl->notifyCondition(-4);
			return;
		}

		IgdImpl->setUpnpService(service);

		g_signal_connect (service, "action-invoked::AddPortMapping", G_CALLBACK (cbAddPortMapping), IgdImpl);
		g_signal_connect (service, "action-invoked::DeletePortMapping", G_CALLBACK (cbDeletePortMapping), IgdImpl);
		g_signal_connect (service, "action-invoked::GetStatusInfo", G_CALLBACK (cbGetStatusInfo), IgdImpl);
		g_signal_connect (service, "action-invoked::GetExternalIPAddress", G_CALLBACK (cbGetExternalIPAddress), IgdImpl);
		g_signal_connect (service, "action-invoked::GetGenericPortMappingEntry", G_CALLBACK (cbGetGenericPortMappingEntry), IgdImpl);
		IgdImpl->notifyCondition(0);
	}

	static void onContextUnavailable(GUPnPContextManager *manager, GUPnPContext *context, gpointer user_data)
	{
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);
	}

	static void cbAddPortMapping(GUPnPService *service, GUPnPServiceAction *action, gpointer user_data)
	{
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);
		gchar* NewRemoteHost = nullptr;
		guint NewExternalPort;
		gchar* NewProtocol = nullptr;
		guint NewInternalPort;
		gchar* NewInternalClient = nullptr;
		gboolean NewEnabled;
		gchar*NewPortMappingDescription = nullptr;
		guint NewLeaseDuration;

		//input
		gupnp_service_action_get(action, "NewRemoteHost"			, G_TYPE_STRING , &NewRemoteHost,  nullptr);
		gupnp_service_action_get(action, "NewExternalPort"			, G_TYPE_UINT   , &NewExternalPort,  nullptr);
		gupnp_service_action_get(action, "NewProtocol"				, G_TYPE_STRING , &NewProtocol,  nullptr);
		gupnp_service_action_get(action, "NewInternalPort"			, G_TYPE_UINT   , &NewInternalPort,  nullptr);
		gupnp_service_action_get(action, "NewInternalClient"		, G_TYPE_STRING , &NewInternalClient,  nullptr);
		gupnp_service_action_get(action, "NewEnabled"				, G_TYPE_BOOLEAN, &NewEnabled,  nullptr);
		gupnp_service_action_get(action, "NewPortMappingDescription", G_TYPE_STRING , &NewPortMappingDescription,  nullptr);
		gupnp_service_action_get(action, "NewLeaseDuration"			, G_TYPE_UINT   , &NewLeaseDuration,  nullptr);

		if (IgdImpl->getEntry(NewExternalPort) != nullptr)
		{
			gupnp_service_action_return_error(action, 700, "Already Exist");
			return;
		}

		 // ExternalPort, RemoteHost, Protocol, InternalPort, InternalClient, Enabled, Description, LeaseDuration
		IgdImpl->addEntry(std::make_tuple(NewExternalPort, NewRemoteHost, NewProtocol, NewInternalPort, NewInternalClient, NewEnabled, NewPortMappingDescription, NewLeaseDuration));

		gupnp_service_action_return (action);
	}

	static void cbDeletePortMapping(GUPnPService *service, GUPnPServiceAction *action, gpointer user_data)
	{
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);
		gchar* NewRemoteHost = nullptr;
		guint NewExternalPort;
		gchar* NewProtocol = nullptr;

		// Input
		gupnp_service_action_get(action, "NewRemoteHost"			, G_TYPE_STRING , &NewRemoteHost,  nullptr);
		gupnp_service_action_get(action, "NewExternalPort"			, G_TYPE_UINT   , &NewExternalPort,  nullptr);
		gupnp_service_action_get(action, "NewProtocol"				, G_TYPE_STRING , &NewProtocol,  nullptr);

		if (IgdImpl->deletgeEntry(NewExternalPort) == false)
		{
			gupnp_service_action_return_error(action, 404, "Not Found");
			return;
		}

		gupnp_service_action_return (action);
	}

	static void cbGetStatusInfo(GUPnPService *service, GUPnPServiceAction *action, gpointer user_data)
	{
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);

		// input
		//	nothing

		// output
		string NewConnectionStatus;
		string NewLastConnectionError;
		if (IgdImpl->isWanConnected())
		{
			NewConnectionStatus = "Connected";
			NewLastConnectionError = "ERROR_NONE";
		}
		else
		{
			NewConnectionStatus = "Disconnected";
			NewLastConnectionError = "ERROR_USER_DISCONNECT";
		}
		gupnp_service_action_set(action, "NewConnectionStatus"		, G_TYPE_STRING	, NewConnectionStatus.c_str(),  nullptr);
		gupnp_service_action_set(action, "NewLastConnectionError"	, G_TYPE_STRING	, NewLastConnectionError.c_str(),  nullptr);

		struct sysinfo info;
		sysinfo(&info);
		gupnp_service_action_set(action, "NewUptime"				, G_TYPE_UINT	, info.uptime,  nullptr);

		gupnp_service_action_return(action);
	}

	static void cbGetExternalIPAddress(GUPnPService *service, GUPnPServiceAction *action, gpointer user_data)
	{
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);

		// input
		//	nothing

		// output
		gupnp_service_action_set(action, "NewExternalIPAddress"		, G_TYPE_STRING	, IgdImpl->getExternalIPAddress().c_str() ,  nullptr);
		gupnp_service_action_return(action);
	}

	static void cbGetGenericPortMappingEntry(GUPnPService *service, GUPnPServiceAction *action, gpointer user_data)
	{
		IgdDeviceImpl* IgdImpl = static_cast<IgdDeviceImpl*>(user_data);
		guint NewPortMappingIndex;
		const PORT_ENTRY* entry;

		//input
		gupnp_service_action_get (action, "NewPortMappingIndex"      , G_TYPE_UINT   , &NewPortMappingIndex,  nullptr);

		entry = IgdImpl->getEntryByIndex(NewPortMappingIndex);
		if (entry == nullptr)
		{
			gupnp_service_action_return_error(action, 404, "Not Found");
			return;
		}

		//output
		gupnp_service_action_set(action, "NewRemoteHost"			, G_TYPE_STRING	, std::get<eRemoteHost>(*entry).c_str()		, nullptr);
		gupnp_service_action_set(action, "NewExternalPort"			, G_TYPE_UINT	, std::get<eExternalPort>(*entry)			, nullptr);
		gupnp_service_action_set(action, "NewProtocol"				, G_TYPE_STRING	, std::get<eProtocol>(*entry).c_str()		, nullptr);
		gupnp_service_action_set(action, "NewInternalPort"			, G_TYPE_UINT	, std::get<eInternalPort>(*entry)			, nullptr);
		gupnp_service_action_set(action, "NewInternalClient"		, G_TYPE_STRING	, std::get<eInternalClient>(*entry).c_str()	, nullptr);
		gupnp_service_action_set(action, "NewEnabled"				, G_TYPE_BOOLEAN, std::get<eEnabled>(*entry)				, nullptr);
		gupnp_service_action_set(action, "NewPortMappingDescription", G_TYPE_STRING	, std::get<eDescription>(*entry).c_str()	, nullptr);
		gupnp_service_action_set(action, "NewLeaseDuration"			, G_TYPE_UINT	, std::get<eLeaseDuration>(*entry)			, nullptr);

		gupnp_service_action_return (action);
	}

private:
	thread mThread;
	GUPnPContextManager* mContextManager = nullptr;
	GUPnPRootDevice* mRootDevice = nullptr;
	GUPnPServiceInfo* mService = nullptr;
	GMainLoop* mMainLoop = nullptr;
	string mDeviceDescription;
	PORT_MAPPING_ENTRIES mPortEnties;
	condition_variable mCond;
	mutex mMutex;
	int mLastError = 0;
	string mExternalIPAddress;
	bool mWanConnected;
};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///  Implementation of IgdDevice class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
IgdDevice::IgdDevice(string deviceDescription)
{
	mImpl = new IgdDeviceImpl(deviceDescription);
}

bool IgdDevice::init()
{
	return mImpl->init();
}

bool IgdDevice::final()
{
	return mImpl->final();
}

const string& IgdDevice::getExternalIP()
{
	return mImpl->getExternalIP();
}

bool IgdDevice::changeExternalIP(std::string ip)
{
	return mImpl->changeExternalIP(ip);
}

const PORT_MAPPING_ENTRIES& IgdDevice::getPortEntries()
{
	return mImpl->getPortEntries();
}

void IgdDevice::setWanConnected(bool bConnected)
{
	return mImpl->setWanConnected(bConnected);
}
