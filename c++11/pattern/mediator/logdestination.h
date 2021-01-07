#ifndef LOGDESTINATION_H
#define LOGDESTINATION_H

#include <destination.h>
#include <string.h>

class LogDestination : public Destination
{
public:
	LogDestination();

	virtual void onSink(std::string text) override;
};

#endif // LOGDESTINATION_H
