#include <iostream>

using namespace std;

#include "directory.h"
#include "file.h"
#include "visitor.h"

int main()
{
	Visitor visitor;
	Directory root("root");
	Directory usr("usr");
	Directory etc("etc");
	File file1("root.txt");
	File file2("jeeho.txt");
	File file3("jeewon.txt");

	root.addEntry(&usr);
	root.addEntry(&etc);
	root.addEntry(&file1);
	usr.addEntry(&file2);
	usr.addEntry(&file3);

	root.accept(&visitor);

	return 0;
}

