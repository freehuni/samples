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

class Tank : public Unit
{
public:
	virtual void stop()
	{
		printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	}

	virtual void say(const string& message)
	{
		printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	}
	virtual void attack(Unit* target)
	{
		printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	}
	virtual void move(int x, int y)
	{
		printf("[%s:%d]\n", __FUNCTION__, __LINE__);
	}
	virtual int getX() const
	{
		printf("[%s:%d]\n", __FUNCTION__, __LINE__);
		return 0;
	}
	virtual int getY() const
	{
		printf("[%s:%d]\n", __FUNCTION__, __LINE__);
		return 0;
	}
};

class MockUnit : public Unit
{
public:
	virtual ~MockUnit()
	{}

	MOCK_METHOD0(stop, void());
	MOCK_METHOD1(say, void(const string& message));
	MOCK_METHOD1(attack, void(Unit* target));
	MOCK_METHOD2(move, void(int x,int y));
	MOCK_CONST_METHOD0(getX, int());
	MOCK_CONST_METHOD0(getY, int());
};

void foo(Unit* p)
{
	p->stop();
}

TEST(MockUnitTest, Sample1)
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
TEST(MockUnitTest, Sample2)
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
TEST(MockUnitTest, Sample3)
{
	MockUnit mock;

	EXPECT_CALL(mock, move(10, _));
	EXPECT_CALL(mock, attack(_));

	hoo(&mock);
}

using ::testing::InSequence;
TEST(MockUnitTest, Sample4)
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

	int getDouble(int value)
	{
		return value * 2;
	}
};

class MockTime : public Time
{
public:
	MOCK_METHOD0(getTimeString, string());
	MOCK_METHOD1(getDouble, int(int));
};

using ::testing::Return;
TEST(TimeTest, Sample5)
{
	MockTime stub;
	ON_CALL(stub, getTimeString()).WillByDefault(Return("12:511"));
	//EXPECT_CALL(stub, getTimeString()).WillOnce(Return("12:511"));
	//EXPECT_CALL(stub, getDouble(100)).WillOnce(Return(100*2));

	EXPECT_CALL(stub, getTimeString());

	cout << stub.getTimeString() << endl;
	//cout << stub.getDouble(100) << endl;
}
