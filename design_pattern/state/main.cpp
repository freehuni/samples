#include <iostream>
#include "lightdevice.h"
#include "brokenstate.h"

using namespace std;

int main()
{
	LightDevice device;
	std::shared_ptr<State> brokenState=make_shared<BrokenState>();

	device.off();
	device.off();
	device.on();
	device.changeState(brokenState);
	device.on();
	device.off();
	device.repair();
	device.on();
	device.off();

	return 0;
}
