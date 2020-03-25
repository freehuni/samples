#include <iostream>
#include <string>
#include <log4cxx/log4cxx.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

using namespace std;
using namespace log4cxx;

int main()
{
	string file="log4cxx.conf";
	PropertyConfigurator::configure(File(file));
	LoggerPtr rootlog=Logger::getRootLogger();
	LoggerPtr log = Logger::getLogger("");


	LOG4CXX_DEBUG(log, "Hello");
	LOG4CXX_WARN(log, "Freehuni");

	return 0;
}
