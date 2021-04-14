#pragma once
#include <Arduino.h>
#include <Date_Time.h>
#include <Conversions.h>
	/// <summary>
	/// A Monostate class representing current time/date.
	/// NOTE: to get minute units and seconds, specific functions must be called.
	/// Date/Time obtained by explicit outward conversion to a Date_Time object.
	/// Derivations update time from an I2C RTC chip every 10 minutes.
	/// Automatic DST may be set from 0-2 hours.
	/// </summary>
	class Clock { 
	public:
		//Clock(const char* date, const char* time);
		Clock() { loadTime(); }
		Clock(bool delayLoad) {}
		// Queries
		uint8_t minUnits() const { return _mins1; }
		uint8_t seconds() const { return _secs; }
		uint8_t autoDSThours() const { return _autoDST; }
		uint8_t mins10() const { return _now.mins10(); }
		uint8_t hrs() const { return _now.hrs(); }
		uint8_t day() const { return _now.day(); }
		uint8_t month() const { return _now.month(); }
		uint8_t year() const { return _now.year(); }
		virtual bool ok() const { return true; }
		bool isNewSecond(uint8_t& oldSecond);
		bool isNewMinute(uint8_t& oldMin);
		bool isNew10Min(uint8_t& oldMin);
		// Conceptually, these are queries, although reading the time triggers an update-check which might modify the time
		explicit operator Date_Time::DateTime() const { return _dateTime(); }
		
		/// <summary>
		/// Returns time in 10's of minutes - no units.
		/// </summary>
		const Date_Time::DateTime now() const { return  _dateTime(); }
		const Date_Time::DateOnly date() const { return _dateTime(); }
		const Date_Time::TimeOnly time() const { return _dateTime(); }
		const uint32_t asInt() const { return _dateTime().asInt(); }

		// Modifiers
		void refresh() const { _dateTime(); }

		void setTime(Date_Time::DateOnly date, Date_Time::TimeOnly time, int min);
		void setTime(Date_Time::TimeOnly time) { _now.time() = time; saveTime();}
		void setDate(Date_Time::DateOnly date) { _now.date() = date; saveTime();}
		void setDateTime(Date_Time::DateTime dateTime) { _now = dateTime; _mins1 = 0; _secs = 0; saveTime(); }
		void setHrMin(int hr, int min);
		void setMinUnits(uint8_t minUnits) { _mins1 = minUnits; } // Must NOT saveTime() else recursive on Clock_I2C
		void setSeconds(uint8_t secs) { _secs = secs; } // Must NOT saveTime() else recursive on Clock_I2C
		void setAutoDSThours(int set) { _autoDST = set; saveTime(); }
		virtual auto saveTime()->uint8_t { return 0; }
		virtual auto loadTime()->uint8_t;

#ifdef ZPSIM
		void testAdd1Min();
#endif

	protected:
		virtual void _update() {} // called by _dateTime() every 10 minutes
		bool dstHasBeenSet() const { return _dstHasBeenSet == 1; }
		void dstHasBeenSet(bool set) const { _dstHasBeenSet = set; }
		auto _timeFromCompiler(int & minUnits, int & seconds) -> Date_Time::DateTime;

		static uint8_t _secs;
		static uint8_t _mins1; // minute units
		static uint8_t _autoDST; // hours to add/remove
		static uint8_t _dstHasBeenSet;
		static Date_Time::DateTime _now;
		static uint32_t _lastCheck_mS;

	private:
		Date_Time::DateTime _dateTime() const;
		void _adjustForDST();
	};

	inline arduino_logger::Logger & operator<<(arduino_logger::Logger & logger, const Clock & clk) {
		clk.refresh();
		GP_LIB::CStr_20 timeStr = GP_LIB::intToString(clk.day(), 2);
		timeStr += F_SLASH;
		timeStr += GP_LIB::intToString(clk.month(), 2);
		timeStr += F_SLASH;
		timeStr += GP_LIB::intToString(clk.year(), 2);
		timeStr += F_SPACE;
		timeStr += GP_LIB::intToString(clk.hrs(), 2);
		timeStr += F_COLON;
		timeStr += GP_LIB::intToString(clk.mins10() * 10 + clk.minUnits(),2);
		timeStr += F_COLON;
		timeStr += GP_LIB::intToString(clk.seconds(), 2);
		return logger << timeStr;
	}

	Clock& clock_();

	inline Date_Time::DateTime Clock::_timeFromCompiler(int & minUnits, int & seconds) { // inlined to ensure latest compile time used.
		// sample input: date = "Dec 26 2009", time = "12:34:56"
		auto year = GP_LIB::c2CharsToInt(&__DATE__[9]);
		// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
		int mnth;
		switch (__DATE__[0]) {
		case 'J': mnth = __DATE__[1] == 'a' ? 1 : mnth = __DATE__[2] == 'n' ? 6 : 7; break;
		case 'F': mnth = 2; break;
		case 'A': mnth = __DATE__[2] == 'r' ? 4 : 8; break;
		case 'M': mnth = __DATE__[2] == 'r' ? 3 : 5; break;
		case 'S': mnth = 9; break;
		case 'O': mnth = 10; break;
		case 'N': mnth = 11; break;
		case 'D': mnth = 12; break;
		default: mnth = 0;
		}
		auto day = GP_LIB::c2CharsToInt(&__DATE__[4]);
		auto hrs = GP_LIB::c2CharsToInt(__TIME__);
		auto mins10 = GP_LIB::c2CharsToInt(&__TIME__[3]);
		minUnits = mins10 % 10;
		seconds = GP_LIB::c2CharsToInt(&__TIME__[6]);
		//Serial.print(F("Compiler Time: ")); Serial.print(__DATE__); Serial.print(F(" ")); Serial.println(__TIME__);
		return { { day,mnth,year },{ hrs,mins10 } };
	}




