#include <iostream>
#include <unistd.h>
#include "TcpAcceptor.h"

using namespace std;

int main()
{
    TcpAcceptor acceptor;


    acceptor.Start(9090);

    while(1)
    {
        usleep(10000);
    }

    acceptor.Stop();
    return 0;
}
