#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

using namespace std;

class Human
{
public:
	virtual ~Unit(){}

	int say(string message) = 0;
	string getMessage() = 0;
	int getSex() = 0;
};

class Boy : public Humax
{
public:
	Boy():mSex(0)
	{}
	virtual~ Boy()
	{}

	int say(string message) = 0;
	string getMessage() = 0;
	int getSex() = 0;


private:
	int mSex = 0;
};


class MockHumax : public Humax
{
public:
	virtual ~MockHumax()
	{}

	MOCK_METHOD0(join, bool());
	MOCK_METHOD0(byebye, bool());	MOCK_METHOD1(say, int(string));
	MOCK_METHOD0(getMessage, string());
};
