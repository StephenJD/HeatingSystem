//====================================================================
// File:        Time_Date.h
// Description: Declaration of DateTime class
//====================================================================

#pragma once
#include "Date_Only.h"
#include "Time_Only.h"
#include "..\..\Sum_Operators\Sum_Operators.h"
#include <Arduino.h>
#include <Logging.h>

#ifdef ZPSIM
#include <ostream>
#endif

namespace Date_Time {
	using namespace Rel_Ops;
	using namespace Sum_Ops;

	enum Field {yy,mm,dd,hh,m10};
	struct incDTfld {
		Field field;
		int offset;
	};

	/// <summary>
	/// Represents date and time in 10's of minutes (no units) for the years 2000-2127.
	/// </summary>
	class DateTime : public TimeOnly, public DateOnly
	{
	public:
		// Constructors
		/*constexpr*/ DateTime() = default;
		explicit constexpr DateTime(const DateOnly & rhs) : TimeOnly(0),DateOnly(rhs) {}
		explicit constexpr DateTime(const TimeOnly & rhs) : TimeOnly(rhs),DateOnly(0) {}
		explicit constexpr DateTime(uint32_t dateTime) : TimeOnly(dateTime % 256), DateOnly(dateTime >> 8) {}
		constexpr DateTime(DateOnly date, TimeOnly time) : TimeOnly{ time }, DateOnly{ date } {}

		// Queries
		explicit /*constexpr*/ operator bool() const { return TimeOnly::operator bool() | DateOnly::operator bool(); }
		/*constexpr*/ uint32_t asInt() const { return (uint32_t(DateOnly::asInt()) << 8) | TimeOnly::asInt(); };
		/*constexpr*/ DateOnly date() const { return *this; }
		/*constexpr*/ TimeOnly time() const { return *this;}
	 /*constexpr*/ bool operator==(const DateTime & rhs) const { return asInt() == rhs.asInt(); }
	 /*constexpr*/ bool operator<(const DateTime & rhs) const { return asInt() < rhs.asInt(); }
	 /*constexpr*/ bool operator>(const DateTime & rhs) const { return asInt() > rhs.asInt(); }

		//DateTime operator~ () const {return ~this->asInt();}
		// obtain other boolean operators via Rel_Ops

		// Modifiers
		DateOnly & date() { return *this; }
		TimeOnly & time() { return *this; }
		DateTime & addOffset_NoRollover(incDTfld offset, DateTime now);
		DateTime & addOffset(incDTfld offset);
		// sum operators are only provided for DateOnly and TimeOnly
	};

	//inline /*constexpr*/ bool operator==(const DateTime & lhs, const DateTime & rhs) { return lhs.asInt() == rhs.asInt(); }
	//inline /*constexpr*/ bool operator<(const DateTime & lhs, const DateTime & rhs) { return lhs.asInt() < rhs.asInt(); }
	//inline /*constexpr*/ bool operator>(const DateTime & lhs, const DateTime & rhs) { return lhs.asInt() > rhs.asInt(); }
	inline Logger & operator << (Logger & stream, const DateTime & dt) {
		return stream << dt.day() << "/" << dt.getMonthStr() << "/" << dt.year() << " " << dt.displayHrs() << ":" << dt.mins10() << "0" << (dt.isPM() ? "pm" : "am");
	}

#ifdef ZPSIM
	inline std::ostream & operator << (std::ostream & stream, const DateTime & dt) {
		return stream << std::dec << dt.day() << "/" << dt.getMonthStr() << "/" << dt.year() << " " << dt.displayHrs() << ":" << dt.mins10() << "0" << (dt.isPM() ? "pm" : "am");
	}
#endif

	const DateTime JUDGEMEMT_DAY({ 31, 15, 127 }, {});
}