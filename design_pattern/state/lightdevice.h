#ifndef LIGHTDEVICE_H
#define LIGHTDEVICE_H

#include <memory>

class State;

class LightDevice
{
public:
	LightDevice();

	void changeState(std::shared_ptr<State> state);
	void on();
	void off();
	void repair();

private:
	std::shared_ptr<State> mState;


};

#endif // LIGHTDEVICE_H
