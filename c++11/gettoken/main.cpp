#include <string>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

vector<string> getToken(const string& data, const char delimiter)
{
    stringstream ss(data);
    vector<string> result;
    string token;

    while(std::getline(ss, token, delimiter))
    {
        result.push_back(token);
    }

    return result;
}

int main()
{
    string mac="1:2:3:4:5:6";
    vector<string> tokens;

    tokens = getToken(mac, ':');
    for(string item: tokens)
    {
        printf("item:%s\n", item.c_str());
    }

    return 0;
}