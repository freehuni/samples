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

vector<string> getToken(string text, const string delimiter)
{
    vector<string> result;
    size_t pos;

    pos = text.find_first_of(delimiter);
    while(pos != string::npos)
    {
        result.push_back(text.substr(0, pos));
        text.erase(0, pos + 1);
        pos = text.find_first_of(delimiter);
    }

    if (!text.empty())
    {
        result.push_back(text);
    }

    return result;
}

int main()
{
    string mac="1:2:3:4:5:6-AAA";
    vector<string> tokens;

    tokens = getToken(mac, ':');
    for(string item: tokens)
    {
        printf("item1:%s\n", item.c_str());
    }

	tokens = getToken(mac, ":-");
    for(string item: tokens)
    {
        printf("item2:%s\n", item.c_str());
    }

    return 0;
}
