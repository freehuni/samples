#ifndef BROKENSTATE_H
#define BROKENSTATE_H

#include "state.h"

class LightDevice;

class BrokenState : public State
{
public:
	BrokenState();
	bool on(LightDevice* device) override;
	bool off(LightDevice* device) override;
};

#endif // BROKENSTATE_H
