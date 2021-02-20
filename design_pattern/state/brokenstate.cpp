#include "brokenstate.h"
#include "lightdevice.h"
#include <iostream>

using namespace std;

BrokenState::BrokenState()
{
}

bool BrokenState::on(LightDevice* device)
{
	cout << "Can't turn on because the device is broken." << endl;
	return false;
}

bool BrokenState::off(LightDevice* device)
{
	cout << "Can't turn off because the device is broken." << endl;
	return false;
}
