#include <regex>
#include <readline/readline.h>
#include <readline/history.h>
#include "menumanager.h"

using namespace  std;

namespace CLI
{
	MenuManager::MenuManager(const string& menuTitle, const MENU_LIST& menuItems) : mMenuTitle(menuTitle), mMenuItems(menuItems)
	{
	}
	MenuManager::~MenuManager()
	{
		mMenuItems.clear();
	}

	string MenuManager::getCommand(string prompt, bool addHistory)
	{
		string command;
		char* line;
		line = readline(prompt.c_str());

		if (addHistory == true && *line ) add_history(line);

		command = line;
		free(line);

		return command;
	}

	void MenuManager::show()
	{
		printf("\n%s\n", mMenuTitle.c_str());

		int i = 0;
		for (MENU_ITEM item: mMenuItems)
		{
			if (std::get<eMenuDescription>(item).empty())	// If no description string eixsts,
			{
				printf("  [%d] %s %s\n", i++,
						std::get<eMenuName>(item).c_str() ,
						std::get<eMenuOption>(item).c_str());
			}
			else
			{
				printf("  [%d] %s %s # %s\n", i++,
						std::get<eMenuName>(item).c_str() ,
						std::get<eMenuOption>(item).c_str(),
						std::get<eMenuDescription>(item).c_str());
			}
		}
	}

	eMENU_RETURN MenuManager::execute(string param)
	{
		int menuIndex = -1;
		void* user_data= nullptr;
		vector<string> tokens;

		tokens = getTokens(param, ' ');
		if (tokens.size() == 0)
		{
			printf(">>> Invalid param:%s\n", param.c_str());
			return eContinue;
		}

		if (tokens.size() > 2)
		{
			printf(">>> Parameters are tool many:%s. Ignore parameters.\n", param.c_str());
		}

		menuIndex = getMenuIndex(tokens[0]);
		if (menuIndex < 0 || mMenuItems.size() <= menuIndex)
		{

			printf(">>> Select index 0 ~ %d\n", mMenuItems.size() - 1);
			return eContinue;
		}

		user_data = std::get<eMenuUserData>(mMenuItems[menuIndex]);

		if (tokens.size() == 1)
		{
			return std::get<eMenuAction>(mMenuItems[menuIndex])("", user_data);
		}
		else
		{
			return std::get<eMenuAction>(mMenuItems[menuIndex])(tokens[1], user_data);
		}
	}

	int MenuManager::getMenuIndex(string menuNameOrIndex)
	{
		if (isNumber(menuNameOrIndex))
		{
			return std::stoi(menuNameOrIndex);
		}
		else
		{
			int i = 0;
			for(MENU_ITEM item : mMenuItems)
			{
				if(std::get<eMenuName>(item).compare(menuNameOrIndex) == 0)
				{
					return i;
				}

				i++;
			}
		}

		return -1;
	}

	bool MenuManager::isNumber(string param)
	{
		regex pattern("[0-9]+");
		smatch m;

		return regex_match(param, m, pattern);
	}

	std::vector<std::string> MenuManager::getTokens(const std::string& data, const char delimiter)
	{
		vector<string> result;
		string token;
		stringstream ss(data);

		while(getline(ss, token, delimiter))
		{
			result.push_back(token);
		}

		return result;
	}

}
