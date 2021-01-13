#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <vector>
#include <string>
#include "entry.h"

class Visitor;

class Directory : public Entry
{
public:
	Directory(std::string name);

	void accept(Visitor* visitor) override;
	void print() override;

	void addEntry(Entry* entry);
	int getEntryCount();
	Entry* getEntry(int index);

private:
	std::vector<Entry*> mEnties;
};

#endif // DIRECTORY_H
