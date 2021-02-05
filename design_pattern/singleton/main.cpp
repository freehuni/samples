#include <iostream>
#include <mutex>
#include <stdio.h>

using namespace std;

class Singleton
{
public:
	static Singleton* getInstance()
	{

		std::call_once(mOnceFlag, [](){ mInstance = new Singleton();} );

		return mInstance;
	}

	int getValue()
	{
		return mValue;
	}

	void setValue(int value)
	{
		mValue = value;
	}

private:
	Singleton() : mValue(0)
	{
	}

	static std::once_flag mOnceFlag;
	static Singleton* mInstance;
	int mValue;
};

std::once_flag Singleton::mOnceFlag;
Singleton* Singleton::mInstance = nullptr;


int main()
{
	Singleton::getInstance()->setValue(1004);
	printf("getValue:%d\n", Singleton::getInstance()->getValue());

	return 0;
}
