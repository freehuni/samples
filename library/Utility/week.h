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
			eSun,
			eMon,
			eTue,
			eWed,
			eThr,
			eFri,
			eSat
		} eWEEK;

	std::map<eWEEK, std::string> mWeekString=
	{
		{eSun,"sun"},
		{eMon,"mon"},
		{eTue,"tue"},
		{eWed,"wed"},
		{eThr,"thr"},
		{eFri,"fri"},
		{eSat,"sat"}
	};

public:
	Week();
	eWEEK GetWeek();
	const char* GetWeek(eWEEK week);
	void SetWeek(eWEEK week);

private:
	eWEEK mWeekDay;
};
}

#endif // WEEK_H
