#include "week.h"

namespace  Freehuni
{
	Week::Week()
	{
	}

	Week::eWEEK Week::GetWeek()
	{
	#ifdef UNIT_TEST
		return mWeekDay;
	#else
		time_t t=time(0);
		struct tm* now = localtime(&t);
		return (eWEEK)now->tm_wday;
	#endif
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
