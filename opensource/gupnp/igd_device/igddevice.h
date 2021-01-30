#ifndef IGDDEVICE_H
#define IGDDEVICE_H

#include <string>
#include <tuple>
#include <vector>

typedef enum {
	eExternalPort = 0,
	eRemoteHost,
	eProtocol,
	eInternalPort,
	eInternalClient,
	eEnabled,
	eDescription,
	eLeaseDuration,
	eMax
} ENTRY_INDEX;

using PORT_ENTRY=std::tuple<int, std::string, std::string , int, std::string, bool, std::string, uint>; // ExternalPort, <RemoteHost, Protocol, InternalPort, InternalClient, Enabled, Description, LeaseDuration>
using PORT_MAPPING_ENTRIES = std::vector<PORT_ENTRY>;
using ENTRY_IT= PORT_MAPPING_ENTRIES::iterator;

class IgdDeviceImpl;

class IgdDevice
{
public:
	IgdDevice(std::string deviceDescription);

	bool init();
	bool final();
	const std::string& getExternalIP();
	bool changeExternalIP(std::string ip);
	const PORT_MAPPING_ENTRIES& getPortEntries();
	void setWanConnected(bool bConnected);

private:
	IgdDeviceImpl* mImpl;

};

#endif // IGDDEVICE_H
