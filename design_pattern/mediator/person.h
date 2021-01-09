#ifndef PERSON_H
#define PERSON_H

#include <string>
#include <vector>

class ChatRoom;

class Person
{
private:
	std::string name;
	ChatRoom* room = nullptr;
	std::vector<std::string> chat_log;

public:
	Person(const std::string name);

	const std::string& get_name();
	void receive(const std::string& origin, const std::string& message);
	void say(const std::string& message) const;
	void set_room(ChatRoom* room);
	void bye();
};

#endif // PERSON_H
