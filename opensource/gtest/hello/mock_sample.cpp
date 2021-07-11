#include <string>
#include <stdio.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std;


class Process
{
public:
	virtual~ Process()
	{}
	virtual bool init() = 0;
	virtual void work(int speed) = 0;
	virtual void deinit() = 0;
	virtual int getState() = 0;
};

class MockProcess : public Process
{
public:
	virtual~ MockProcess()
	{}

	MOCK_METHOD(bool, init, (), (override));
	MOCK_METHOD(void, work,(int speed), (override));
	MOCK_METHOD(void, deinit,(), (override));
	MOCK_METHOD(int , getState,(), (override));
};


void test1(Process* process)
{
	process->init();
}

TEST(MockTest, InitTest)
{
	MockProcess mock;

	EXPECT_CALL(mock, init()).Times(1);

	test1(&mock);
}


void test2(Process* process)
{
	process->init();
	process->work(0);
	process->work(1);
	process->deinit();
}

TEST(MockTest, SequenceTest)
{
	MockProcess mock;
	::testing::InSequence seq;

	EXPECT_CALL(mock, init()).Times(1);
	EXPECT_CALL(mock, work(::testing::_)).Times(2);
	// EXPECT_CALL(mock, work(0)).Times(1);
	// EXPECT_CALL(mock, work(1)).Times(1);
	EXPECT_CALL(mock, deinit()).Times(1);

	test2(&mock);
}

void test3(Process* process)
{
	if (process->getState() == 1)
	{
		process->work(0);
	}
}

using ::testing::Return;
TEST(MockTest, ReturnTest)
{
	MockProcess mock;

	ON_CALL(mock, getState()).WillByDefault(Return(1));
	EXPECT_CALL(mock, getState()).Times(1);
	EXPECT_CALL(mock, work(0)).Times(1);

	test3(&mock);
}


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

class FakeUnit : public Unit
{
public:
	FakeUnit()
	{}

	virtual void stop()
	{

	}
	virtual void say(const string& message)
	{

	}

	virtual void attack(Unit* target)
	{

	}

	virtual void move(int x, int y)
	{

	}

	virtual int getX() const
	{
		return 1000;
	}

	virtual int getY() const
	{
		return 0;
	}

};

using ::testing::Invoke;
TEST(MockUnitTest, InvokeTest)
{
	MockUnit mock;
	FakeUnit fake;

	ON_CALL(mock, getX()).WillByDefault(Invoke(&fake, & FakeUnit::getX));
	EXPECT_CALL(mock, getX()).Times(1);

	cout<< "getX:" << mock.getX() << endl;
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
