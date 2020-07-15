#include <iostream>
#include <fstream>
#include <string>

#include "cpp/hello.pb.h"

using namespace std;
using namespace tutorial;

void toFile()
{
    Person man;
    ofstream file("person.db");
    man.set_name("mhkang");
    man.set_id(100);
    man.set_email("freehuni@hanmail.net");

    ///man.SerializePartialToOstream(&file);
    man.SerializeToOstream(&file);
}

void fromFile()
{
    Person man;
    ifstream file("person.db");

    man.ParseFromIstream(&file);
    cout << "name:" << man.name() << endl;
    cout << "id:" << man.id() << endl;
    cout << "email:" << man.email() << endl;
}

int main()
{
    toFile();
    fromFile(); 

    return 0;
}