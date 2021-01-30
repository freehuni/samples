#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <map>
#include <tuple>
#include <libgupnp/gupnp.h>
#include "utility.h"
#include "igddevice.h"
#include "menumanager.h"

using namespace std;

CLI::eMENU_RETURN cmdStart(string option, void* user_data);
CLI::eMENU_RETURN cmdStop(string option, void* user_data);
CLI::eMENU_RETURN cmdShowExternalIP(string option, void* user_data);
CLI::eMENU_RETURN cmdChange(string option, void* user_data);
CLI::eMENU_RETURN cmdWanConnection(string option, void* user_data);
CLI::eMENU_RETURN cmdShow(string option, void* user_data);
CLI::eMENU_RETURN cmdQuit(string option, void* user_data);

shared_ptr<IgdDevice> gIGD;
string gDefaultDevDescription = "WANConnectionDevice.xml";

int main(int argc, char* argv[])
{
	CLI::MENU_LIST items = {
		// MenuName		Option						Description			Action		UserData
		{"Start"		, "<DeviceDescription.xml>"	, ""				, cmdStart, nullptr},
		{"Stop"			, ""						, ""				, cmdStop, nullptr},
		{"ExternalIP"	, ""						, ""				, cmdShowExternalIP, nullptr},
		{"Change"		, "<External IP>"			, ""				, cmdChange, nullptr},
		{"WanConnection", "<true or false>"			, ""				, cmdWanConnection, nullptr},
		{"List"			, ""						, "Port Forwarding"	, cmdShow, nullptr},
		{"Quit"			, ""						, ""				, cmdQuit, nullptr}};
	CLI::MenuManager menu("Select the following menu:", items);
	string command;

	do
	{
		menu.show();
		command = menu.getCommand("IGD > ");
	} while(menu.execute(command) == CLI::eContinue);

	if (gIGD != nullptr)
	{
		gIGD->final();
	}

	printf("Bye bye!\n");

	return 0;
}

CLI::eMENU_RETURN cmdStart(string option, void* user_data)
{
	if (gIGD != nullptr)
	{
		printf(">>> IGD is already running.\n");
		return CLI::eContinue;
	}

	string DevDescription;

	if (option.empty())
	{
		printf(">>> No Device Description XML.\n");
		printf(">>> Use default %s\n", gDefaultDevDescription.c_str());
		DevDescription = gDefaultDevDescription;
	}
	else
	{
		DevDescription = option;
	}

	gIGD = make_shared<IgdDevice>(DevDescription.c_str());

	if (gIGD->init() == false)
	{
		printf(">>> gIGD->init failed\n");
	}

	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdStop(string option, void* user_data)
{
	if (gIGD == nullptr)
	{
		printf(">>> IGD does not run yet.\n");
		return CLI::eContinue;
	}

	gIGD->final();
	gIGD.reset();
	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdShowExternalIP(string option, void* user_data)
{
	if (gIGD == nullptr)
	{
		printf(">>> IGD does not run yet.\n");
		return CLI::eContinue;
	}

	const string ExternalIP = gIGD->getExternalIP();
	printf(">>> %s\n", ExternalIP.c_str());

	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdChange(string option, void* user_data)
{
	if (gIGD == nullptr)
	{
		printf(">>> IGD does not run yet.\n");
		return CLI::eContinue;
	}

	if (option.empty())
	{
		printf(">>> Input external IP address of NAT.\n");
		printf(">>> Usage) Change 10.20.30.40\n");
		return CLI::eContinue;
	}

	gIGD->changeExternalIP(option);

	return CLI::eContinue;
}
#include <regex>
CLI::eMENU_RETURN cmdWanConnection(string option, void* user_data)
{
	if (gIGD == nullptr)
	{
		printf(">>> IGD does not run yet.\n");
		return CLI::eContinue;
	}

	regex pattern("true|false");
	smatch m;

	if (option.empty() || regex_match(option, m, pattern) == false)
	{
		printf(">>> Input true or false value\n");
		printf(">>> Usage) WanConnection true\n");
		return CLI::eContinue;
	}

	bool bConnection = (option.compare("true") == 0);
	gIGD->setWanConnected(bConnection);

	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdShow(string option, void* user_data)
{
	if (gIGD == nullptr)
	{
		printf(">>> IGD does not run yet.\n");
		return CLI::eContinue;
	}

	const PORT_MAPPING_ENTRIES Entries = gIGD->getPortEntries();

	if (Entries.size() == 0)
	{
		printf(">>> No Port Mapping Entry.\n");
		return CLI::eContinue;
	}

	int i = 0;
	for(PORT_ENTRY entry : Entries)
	{
		printf(" [%d] ExternalPort:%d RemoteHost:%s Protocol:%s InternalPort:%d InternalClient:%s Enabled:%d Description:%s LeaseDuration:%d\n", i++,
		std::get<eExternalPort>(entry),
		std::get<eRemoteHost>(entry).c_str(),
		std::get<eProtocol>(entry).c_str(),
		std::get<eInternalPort>(entry),
		std::get<eInternalClient>(entry).c_str(),
		std::get<eEnabled>(entry),
		std::get<eDescription>(entry).c_str(),
		std::get<eLeaseDuration>(entry));
	}

	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdQuit(string option, void* user_data)
{
	return CLI::eBack;
}
