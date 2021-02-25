#include <string>
#include <stdio.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std;

class Unit
{
public:
	virtual ~Unit()
	{}

	virtual void stop() = 0;
	virtual void say(const string& message) = 0;
	virtual void attack(Unit* target) =0;
	virtual void move(int x, int y) = 0;
	virtual int getX() const = 0;
	virtual int getY() const = 0;
};

class MockUnit : public Unit
{
public:
	virtual ~MockUnit()
	{}

	MOCK_METHOD(void, stop, (),(override));
	MOCK_METHOD(void, say, (const string& message),(override));
	MOCK_METHOD(void, attack, (Unit* target),(override));
	MOCK_METHOD(void, move, (int x,int y),(override));
	MOCK_METHOD(int, getX, (),(const, override));
	MOCK_METHOD(int, getY, (),(const, override));
};

void foo(Unit* p)
{
	p->stop();
}

TEST(MockUnitTest, CheckCallOnce)
{
	MockUnit mock;

	EXPECT_CALL(mock, stop()).Times(1);

	foo(&mock);
}

void goo(Unit* p)
{
	p->stop();
	p->stop();
}

using ::testing::AnyNumber;
TEST(MockUnitTest, CheckCallCount)
{
	MockUnit mock;

	EXPECT_CALL(mock, stop()).Times(AnyNumber());

	goo(&mock);
}

void hoo(Unit* p)
{
	p->attack(nullptr);
	p->move(10, 20);
}

using ::testing::_;
TEST(MockUnitTest, CheckParameters)
{
	MockUnit mock;

	EXPECT_CALL(mock, move(10, _));
	EXPECT_CALL(mock, attack(_));

	hoo(&mock);
}

using ::testing::InSequence;
TEST(MockUnitTest, CheckSequence)
{
	InSequence seq;
	MockUnit mock;

	EXPECT_CALL(mock, attack(nullptr));
	EXPECT_CALL(mock, move(10, 20));

	hoo(&mock);
}

class Time {
public:
	string getTimeString()
	{
		return "12:50";
	}

	int getValue()
	{
		return 0;
	}

	int getDouble(int value)
	{
		return value * 2;
	}
};

class MockTime : public Time
{
public:
	MOCK_METHOD(string, getTimeString, ());
	MOCK_METHOD(int, getDouble, (int));
	MOCK_METHOD(int, getValue, ());
};

using ::testing::Return;
TEST(TimeTest, Sample5)
{
	MockTime stub;
	EXPECT_CALL(stub, getValue()).Times(2).WillOnce(Return(10)).WillOnce(Return(1000));

	cout << stub.getValue() << endl;
	cout << stub.getValue() << endl;
}
