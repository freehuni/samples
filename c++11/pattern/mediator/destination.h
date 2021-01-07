#ifndef DESTINATION_H
#define DESTINATION_H

#include <string>

class Destination
{
public:
	Destination();

	virtual void onSink(std::string text) = 0;
};

#endif // DESTINATION_H
