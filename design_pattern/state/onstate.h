#ifndef ONSTATE_H
#define ONSTATE_H

#include "state.h"

class LightDevice;

class OnState : public State
{
public:
	OnState();
	bool on(LightDevice* device) override;
	bool off(LightDevice* device) override;
};

#endif // ONSTATE_H
