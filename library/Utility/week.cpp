#include "week.h"

namespace  Freehuni
{
	Week::Week()
	{
	}

	Week::eWEEK Week::GetWeek()
	{
		return mWeekDay;
	}

	const char* Week::GetWeek(eWEEK week)
	{
		return mWeekString[week].c_str();
	}

	void Week::SetWeek(eWEEK week)
	{
		mWeekDay = week;
	}
}
