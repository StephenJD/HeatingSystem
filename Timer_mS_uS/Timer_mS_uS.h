#pragma once
#include <Arduino.h>

//#define DEBUG_TIMER
//#ifdef DEBUG_TIMER
#include <Logging.h>
namespace arduino_logger {
	Logger& logger();
}
using namespace arduino_logger;
//#endif

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

	/// <summary>
	/// Construct with signed millisecs period, up to 2147s (35 minutes).
	/// Counts period down to zero.
	/// timeLeft() reports remaining mS, or zeros period if expired.
	/// hasLapped() divides the remaining period by the lap-period and compares its evenness with the supplied evenness
	/// If the evenness has changed, a lap has occurred (assuming checks are made more frequently than the lap period!)
	/// Roll-over safe. Uses micros() because millis() is unreliable.
	/// </summary>
	class LapTimer_mS {
	public:
		LapTimer_mS(uint32_t period_mS) : _endTime_uS(micros() + period_mS * 1000) {}
		LapTimer_mS& operator=(uint32_t end_mS) { _endTime_uS = micros() + end_mS * 1000; return *this; }
		int32_t timeLeft() {
			int32_t diff = _endTime_uS - micros();
			if (diff < 0) {
				_endTime_uS = micros();
#ifdef DEBUG_TIMER
				//logger() << L_tabs << "Lap_ended:" << _endTime_uS << L_endl;
#endif
				//logger() << F("Lap_ended:") << diff << L_endl;
			}
			return diff/1000;
		}
		bool hasLapped(uint32_t lapPeriod_mS, bool lastLapWasEven) {
			auto remaining = timeLeft();
#ifdef DEBUG_TIMER
			//logger() << L_tabs << "End:" << _endTime_uS << L_endl;
			//logger() << L_tabs << "NoOfLaps:" << remaining / lapPeriod_mS << (lastLapWasEven ? "WasEven" : "WasOdd") << L_endl;
#endif
			return (remaining / lapPeriod_mS) % 2 != lastLapWasEven;
		}
	private:
		uint32_t _endTime_uS;
	};

	inline bool hasLapped(uint32_t lapPeriod_mS, bool lastLapWasEven) {
		return (micros() / 1000 / lapPeriod_mS ) % 2 != lastLapWasEven;
	}
	int secondsSinceLastCheck(uint32_t & lastCheck_uS);
	inline uint32_t microsSince(uint32_t lastCheck_uS) { return micros() - lastCheck_uS; } // Since unsigned ints are used, rollover just works.