#include <iostream>
#include <vector>

using namespace std;

/**
		foreground background
black      30         40
red        31         41
green      32         42
yellow     33         43
blue       34         44
magenta    35         45
cyan       36         46
white      37         47
 */

// Reference Site: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
int main()
{
	vector<unsigned char> foreColorTables={30,31,32,33,34,35,36,37};
	vector<unsigned char> backColorTables={40,41,42,43,44,45,46,47};

	for(unsigned char backColor : backColorTables)
	{
		for(unsigned char foreColor : foreColorTables)
		{
			if (backColor == 40)
			{
				// if BackColor is black, background coloe should be transparent.
				printf("\033[1;%dmCOLOR TEXT\033[0m  [1:%d:%d] ",foreColor,foreColor, backColor);
				printf("\033[2;%dmCOLOR TEXT\033[0m  [2:%d:%d]\n" ,foreColor,foreColor, backColor);
			}
			else
			{
				printf("\033[1;%d;%dmCOLOR TEXT\033[0m  [1:%d:%d] ",foreColor, backColor,foreColor, backColor);
				printf("\033[2;%d;%dmCOLOR TEXT\033[0m  [2:%d:%d]\n" ,foreColor, backColor,foreColor, backColor);
			}

		}
	}

	return 0;
}
