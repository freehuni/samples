#include <algorithm>
#include "person.h"
#include "chatroom.h"

using namespace std;

ChatRoom::ChatRoom()
{
}

void ChatRoom::join(std::shared_ptr<Person> person)
{
	broadcast("room", person->get_name() + " join.");
	person->set_room(this);
	peoples.push_back(person);
}

void ChatRoom::bye(const std::string& who)
{
	std::remove_if(peoples.begin(), peoples.end(),
				   [&](const shared_ptr<Person>& person){ return (person->get_name() == who);});

	broadcast("room", who + " left.");
}

void ChatRoom::broadcast(const std::string&origin, const std::string& message)
{
	for(const shared_ptr<Person>& person :  peoples)
	{
		if (origin != person->get_name())
		{
			person->receive(origin, message);
		}
	}
}
