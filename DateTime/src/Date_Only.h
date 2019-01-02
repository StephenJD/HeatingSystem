//====================================================================
// File:        Date_Only.h
// Description: Declaration of DateOnly class
//====================================================================
#pragma once

#include "Time_Only.h"
#include "..\..\Bitfield\Bitfield.h"

namespace Date_Time {
	using namespace BitFields;
	using namespace Rel_Ops;
	using namespace Sum_Ops;

	constexpr char monthNames[][4] = { "","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
	constexpr char dayNames[][4] = { "Mon","Tue","Wed","Thu","Fri","Sat","Sun" };
	constexpr int16_t daysToMonth[] = { 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
	enum { MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY };
	inline constexpr uint16_t getIni(int day, int month, int year) {
		// Required because you are only allowd to initialise one union member in a constructor
		// Constexpr constructors must have empty bodies.
		return UBitField<uint16_t, 0, 5>{uint16_t(day), true}.asBase() |
			UBitField<uint16_t, 5, 4>{uint16_t(month), true}.asBase() |
			UBitField<uint16_t, 9, 7>{uint16_t(year), true}.asBase();
	}

	class DateOnly {
	public:
		// Constructors
		constexpr DateOnly() : _dateInt(0) {}
		constexpr DateOnly(const DateOnly & rhs) : _dateInt(rhs._dateInt) {} // required in VS! Default is not constexpr
		constexpr DateOnly(int day, int month, int year) : _dateInt(getIni(day, month, year)) {}
		explicit constexpr DateOnly(uint32_t intRepr) : _dateInt(static_cast<uint16_t>(intRepr)) {}

		// Queries
		// Note: Only the data members expressly initialised during a constexpr construction 
		// can be queried in a constexpr function.
		// Since a union only allows one member to be initialised,
		// only that member can be queried as a constexpr
		/*constexpr*/ int day() const { return _day; }
		/*constexpr*/ int month() const { return _month; }
		/*constexpr*/ int year() const { return _yr; }
		/*constexpr*/ int asInt() const { return _dateInt; }
		/*constexpr*/ int weekDayNo() const;
		/*constexpr*/ const char * getDayStr() const { return dayNames[weekDayNo()]; }
		/*constexpr*/ const char * getMonthStr() const { return monthNames[month()]; }
		/*constexpr*/ int daysInMnth(DateOnly now = DateOnly()) const;
		/*constexpr*/ int dayOfYear() const;

		/*constexpr*/ explicit operator bool() const { return asInt() != 0; }
		/*constexpr*/ bool operator==(const DateOnly & rhs) const { return asInt() == rhs.asInt(); }
		/*constexpr*/ bool operator<(const DateOnly & rhs) const { return asInt() < rhs.asInt(); }
		/*constexpr*/ bool operator>(const DateOnly & rhs) const { return asInt() > rhs.asInt(); }
		// obtain other boolean operators via <utility>

		// Modifiers
		DateOnly & setMonth(int m) { _month = m; return *this; }
		DateOnly & setDay(int day) { _day = day; return *this; }
		DateOnly & setYear(int y) { _yr = y; return *this; }
		/// <summary>
		/// Adds or subtracts days
		/// </summary>	
		DateOnly & addDays(int dd);
		// All sum-ops add days only.
		DateOnly & operator+=(int dd) { return addDays(dd); }
		DateOnly & operator-=(int dd) { return operator+=(-dd); }
		//DateOnly & operator++ () { return (*this) += 1; }
		DateOnly & operator-- () { return (*this) -= 1; }
		DateOnly operator+(int dd) { DateOnly result(*this); return result += dd; }
		DateOnly operator-(int dd) { DateOnly result(*this); return result -= dd; }
		// other Sum operators from Sum_Ops.

	private:
		union {
			uint16_t _dateInt;
			UBitField<uint16_t, 0, 5> _day;
			UBitField<uint16_t, 5, 4> _month;
			UBitField<uint16_t, 9, 7> _yr; // 2000-2127
		};

	};

	inline /*constexpr*/ int DateOnly::weekDayNo() const {
		int yy = year();
		int yearStartDay = (yy + 5 + (yy - 1) / 4) % 7;
		return (dayOfYear() + yearStartDay) % 7;
	}

	inline /*constexpr*/ int DateOnly::dayOfYear() const {
		int yy = year();
		int mm = month();
		int leapDay = (mm > 2) && (yy % 4 == 0) ? 1 : 0;
		return daysToMonth[mm] + day() + leapDay;
	}

	inline /*constexpr*/ int DateOnly::daysInMnth(DateOnly now) const {
		int yy = year();
		int mm = month();
		if (now) { // if now is provided and month is less than now, use following yaer
			yy = now.year();
			if (mm < now.month()) ++yy;
		}
		return daysToMonth[mm + 1] - daysToMonth[mm] + (mm == 2 ? (yy % 4) == 0 : 0);
	}
}