#include "visitor.h"
#include "file.h"
#include "directory.h"
#include "entry.h"

Visitor::Visitor()
{
}

void Visitor::visit(File* file)
{
	file->print();
}
void Visitor::visit(Directory* dir)
{
	dir->print();

	for(int i = 0 ; i < dir->getEntryCount() ; i++)
	{
		Entry* entry = dir->getEntry(i);
		if (entry != nullptr)
		{
			entry->accept(this);
		}
	}
}
