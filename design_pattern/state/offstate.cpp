#include "offstate.h"
#include "onstate.h"
#include "lightdevice.h"
#include <iostream>

using namespace std;

OffState::OffState()
{

}

bool OffState::on(LightDevice* device)
{
	device->changeState(make_shared<OnState>());
	cout << "Turn on the device." << endl;
	return true;
}

bool OffState::off(LightDevice* device)
{
	cout << "Already off" << endl;
	return false;
}
