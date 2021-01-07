#ifndef MEDIATORIMPL_H
#define MEDIATORIMPL_H

#include <mediator.h>

class Source;

class MediatorImpl : public Mediator
{
public:
	MediatorImpl();	

	void onEvent(Source* source, std::string text) override;

};

#endif // MEDIATORIMPL_H
