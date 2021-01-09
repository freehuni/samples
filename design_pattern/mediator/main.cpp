#include <iostream>
#include <memory>
#include "person.h"
#include "chatroom.h"

using namespace std;

int main()
{
	shared_ptr<Person> man1 = make_shared<Person>("Ji Ho");
	shared_ptr<Person> man2 = make_shared<Person>("Myung Hun");
	ChatRoom room;

	room.join(man1);
	room.join(man2);

	man1->say("Hello");
	man2->say("Hi~");
	man1->say("Who are you?");
	man2->say("I'm your father!");
	man1->say("Oh my god!");
	man1->bye();

	return 0;
}
