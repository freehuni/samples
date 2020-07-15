#include <iostream>
#include <string>
#include <map>
#include <future>
#include "logger.h"
#include <unistd.h>

using namespace std;

void threadproc(string name)
{
    for(int i = 0 ; i < 10 ; i++)
    {
        cout << "NAME:" << name << endl;
        usleep(1000000);
    }
}

void test_future()
{
    string name="freehuni";
    future<void> call=std::async(launch::async, threadproc, name);

    printLog(__FUNCTION__, __LINE__, "%s", "hello");
    call.get();
}

int main()
{
    test_future();    
    printLog(__FUNCTION__, __LINE__, "%s","frehuni");
    
    return 0;
}
