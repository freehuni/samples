#include <iostream>
#include "menumanager.h"

using namespace std;


CLI::eMENU_RETURN cmdStart(string option, void* user_data)
{	
	printf("%s(%s) : Do something\n", __FUNCTION__, option.c_str());
	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdStop(string option, void* user_data)
{
	printf("%s(%s) : Do something\n", __FUNCTION__, option.c_str());
	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdShow(string option, void* user_data)
{
	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdBack(string option, void* user_data)
{
	return CLI::eBack;
}

CLI::eMENU_RETURN cmdSubMenu(string option, void* user_data)
{
	CLI::MENU_LIST subItems = {{"Show", "", "", cmdShow, nullptr}, {"Back", "", "", cmdBack, nullptr}};
	string command;
	printf("%s(%s) : Do something\n", __FUNCTION__, option.c_str());

	CLI::MenuManager menu("Select sub menus:", subItems);
	do
	{
		menu.show();

		command = menu.getCommand("Console> ");
	} while(menu.execute(command) == CLI::eContinue);

	return CLI::eContinue;
}

CLI::eMENU_RETURN cmdQuit(string option, void* user_data)
{
	printf("%s(%s) : exit.\n", __FUNCTION__, option.c_str());

	return CLI::eBack;
}



int main()
{
	CLI::MENU_LIST items = {{"Start", "option1", "", cmdStart, nullptr}, {"Stop", "option2", "", cmdStop, nullptr}, {"Option", "param","goto sub menu", cmdSubMenu, nullptr}, {"Quit", "","", cmdQuit, nullptr}};
	CLI::MenuManager menu("Select the following menu number:", items);
	string command;

	do
	{
		menu.show();

		command = menu.getCommand("$ > ");

	} while(menu.execute(command) == CLI::eContinue);

	return 0;
}
