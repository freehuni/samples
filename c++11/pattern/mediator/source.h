#ifndef SOURCE_H
#define SOURCE_H

#include <string>

class Mediator;

class Source
{
public:
	Source(Mediator* mediator, int type);
	virtual void onFire(std::string text);
	virtual int getType();


private:
	Mediator* mediator;
	int type;
};

#endif // SOURCE_H
