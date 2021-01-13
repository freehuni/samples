#include "directory.h"
#include "entry.h"
#include "visitor.h"
#include <iostream>

using namespace std;

Directory::Directory(std::string name) : Entry(name)
{
}

void Directory::accept(Visitor* visitor)
{
	visitor->visit(this);
}

void Directory::print()
{
	std::string name = *this;
	cout << "[DIR] " + name  << endl;
}

void Directory::addEntry(Entry* entry)
{
	mEnties.push_back(entry);
}

int Directory::getEntryCount()
{
	return mEnties.size();
}

Entry* Directory::getEntry(int index)
{
	return mEnties[index];
}
