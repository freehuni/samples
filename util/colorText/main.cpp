#include <iostream>

using namespace std;

int main()
{
/*
    printf("\x1B[31mTexting\033[0m\n");  // Red
    printf("\x1B[32mTexting\033[0m\n");  // Green
    printf("\x1B[33mTexting\033[0m\n");  // Yellow
    printf("\x1B[34mTexting\033[0m\n");  // Blue
    printf("\x1B[35mTexting\033[0m\n");  // Magenta

    printf("\x1B[36mTexting\033[0m\n"); // Cyon
    printf("\x1B[37mTexting\033[0m\n"); // Gray
    printf("\x1B[39mTexting\033[0m\n"); // White
    printf("\x1B[93mTexting\033[0m\n"); // Light Yellow

    printf("\033[3;42;30mTexting\033[0m\n");    // black/green
    printf("\033[3;43;30mTexting\033[0m\n");    // black/yellow
    printf("\033[3;43;31mTexting\033[0m\n");    // red/yellow
    printf("\033[3;44;30mTexting\033[0m\n");    // black/blue
    printf("\033[3;104;30mTexting\033[0m\n");   // black/light blue
    printf("\033[3;100;30mTexting\033[0m\n");   // black/dark gray

    printf("\033[3;47;35mTexting\033[0m\n");
    printf("\033[2;47;31mTexting\033[0m\n");
    printf("\033[1;47;35mTexting\033[0m\n");
*/
    for(int i=0 ; i < 100 ; i++)
    {
        printf("\x1B[%dmTexting(%d)\033[0m\n", i, i);
    }

    return 0;
}
