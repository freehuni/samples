#include "onstate.h"
#include "offstate.h"
#include "lightdevice.h"
#include <iostream>

using namespace std;

OnState::OnState()
{

}

bool OnState::on(LightDevice* device)
{
	cout << "Already On" << endl;
	return false;
}

bool OnState::off(LightDevice* device)
{
	device->changeState(make_shared<OffState>());
	cout << "OnState::off" << endl;
	return true;
}
