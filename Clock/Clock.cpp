#include "Clock.h"
#include <Timer_mS_uS.h>

///////////////////////////////////////////////////
//         Public Global Functions        //
///////////////////////////////////////////////////

using namespace GP_LIB;
using namespace Date_Time;

	uint8_t Clock::_mins1; // minute units
	uint8_t Clock::_secs;
	uint8_t Clock::_autoDST;
	uint8_t Clock::_dstHasBeenSet;
	Date_Time::DateTime Clock::_now;
	uint32_t Clock::_lastCheck_mS = millis() /*- uint32_t(60ul * 1000ul * 11ul)*/;

	///////////////////////////////////////////////////
	//             Clock Member Functions            //
	///////////////////////////////////////////////////

	//Date_Time::DateTime Clock::_timeFromCompiler(int & minUnits, int & seconds) { // inlined to ensure latest compile time used.
	//// sample input: date = "Dec 26 2009", time = "12:34:56"
	//	auto year = GP_LIB::c2CharsToInt(&__DATE__[9]);
	//	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
	//	int mnth;
	//	switch (__DATE__[0]) {
	//	case 'J': mnth = __DATE__[1] == 'a' ? 1 : mnth = __DATE__[2] == 'n' ? 6 : 7; break;
	//	case 'F': mnth = 2; break;
	//	case 'A': mnth = __DATE__[2] == 'r' ? 4 : 8; break;
	//	case 'M': mnth = __DATE__[2] == 'r' ? 3 : 5; break;
	//	case 'S': mnth = 9; break;
	//	case 'O': mnth = 10; break;
	//	case 'N': mnth = 11; break;
	//	case 'D': mnth = 12; break;
	//	default: mnth = 0;
	//	}
	//	auto day = GP_LIB::c2CharsToInt(&__DATE__[4]);
	//	auto hrs = GP_LIB::c2CharsToInt(__TIME__);
	//	auto mins10 = GP_LIB::c2CharsToInt(&__TIME__[3]);
	//	minUnits = mins10 % 10;
	//	seconds = GP_LIB::c2CharsToInt(&__TIME__[6]);
	//	logger() << F("Compiler Time: ") << __DATE__ << " " << __TIME__ << L_endl;
	//	return { { day,mnth,year },{ hrs,mins10 } };
	//}

	DateTime Clock::_dateTime() const { 
		// called on every request for time/date.
		// Only needs to check anything every 10 minutes
		static uint8_t oldHr = _now.hrs() + 1; // force check first time through
		int newSecs = _secs + secondsSinceLastCheck(_lastCheck_mS);
		_secs = newSecs % 60;
		int newMins = _mins1 + newSecs / 60;
		_mins1 = newMins % 10;
		if (newMins >= 10) {
			Clock & modifierRef = const_cast<Clock &>(*this);
			_now.addOffset({ m10, newMins / 10 });
			modifierRef._update();
			if (_now.hrs() != oldHr) {
				oldHr = _now.hrs();
				modifierRef._adjustForDST();
			}
		}
		//Serial.print(" Refresh: "); Serial.println(_now.hrs());
		return _now;
	}

	void Clock::setHrMin(int hr, int min) {
		_now.time() = TimeOnly(hr, min);
		setMinUnits(min % 10);
		saveTime();
	}

	void Clock::setTime(Date_Time::DateOnly date, Date_Time::TimeOnly time, int min) {
		_now = DateTime{ date, time };
		setMinUnits(min);
		setAutoDSThours(true);
		setSeconds(0);
		saveTime();
	}

	void Clock::_adjustForDST() {
		//Last Sunday in March and October. Go forward at 1 am, back at 2 am.
		if ((autoDSThours()) && (_now.month() == 3 || _now.month() == 10)) { // Mar/Oct
			if (_now.day() == 1 && dstHasBeenSet()) {
				dstHasBeenSet(false); // clear
				saveTime();
			}
			else if (_now.day() > 24) { // last week
				if (_now.weekDayNo() == SUNDAY) {
					if (!dstHasBeenSet() && _now.month() == 3 && _now.hrs() == 1) {
						_now.setHrs(autoDSThours()+1);
						dstHasBeenSet(true); // set
						saveTime();
					}
					else if (!dstHasBeenSet() && _now.month() == 10 && _now.hrs() == 2) {
						_now.setHrs(2-autoDSThours());
						dstHasBeenSet(true); // set
						saveTime();
					}
				}
			}
		}
	}

#ifdef ZPSIM
	void Clock::testAdd1Min() {
		if (minUnits() < 9) setMinUnits(minUnits() + 1);
		else {
			setMinUnits(0);
			_now.addOffset({ m10,1 });
		}
		_update();
	}
#endif