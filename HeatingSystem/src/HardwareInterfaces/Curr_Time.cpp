#include "stdafx.h" 
#include "Curr_Time.h"
#include <iostream>
using namespace std;

namespace TimesDates {
	// static definitions
	bool CurrTime::_instantiated = false;

	std::shared_timed_mutex clock_mutex;

	TimeDate CurrTime::now() const {
		std::shared_lock<std::shared_timed_mutex> read_lock(clock_mutex);
		return _now;
	}

	/// <summary>
	/// Checks if the argument is the date/time at which DST is adjusted.
	/// If it is, it returns the hour offset required.
	/// </summary>
	/// <param name="now">Current TimeDate</param>
	/// <returns></returns>
	int adjustForDST(const TimeDate & now) {
		int returnVal = 0;
		if (now.month() == 3 || now.month() == 10) { // Mar/Oct
			if (now.day() > 24) { // last week
				if (weekDayNo(now) == 6) {
					if (now.month() == 3 && now.hrs() == 1) {
						returnVal = 1;
					}
					else if (now.month() == 10 && now.hrs() == 2) {
						returnVal = -1;
					}
				}
			}
		}
		return returnVal;
	}


	/// <summary>
	/// Updates CurrTime from the clock and returns the current time.
	/// </summary>
	/// <returns></returns>
	TimeDate CurrTime::refreshNow() {
		_now.set_year(fromBCD(_clock->_year));
		_now.set_month(fromBCD(_clock->_month));
		_now.set_day(fromBCD(_clock->_day));
		_now.time() = { fromBCD(_clock->_hr), fromBCD(_clock->_min) };
		checkDST();
		return _now;
	}

	/// <summary>
	/// Adjusts the CurrTime for DST if required
	/// </summary>
	void CurrTime::checkDST() {
		static bool dstHasBeenSet = false;
		auto addHour = adjustForDST(now());
		if (addHour && !dstHasBeenSet) {
			dstHasBeenSet = true;
			this->_now.time() += {addHour, 0};
		} 
		if (this->date().day() < 24 && dstHasBeenSet) {
			dstHasBeenSet = false; // clear
		}
	}
}