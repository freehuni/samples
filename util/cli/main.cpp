#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <tuple>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

class CLI
{

public:
	static string getCommand(string prompt, bool addHistory = false)
	{
		string command;
		char* line;
		line = readline(prompt.c_str());

		if (addHistory == true && *line ) add_history(line);

		command = line;
		free(line);

		return command;
	}
};

typedef struct
{
	string name;
	string option;
	function<bool(string)>  cbExecute;

} MenuItem;

class MenuManager
{
public:
	typedef function<bool(string)>				MENU_ACTION;
	typedef tuple<string, string, MENU_ACTION>	MENU_ITEM;
	typedef vector<MENU_ITEM>					MENU_LIST;
	typedef MENU_LIST::iterator					MENU_IT;

private:

	string		_menuTitle;
	MENU_LIST	_menuItems;

public:
	MenuManager(string menuTitle, MENU_LIST menuItems)
	{
		_menuTitle = menuTitle;
		_menuItems = menuItems;
	}
	~MenuManager()
	{
		_menuItems.clear();
	}

	void show()
	{		
		int i;

		printf("\n%s\n", _menuTitle.c_str());

		i = 0;
		for (MENU_ITEM item: _menuItems)
		{			
			printf("  [%d] %s %s\n", i++, std::get<0>(item).c_str() , std::get<1>(item).c_str());
		}
	}

	bool execute(string param)
	{
		std::size_t pos = param.find(" ");
		string cmd, arg;
		int menuIndex = -1;

		if (pos != string::npos)
		{
			cmd = param.substr(0, pos);
			arg = param.substr(pos+1);
		}
		else
		{
			cmd = param;
		}

		sscanf(cmd.c_str(), "%d", &menuIndex);

		if (menuIndex >= 0)
		{
			if (menuIndex >= _menuItems.size())
			{
				printf("Select index 0 ~ %d\n", _menuItems.size() - 1);
				return false;
			}

			std::get<2>(_menuItems[menuIndex])(arg);
		}

		for (MENU_ITEM item: _menuItems)
		{
			if (cmd.compare(std::get<0>(item)) == 0)
			{
				std::get<2>(item)( arg );
				return true;
			}
		}

		if (param.size() > 0) printf("Unknown command: %s\n", param.c_str());

		return false;
	}
};

// MENU Porting Layer =============================================================

bool cmdStart(string arg)
{
	printf("%s(%s) : Do something\n", __FUNCTION__, arg.c_str());
	return true;
}

bool cmdStop(string arg)
{
	printf("%s(%s) : Do something\n", __FUNCTION__, arg.c_str());
	return true;
}

bool isRunning = 1;

bool cmdQuit(string arg)
{
	printf("%s(%s) : exit.\n", __FUNCTION__, arg.c_str());
	isRunning = 0;

	return true;
}

int main()
{
	MenuManager::MENU_LIST items = {{"start", "loglevel=1",  cmdStart}, {"stop", "", cmdStop}, {"quit", "", cmdQuit}};
	MenuManager menu("Select the following menu:", items);

	while(isRunning)
	{
		menu.show();

		string command = CLI::getCommand("ACE> ");

		menu.execute(command);
	}

	return 0;
}
