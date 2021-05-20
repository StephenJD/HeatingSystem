#pragma once
#include <Arduino.h>
	/// <summary>
	/// Construct with signed microsecs period, up to 2147s (35 minutes).
	/// Returns true when period has expired.
	/// Provides time used and time left.
	/// restart() starts a new period from now.
	/// repeat() adds another period, so there is no timing drift.
	/// newLap() continues the original period, but resets timeUsed() from now.
	/// Roll-over safe.
	/// </summary>
	class Timer_uS {
	public:
		Timer_uS(int32_t period_uS) : _period(period_uS), _startTime(micros()) {} // negative period timesout immediatly
		// Queries
		operator bool() const { return int32_t(micros() - _startTime) >= _period; }
		int32_t timeLeft() const { return _period - timeUsed(); }
		uint32_t timeUsed() const { return micros() - _startTime; }
		int32_t period() const { return _period; }
		// Modifiers
		void set_uS(int32_t period_uS) { _period = period_uS; }
		void set_startTime_uS(int32_t start_uS) { _startTime = start_uS; }
		void restart() { _startTime = micros(); }
		void repeat() { _startTime += _period; }
		void newLap() { _period = timeLeft(); restart(); }
	private:
		int32_t _period;
		uint32_t _startTime;
	};

	/// <summary>
	/// Construct with signed millisecs period, up to 2147s (35 minutes).
	/// Returns true when period has expired.
	/// Provides time used and time left.
	/// restart() starts a new period from now.
	/// repeat() adds another period, so there is no timing drift.
	/// newLap() continues the original period, but resets timeUsed() from now.
	/// Roll-over safe.
	/// </summary>
	class Timer_mS {
	public:
		Timer_mS(int32_t period_mS) : _timer_uS(period_mS * 1000) {} // negative period timesout immediatly
		// Queries
		operator bool() const { return _timer_uS; }
		int32_t timeLeft() const { return _timer_uS.timeLeft() / 1000; }
		uint32_t timeUsed() const { return _timer_uS.timeUsed() / 1000; }
		int32_t period() const { return _timer_uS.period() / 1000; }
		// Modifiers
		void set_mS(int32_t period_mS) { _timer_uS.set_uS(period_mS * 1000); }
		void set_startTime_mS(int32_t start_mS) { _timer_uS.set_startTime_uS(start_mS * 1000); }
		void restart() { _timer_uS.restart(); }
		void repeat() { _timer_uS.repeat(); }
		void newLap() { _timer_uS.newLap(); }
	private:
		Timer_uS _timer_uS;
	};


	int secondsSinceLastCheck(uint32_t & lastCheck_mS);
	inline uint32_t millisSince(uint32_t lastCheck_mS) { return millis() - lastCheck_mS; } // Since unsigned ints are used, rollover just works.