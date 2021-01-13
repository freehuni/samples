#include "file.h"
#include "visitor.h"
#include <iostream>

using namespace std;

File::File(std::string name) : Entry(name)
{

}

void File::accept(Visitor* visitor)
{
	visitor->visit(this);
}

void File::print()
{
	std::string name = *this;
	cout << " - " + name  << endl;
}
