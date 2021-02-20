#ifndef OFFSTATE_H
#define OFFSTATE_H

#include "state.h"

class LightDevice;

class OffState : public State
{
public:
	OffState();
	bool on(LightDevice* device) override;
	bool off(LightDevice* device) override;
};

#endif // OFFSTATE_H
