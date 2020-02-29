#include <iostream> // std::cout
#include <stdio.h>
#include <future>         // std::async, std::future
#include <chrono>         // std::chrono::milliseconds
#include <tuple>
#include <bitset>
#include <unistd.h>
#include <functional>

using namespace std;

void test_tuple_tie()
{
	printf("[%s]==============\n", __FUNCTION__);

	std::tuple<int, string, string> man1(0, "mhkang2", "010-0000-0000");
	std::tuple<int, string, string> man2;
	int id;
	string name;
	string phone;


	man2 = std::make_tuple(1,"freehuni", "064-123-12345");
	printf("[tuple] id:%d name:%s phone:%s\n", std::get<0>(man1), std::get<1>(man1).c_str(), std::get<2>(man1).c_str());

	std::tie(id, name, phone) = man2;
	printf("[tie] id:%d name:%s phone:%s\n", id, name.c_str(), phone.c_str());

}

int print_char(char ch)
{	
	usleep(1000000);
	printf("print_char exit\n");
	return ch;
}

int print_name(string name)
{
    int i;

    for (i = 0 ; i < 10 ; i++)
    {
        cout << name << endl;
        usleep(100000);
    }

    return name.size();
}

void test_future_async()
{
    future<int> async_func = async(launch::async, print_name, "async test");
    future_status status;
    chrono::milliseconds span(35);

    printf("[%s]==============\n", __FUNCTION__);

    do
    {
        status = async_func.wait_for(span);
        printf("wait(status:%d)...\n", status);
    } while(status != future_status::ready);

    printf("result of async_func : %d\n", async_func.get());
}

void do_work(string name)
{
    printf("name:%s\n", name.c_str());
}

void test_future_sync()
{
    auto sync_func = async(launch::deferred, print_name, "sync test");
    auto func1 = async([](string name){do_work(name);}, "hello");
    int ret;

    ret = sync_func.get();    
    printf("result of sync_func: %d\n", ret);

    func1.get();
}

void worker(std::promise<string>* p)
{
    usleep(2000000);
    p->set_value("some data...");
}

void test_promise()
{
    std::promise<string> p;
    std::future<string> data = p.get_future();
    std::thread task(worker, &p);

    //data.wait();
    std::future_status status = data.wait_for(std::chrono::seconds(1));
    switch(status)
    {
    case std::future_status::deferred:
        printf("deferred\n");
        break;
    case std::future_status::ready:
        printf("result:%s\n", data.get().c_str());
        break;
    case std::future_status::timeout:
        printf("timeout\n");
        break;
    }



    task.join();
    printf("end...\n");
}

void test_smart_pointer()
{
	printf("[%s]==============\n", __FUNCTION__);

	shared_ptr<string> aa(new string);
	//shared_ptr<string> bb;
	auto bb = aa; // equal to "shared_ptr<string> bb = aa;

	*aa = "mhkang";
	bb = aa;

	printf("aa:%s\n", aa->c_str());
	*bb = "freehuni";

	printf("aa:%s\n", aa->c_str());
	printf("bb:%s\n", bb->c_str());
}

void test_bitset()
{
	printf("[%s]==============\n", __FUNCTION__);

	std::bitset<8> flag;

	flag.set(0);
	flag.set(1);
	flag.set(2);
	cout<< flag << endl;
}

void test_chrono()
{
	printf("[%s]==============\n", __FUNCTION__);
	auto start = chrono::system_clock::now();
	usleep(12340);
	chrono::system_clock::time_point end = chrono::system_clock::now();

	std::chrono::microseconds diff3;
	chrono::duration<int64_t, std::micro> diff;
	chrono::duration<int64_t, std::milli> msecdiff;
	chrono::duration<double> diff2;
	std::chrono::milliseconds ms;

//	diff = end - start;
	diff2 = end - start;
//	diff3 = end - start;

	cout << "microdiff:" << diff.count() << endl;
	cout << "diff2:" << diff2.count() << endl;

	ms =std::chrono::duration_cast<std::chrono::milliseconds>(diff2);
	cout << "msecdiff:" << ms.count() << endl;

	chrono::milliseconds  msec(1);
	chrono::nanoseconds  nanosec;

	nanosec = msec;

	printf("msec:%lld\n", msec.count());
	printf("nanosec:%lld\n", nanosec.count());
}

void threadproc(string name, std::mutex* mux)
{
	int i;
    printf("\n[%s] %s (+)\n", __FUNCTION__, name.c_str());

	mux->lock();
	for (i = 0 ; i < 10 ; i++)
	{
        printf("%s\n", name.c_str());
		usleep(10);
	}
	mux->unlock();

    printf("\n[%s] %s (-)\n", __FUNCTION__, name.c_str());
}

void test_thread()
{
	printf("[%s]==============\n", __FUNCTION__);
	std::mutex mux;
    std::thread proc1(threadproc, "parameter1 of thread", &mux);
    std::thread proc2(threadproc, "parameter2 of thread", &mux);

	proc1.join();
	proc2.join();
}


// Closure example
function<int(int)> startAt(int x)
{
   function<int(int)> f = [x](int y)->int{return (x + y);};

   return f;
}

void test_closure()
{
	auto closure1 = startAt(100);
	auto closure2 = startAt(200);

	printf("value=%d\n", closure1(10));
	printf("value=%d\n", closure2(10));
}

int main ()
{    
    //test_bitset();
    //test_chrono();
    //test_closure();
    //test_future();
    //test_future_async();
    test_future_sync();
    test_promise();;
    //test_smart_pointer();
    //test_thread();
    //test_tuple_tie();

    return 0;
}
