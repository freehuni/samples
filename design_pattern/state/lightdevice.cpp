#include "lightdevice.h"
#include "state.h"
#include "offstate.h"

LightDevice::LightDevice()
{
	mState = std::make_shared<OffState>();
}

void LightDevice::changeState(std::shared_ptr<State> state)
{
	mState = state;
}

void LightDevice::on()
{
	mState->on(this);
}

void LightDevice::off()
{
	mState->off(this);
}

void LightDevice::repair()
{
	mState=std::make_shared<OffState>();
}
