#include <iostream>
#include <unistd.h>
#include <string.h>

using namespace std;

int main(int argc, char *argv[])
{
	int ac =5;
	char*av[]={"test", "-h", "-v", "-f", "hello.c"};
	int opt;
	int opt_ok;
	char file_name[16];

	while((opt = getopt(ac, av, "hvf:")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf(" - option:h:%s\n", file_name);
				break;
			case 'v':
				printf(" - option:v:%s\n", file_name);
				break;
			case 'f':
				memcpy(file_name, optarg, 16);
				opt_ok = 1;
				printf(" - option:f:%s\n", file_name);
				break;
		}
	}

	return 0;
}
