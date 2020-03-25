#include <iostream>
#include <string>
#include <log4cxx/log4cxx.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>

using namespace std;
using namespace log4cxx;

LoggerPtr rootlog=Logger::getRootLogger();
LoggerPtr log = Logger::getLogger("");

class Base
{
public:
	Base()
	{

	}
	~Base()
	{
		LOG4CXX_ERROR(log, "Destructor");
	}
	void GetValue()
	{
		LOG4CXX_INFO(log, "My name is airboy.");
	}
};

int main()
{
	string file="log4cxx.conf";
	PropertyConfigurator::configure(File(file));
	Base obj;

	obj.GetValue();

	LOG4CXX_DEBUG(log, "Hello");
	LOG4CXX_WARN(log, "Freehuni");

	return 0;
}
