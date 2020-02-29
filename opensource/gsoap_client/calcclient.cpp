#include <stdio.h>
#include "calc.nsmap"
#include "soapcalcProxy.h"

using namespace std;

int main()
{
    calcProxy calc;
    double sum;

    if (calc.add(1.25, 3.55, sum) == SOAP_OK)
    {
        std::cout << "Sum=" << sum << endl;
    }
    else
    {
        calc.soap_stream_fault(std::cerr);
    }

    calc.destroy();
}
