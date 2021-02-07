#include "hello.h"

static int g_value=0;

void setValue(int value)
{
	g_value = value;
}

int getValue(void)
{
	return g_value;
}
