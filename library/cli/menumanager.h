#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <functional>
#include <vector>
#include <string>
#include <tuple>

namespace CLI
{
typedef enum
{
	eContinue,
	eBack
} eMENU_RETURN;

typedef enum
{
	eMenuName = 0,
	eMenuOption,
	eMenuDescription,
	eMenuAction,
	eMenuUserData
} eITEM_INDEX;

using MENU_ACTION=std::function<eMENU_RETURN(std::string, void*)>;
using MENU_ITEM=std::tuple<std::string, std::string, std::string, MENU_ACTION, void*>;
using MENU_LIST=std::vector<MENU_ITEM>;
using MENU_IT=MENU_LIST::iterator;

class MenuManager
{
public:
	MenuManager(const std::string& menuTitle, const MENU_LIST& menuItems);
	~MenuManager();

	std::string getCommand(std::string prompt, bool addHistory = false);
	void show();
	eMENU_RETURN execute(std::string param);

protected:
	int getMenuIndex(std::string menuNameOrIndex);
	bool isNumber(std::string param);
	std::vector<std::string> getTokens(const std::string& data, const char delimiter);

private:
	std::string	mMenuTitle;
	MENU_LIST	mMenuItems;
};
}

#endif // MENUMANAGER_H
