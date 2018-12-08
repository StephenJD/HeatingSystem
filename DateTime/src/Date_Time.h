//====================================================================
// File:        Time_Date.h
// Description: Declaration of DateTime class
//====================================================================

#pragma once
#include "Date_Only.h"
#include "Time_Only.h"
#include <Sum_Operators.h>
#include <Arduino.h>

namespace Date_Time {
	using namespace Rel_Ops;
	using namespace Sum_Ops;

	enum Field {yy,mm,dd,hh,m10};
	struct incDTfld {
		Field field;
		int offset;
	};

	class DateTime : public TimeOnly, public DateOnly
	{
	public:
		// Constructors
		constexpr DateTime() = default;
		explicit constexpr DateTime(const DateOnly & rhs) : TimeOnly(0),DateOnly(rhs) {}
		explicit constexpr DateTime(const TimeOnly & rhs) : TimeOnly(rhs),DateOnly(0) {}
		explicit constexpr DateTime(uint32_t dateTime) : TimeOnly(dateTime % 256), DateOnly(dateTime >> 8) {}
		constexpr DateTime(DateOnly date, TimeOnly time) : TimeOnly{ time }, DateOnly{ date } {}

		// Queries
		explicit constexpr operator bool() const { return TimeOnly::operator bool() | DateOnly::operator bool(); }
		constexpr uint32_t asInt() const { return (uint32_t(DateOnly::asInt()) << 8) | TimeOnly::asInt(); };
		constexpr DateOnly date() const { return *this; }
		constexpr TimeOnly time() const { return *this;}

		constexpr bool operator==(const DateTime & rhs) const { return asInt() == rhs.asInt(); }
		constexpr bool operator<(const DateTime & rhs) const { return asInt() < rhs.asInt(); }
		constexpr bool operator>(const DateTime & rhs) const { return asInt() > rhs.asInt(); }
		//DateTime operator~ () const {return ~this->asInt();}
		// obtain other boolean operators via Rel_Ops

		// Modifiers
		DateOnly & date() { return *this; }
		TimeOnly & time() { return *this; }
		DateTime & addOffset_NoRollover(incDTfld offset, DateTime now);
		DateTime & addOffset(incDTfld offset);
		// sum operators are only provided for DateOnly and TimeOnly
	};

	const DateTime JUDGEMEMT_DAY({ 31, 15, 127 }, {});
}