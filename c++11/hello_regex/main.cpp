#include <iostream>
#include <regex>
#include <string>

using namespace std;

void regex_test_extract()
{
    string input_string("Device.LAN.1.DHCPOption.23.Request.Test4.Value");
    regex pattern("\\.[0-9]{1,3}\\.");
    smatch m;

    while (regex_search(input_string, m, pattern))
    {
        for (auto index : m)
        {
            printf("index:%s\n", index.str().substr(1, index.str().size() -2).c_str());
        }

        input_string = m.suffix();
    }
}

void regex_test_extract2()
{
    string input_string("Device.LAN.1.DHCPOption.23.Request.Test4.Value.4.");
    //regex pattern("\\.([0-9]{1,3})\\.$");
    regex pattern("\\.(\\d)\\.$");
    //regex pattern("\\.([0-9]{1,3})\\.");
    smatch m;

    while (regex_search(input_string, m, pattern))
    {
        for (auto index : m)
        {
            printf("index:%s\n", index.str().c_str());
        }

        input_string = m.suffix();
    }
}

void regex_test_replace1()
{
    string input_string("Device.LAN.1.DHCPOption.23.Request.Test4.Value.tmp.345First.dummy");
    regex pattern("\\.[0-9]{1,3}\\.");

    string result = regex_replace(input_string, pattern, string(".{i}."));

    printf("replace:%s\n", result.c_str());
}

void regex_test_replace2()
{
    //string input_string("Device.LAN.1.DHCPOption.1.Request.");
    string input_string("Device.LAN.1.");
    //string input_string("Device.");
    regex pattern("\\.");

    string result = string("/")+regex_replace(input_string, pattern, string("/")) +"*";

    printf("replace:%s\n", result.c_str());
}

int main()
{
    //regex_test_extract();
    string str;
    string result;
    int index = 1;


    //result = string("value:") << index << endl;
    //printf("value:%d\n", string(index).c_str());

    regex_test_extract2();
    //regex_test_replace2();

    return 0;
}

