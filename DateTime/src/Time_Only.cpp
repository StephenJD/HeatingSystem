#include "Time_Only.h"

namespace Date_Time {

	int TimeOnly::addClockTime(const TimeOnly & rhs) {
		int newTime = asInt() + rhs.asInt();
		int overflow = 0;
		while (newTime > 6 * 24) {
			newTime -= 6 * 24;
			++overflow;
		}
		*this = newTime;
		return overflow;
	}

	int TimeOnly::subClockTime(const TimeOnly & rhs) {
		int newTime = asInt() - rhs.asInt();
		int overflow = 0;
		while (newTime < 0) {
			newTime += 6 * 24;
			--overflow;
		}
		*this = newTime;
		return overflow;
	}

	int TimeOnly::displayHrs() const {
		int hh = hrs();
		while (hh > 12) hh -= 12;
		if (hh == 0) hh = 12;
		return hh;
	}
}