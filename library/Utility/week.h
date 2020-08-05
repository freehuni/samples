#ifndef WEEK_H
#define WEEK_H

#include <map>
#include <string>

namespace Freehuni
{
class Week
{
public:
	typedef enum
		{
			eWeekSun,
			eWeekMon,
			eWeekTue,
			eWeekWed,
			eWeekThr,
			eWeekFri,
			eWeekSat,
			eWeekUnknown,
		} eWEEK;

	std::map<eWEEK, std::string> mWeekString=
	{
		{eWeekSun,"sun"},
		{eWeekMon,"mon"},
		{eWeekTue,"tue"},
		{eWeekWed,"wed"},
		{eWeekThr,"thr"},
		{eWeekFri,"fri"},
		{eWeekSat,"sat"}
	};

public:
	Week();
	eWEEK GetWeek();
	const char* GetWeek(eWEEK week);
	void SetWeek(eWEEK week);
	const std::string operator()()
	{
		return mWeekString[mWeekDay];
	}

private:
	eWEEK mWeekDay;
};
}

#endif // WEEK_H
