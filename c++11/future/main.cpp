#include <iostream>
#include <thread>
#include <future>
#include <string>
#include <chrono>
#include <unistd.h>

using namespace std;


string worker(string name)
{
	string in;
	while(true)
	{
		printf("[%ld: %s > ", std::this_thread::get_id(), name.c_str());
		cin >> in;
		if (in.find("q") != string::npos) break;

	}
	return "ByeBye: " + name;
}

int main()
{
	future<string> fut = std::async(std::launch::async, worker, "freehuni");
	//future<string> fut = std::async(std::launch::deferred, worker, "freehuni");

	while(true)
	{
		future_status ret = fut.wait_for(chrono::seconds(1));
		if (ret == future_status::ready)
		{
			cout << "<<< ready" << endl;
			break;
		}

		if (ret == future_status::deferred)
		{
			cout << "<<< deferred" << endl;
			break;
		}
	}

	printf("[%ld] (+)\n ", std::this_thread::get_id());
	cout << fut.get() << endl;
	printf("[%ld] (-)\n ", std::this_thread::get_id());

	return 0;
}
