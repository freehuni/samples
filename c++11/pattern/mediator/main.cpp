#include <iostream>
#include <mediatorimpl.h>
#include <filesource.h>
#include <logdestination.h>

using namespace std;

int main()
{
	cout << "Hello World!" << endl;
	MediatorImpl mediator;
	FileSource file(&mediator, 0);
	FileSource file2(&mediator, 1);
	LogDestination log;
	mediator.addDestination(&log);

	file.onFire("hello");
	file2.onFire("hello2");

	return 0;
}
