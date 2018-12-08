//====================================================================
// File:        Curr_Time.h
// Description: Declaration of Curr_Time class
//====================================================================
#pragma once
// most specialised first
#include "Time_Date.h"
#include "Date_Only.h"
#include "Time_Only.h"
#include <stdexcept>
#include <shared_mutex>

namespace TimesDates {

	int adjustForDST(const TimeDate & now);

	union ClockAsBCD;
	extern std::shared_timed_mutex clock_mutex;

/// <summary>
/// Maintains clock-time. now(), date() and time() return the most recently acquired time.
///	To refresh the time from the source, call refreshNow()
///	which returns the most up-to-date time available.
/// DST is applied automatically when appropriate.
/// </summary>

	class CurrTime {
	public:
		CurrTime(const volatile ClockAsBCD & clock) : _clock(&clock) {
			if (_instantiated) throw std::length_error("Only one instance allowed");
			_instantiated = true;
			refreshNow();
		}

		// Queries
		std::string print() const { return now().print(); }
		TimeDate now() const;
		DateOnly date() const { return now().date(); }
		TimeMins time() const { return now().time(); }
		operator TimeDate() const { return now(); }

		TimeDate refreshNow();
		void checkDST();

		~CurrTime() { _instantiated = false; }
	private:
		TimeDate _now;
		const volatile ClockAsBCD * _clock;
		static bool _instantiated;
	};


	union ClockAsBCD {
		UBitField<uint64_t, 0, 8> _min;
		UBitField<uint64_t, 8, 8> _hr;
		UBitField<uint64_t, 16, 8> _day;
		UBitField<uint64_t, 24, 8> _month;
		UBitField<uint64_t, 32, 8> _year;
	};
}
