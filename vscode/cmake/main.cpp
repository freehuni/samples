#include <stdio.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

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
    string mac="aa:bbb:dddd:qq-1000:";
    vector<string> result;
    
    result = getTokens(mac, ":-");
    for(string item: result)
    {
        cout << "itme:" << item << endl;
    }
    

    return 0;
}

