#include "entry.h"

Entry::Entry(std::string name) : mName(name)
{

}

Entry::operator std::string()
{
	return mName;
}
