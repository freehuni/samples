#include "logdestination.h"
#include <iostream>

using namespace std;

LogDestination::LogDestination()
{

}


void LogDestination::onSink(std::string text)
{
	cout << "onSink:" << text << endl;
}
