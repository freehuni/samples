#include "mediator.h"

Mediator::Mediator()
{

}

void Mediator::addDestination(Destination* dest)
{
	this->Destinations.push_back(dest);
}
