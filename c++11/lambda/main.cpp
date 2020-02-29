#include <iostream>
#include <functional>
#include <vector>

using namespace std;

typedef function<void(void)> FUNC_VOID;
typedef vector<FUNC_VOID> LIST_FUNC_VOID;
typedef LIST_FUNC_VOID::iterator LIST_FUNC_VOID_IT;
void call_void_functions(LIST_FUNC_VOID list);

struct FooFunctor
{
	void operator()(int i)
	{
		cout<<i;
	}
};


int getvalue(int x)
{
	return x * 2;
}

function<int(int)>  startAt(int x)
{
	auto incrementBy = [x](int y)->int {return (x+y);};

	return incrementBy;
}

void test_closure()
{
	auto closure1 = startAt(100);
	auto closure2 = startAt(200);
	function<int(int)> fGet= getvalue;

	printf("closure1:%d\n", closure1(10));
	printf("closure2:%d\n", closure2(10));

	// Currying
	printf("startAt(100)(10):%d\n", startAt(100)(10));
}


int main(int argc, char *argv[])
{
	LIST_FUNC_VOID functions;
 //   FooFunctor f()(100);

	functions.push_back([]()->void{
		printf("call first!\n");
						});

	functions.push_back([]()->void{
		printf("call second!\n");
						});

	functions.push_back([]()->void{
		printf("call third!\n");
						});

	call_void_functions(functions);

	test_closure();

	return 0;
}


void call_void_functions(LIST_FUNC_VOID list)
{
	LIST_FUNC_VOID_IT it;
	FUNC_VOID func;

	for (it = list.begin() ; it != list.end() ; it++)
	{
		(*it)();

	}
}

