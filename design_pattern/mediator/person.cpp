#include <iostream>
#include "person.h"
#include "chatroom.h"

using namespace std;

Person::Person(const std::string name)
{
	this->name = name;
}

const std::string& Person::get_name()
{
	return name;
}

void Person::receive(const std::string& origin, const string& message)
{
	printf("[%-10s session] %-10s: %s\n", name.c_str(), origin.c_str(), message.c_str());
}

void Person::say(const std::string& message) const
{
	room->broadcast(name, message);
}

void Person::set_room(ChatRoom* room)
{
	this->room = room;
}

void Person::bye()
{
	room->bye(name);
}
