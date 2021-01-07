#include "mediatorimpl.h"
#include <source.h>
#include <destination.h>
#include <iostream>

using namespace  std;

MediatorImpl::MediatorImpl()
{

}

void MediatorImpl::onEvent(Source* source, std::string text)
{
	switch(source->getType())
	{
	case 0:
		if (Destinations[0] != nullptr)
		{
			Destinations[0]->onSink(text);
		}
		break;
	case 1:
		cout << "not implemented" << endl;
		if (Destinations[1] != nullptr)
		{
			cout << "not null" << endl;
		}
		else
		{
			cout << "null" << endl;
		}
		break;
	default:
		break;
	}
}
