#include "source.h"
#include <mediator.h>

Source::Source(Mediator* mediator, int type)
{
	this->mediator = mediator;
	this->type = type;
}


void Source::onFire(std::string text)
{
	this->mediator->onEvent(this, text);
}

int Source::getType()
{
	return type;
}
