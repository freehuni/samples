#ifndef ENTRY_H
#define ENTRY_H

#include <string>
#include <stdio.h>

class Visitor;

class Entry
{
public:
	Entry(std::string name);

	virtual void accept(Visitor* visitor) = 0;
	virtual void print() = 0;
	operator std::string();

private:
	std::string mName;
};

#endif // ENTRY_H
