#pragma once
#include <Arduino.h>
#include "..\..\Conversions\Conversions.h"
#include "..\..\Sum_Operators\Sum_Operators.h"
//====================================================================
// File:        TimeOnly.h
// Description: Declaration of TimeOnly class and its literal opeartors
//====================================================================

namespace Date_Time {
	using namespace Rel_Ops;
	using namespace Sum_Ops;

	class TimeOnly {
	public:
		// Constructors & Assignment
		constexpr TimeOnly() = default;
		constexpr TimeOnly(const TimeOnly & rhs) : _mins(rhs._mins) {} // required in VS! Default is not constexpr
		constexpr TimeOnly(int hrs, int mins) : TimeOnly{ hrs * 6 + mins/10 } {}
		constexpr explicit TimeOnly(int mins10) : _mins{static_cast<uint8_t>(mins10)} {};
		TimeOnly & operator=(int mins10) { _mins = mins10; return *this; }
		
			// Queries
		constexpr int mins10() const { return _mins % 6; } // 0-5
		constexpr int hrs() const { return _mins / 6; }; // 0-41
		int displayHrs() const; // 1-12
		bool isPM() const { return hrs() >= 12; }

		constexpr uint8_t asInt() const { return _mins; }
		constexpr bool operator==(const TimeOnly & rhs) const { return _mins == rhs._mins; }
		constexpr bool operator<(const TimeOnly & rhs) const { return _mins < rhs._mins; }
		constexpr explicit operator bool() const { return _mins != 0; }
		// obtain other boolean operators via Rel_Ops

		// Modifiers
		/// <summary>
		/// returns any overflow as days
		/// </summary>	
		int addClockTime(int mins10) {return mins10 >= 0 ? addClockTime(TimeOnly{ mins10 }) : subClockTime(TimeOnly{ -mins10 });}
		int addClockTime(const TimeOnly & rhs);
		int subClockTime(const TimeOnly & rhs);
		void setMins10(int mins10) {_mins = hrs() * 6 + mins10;}
		void setHrs(int hrs) { _mins = hrs * 6 + mins10(); }
		TimeOnly & operator+=(int mins) { _mins += mins; return *this; }
		TimeOnly & operator+=(const TimeOnly & rhs) { _mins += rhs._mins; return *this; }
		TimeOnly & operator-=(const TimeOnly & rhs) { _mins -= rhs._mins; return *this; }
		// Obtain other sum operators from templates
	private:
		uint8_t _mins = 0;
	};

	namespace Literals {
		inline TimeOnly operator"" _mins(unsigned long long m) {
			return TimeOnly{ 0,static_cast<int>(m) };
		}

		inline TimeOnly operator"" _hrs(unsigned long long h) {
			return TimeOnly{ static_cast<int>(h),0 };
		}
	}
	using namespace Literals;
}