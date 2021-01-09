#ifndef CHATROOM_H
#define CHATROOM_H

#include <string>

#include <vector>
#include <memory>

class Person;

class ChatRoom
{
public:
	ChatRoom();

	void join(std::shared_ptr<Person> person);
	void bye(const std::string& who);
	void broadcast(const std::string&origin, const std::string& message);

	std::vector<std::shared_ptr<Person>> peoples;
};

#endif // CHATROOM_H
