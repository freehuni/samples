#ifndef STATE_H
#define STATE_H

class LightDevice;

class State
{
public:
	State();

	virtual bool on(LightDevice* device) = 0;
	virtual bool off(LightDevice* device) = 0;
};

#endif // STATE_H
