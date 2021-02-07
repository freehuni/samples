#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef int (*HELLO_GetValue)(void);
typedef void (*HELLO_SetValue)(int value);

HELLO_GetValue pGetValue;
HELLO_SetValue pSetValue;
void *handle;
char *error;

int main()
{
	handle = dlopen ("../../library/libhello.so", RTLD_LAZY);
	if (!handle)
	{
		fputs (dlerror(), stderr);
		exit(1);
	}

	pGetValue = (HELLO_GetValue)dlsym(handle, "getValue");
	if ((error = dlerror()) != NULL)
	{
		fputs(error, stderr);
		exit(1);
	}

	pSetValue = (HELLO_SetValue)dlsym(handle, "setValue");
	if ((error = dlerror()) != NULL)
	{
		fputs(error, stderr);
		exit(1);
	}

	pSetValue(1234);
	printf("value:%d\n", pGetValue());

	dlclose(handle);

	return 0;
}

