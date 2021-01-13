#ifndef VISITOR_H
#define VISITOR_H

class File;
class Directory;

class Visitor
{
public:
	Visitor();

	virtual void visit(File* file);
	virtual void visit(Directory* dir);
};

#endif // VISITOR_H
