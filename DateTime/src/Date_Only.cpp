#include "Date_Only.h"

namespace Date_Time {

	DateOnly & DateOnly::addDays(int dd) {
		int newDay = day() + dd;
		int daysThisMnth = daysInMnth();
		while (newDay > daysThisMnth) {
			newDay -= daysThisMnth;
			++_month;
			if (_month > 12) { _month = 1; ++_yr; }
			daysThisMnth = daysInMnth();
		}
		while (newDay < 1) {
			--_month;
			if (_month == 0) { _month = 12; --_yr; }
			newDay += daysInMnth();
		}
		_day = newDay;
		return *this;
	}

}