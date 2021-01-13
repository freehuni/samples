#ifndef FILE_H
#define FILE_H

#include <string>
#include "entry.h"

class File : public Entry
{
public:
	File(std::string name);
	void accept(Visitor* visitor) override;
	void print() override;
};

#endif // FILE_H
