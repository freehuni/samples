#ifndef MEDIATOR_H
#define MEDIATOR_H

#include <vector>
#include <string>

class Source;
class Destination;

class Mediator
{
public:
	Mediator();

	void addDestination(Destination* dest);
	virtual void onEvent(Source* source, std::string text) = 0;

protected:
	std::vector<Destination*> Destinations;
};

#endif // MEDIATOR_H
