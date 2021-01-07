#ifndef FILESOURCE_H
#define FILESOURCE_H

#include <source.h>

class Mediator;

class FileSource : public Source
{
public:
	FileSource(Mediator* mediator, int type);

};

#endif // FILESOURCE_H
