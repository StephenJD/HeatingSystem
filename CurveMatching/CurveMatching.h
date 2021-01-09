#pragma once
#include <math.h>
#include <Arduino.h>

namespace GP_LIB {
	/// <summary>
	/// Obtains the exponential curve Limit and Time-constant that matches three values obtained over a period of time. 
	/// The constructor argument sets the minimum valid value rise since the previous value.
	/// Values must be provided at regular time intervals, the time constant is in multiples of this interval.
	/// The values used for the match are the first and last valid values and an approximatly mid-period value.
	/// </summary>
	class GetExpCurveConsts {
	public:
		GetExpCurveConsts(int min_rise) : _min_rise(int16_t(min_rise)) {} //stop();
		
		// Nested Type
		struct CurveConsts {
			int16_t timeConst = 0;
			int16_t limit = 0;
			int16_t range = 0;
			//int16_t start = 0;
			//int16_t period = 0;
			bool resultOK = false;
		};
		
		static double uncompressTC(uint8_t compressed_tc) {
			return exp(compressed_tc / 50.) * 20;
		}

		static uint8_t compressTC(double timeConst) {
			if (timeConst > 3280) timeConst = 3280;
			if (timeConst < 20) timeConst = 20;
			return static_cast<uint8_t>(log(timeConst/20.) * 50. + 0.5);
		}
		
		// ************** Queries **************
		/// <summary>
		/// Call to obtain curve constants.
		/// May be called repeatedly until best result obtained.
		/// Can be called at any time after the final value is submitted.
		/// </summary>
		/// <returns></returns>
		CurveConsts matchCurve(int limitVal);

		struct XY_Values; // only required public for testing purposes
		XY_Values & getXY() { return _xy; } // only required for testing purposes
		
		// ************** Modifiers **************
		/// <summary>
		/// Call to initiate a new curve match
		/// </summary>
		/// <param name="currValue">The curr value.</param>
		void firstValue(int currValue);

		/// <summary>
		/// Call to add new values to the curve match.
		/// Must be called at regular time intervals.
		/// </summary>
		/// <param name="currValue">The curr value.</param>
		bool nextValue(int currValue);
		bool nextValue(double currValue) { return nextValue(static_cast<int>(currValue)); }
		uint16_t calcNewTimeConst(double limit, bool& OK) const;

		struct XY_Values { // only required public for testing purposes
			int16_t firstRiseValue = 0;
			int16_t midRiseValue = 0;
			int16_t midRiseTime = 0;
			int16_t lastRiseValue = 0;
			int16_t lastRiseTime = 0;
		};
	private:
		friend class LimitTemp;

		// ************** Queries **************
		bool needsStarting() const;
		bool hasRisenEnough() const;
		bool periodIsDoublePreviousPeriod();

		// ************** Modifiers **************
		bool startTiming();
		void averageTimeAtThisValue();
		void shuffleRecordsAlong();
		void recordCurrent();
		//uint16_t calcNewLimit(bool& OK) const;

		// ************** Data **************
		static int16_t _currValue;
		XY_Values _xy;
		int16_t _twiceMidRiseValue = 0;
		int16_t _twiceMidRiseTime = 0;
		int16_t _lastRiseStartTime = 0;
		int16_t _timeSinceStart = 0;
		int16_t _min_rise;
	};

	//class LimitTemp { // functor for determining Limit of exponential curve
	//public:
	//	LimitTemp(const GetExpCurveConsts::XY_Values& xy) // double T0, double T1, double T2, int t1, int t2) 
	//		: T0(xy.firstRiseValue),
	//		T1(xy.midRiseValue),
	//		T2(xy.lastRiseValue),
	//		t1(xy.midRiseTime),
	//		t2(xy.lastRiseTime) {}
	//	double operator()(double TL) const {
	//		if (TL < T2) return 1;
	//		return t1 * log((TL - T2) / (TL - T0)) - t2 * log((TL - T1) / (TL - T0));
	//	}
	//private:
	//	double T0, T1, T2; // values at time 0, t1 and t2.
	//	double t1, t2; // times
	//};

	template<typename functor>
	double SecantMethodForEquation(const functor& Fn, double x0, double x1, double error) {
		/*
			* Secant method for solving equation F(x) = 0
			* Input:
			* x0 - the first initial approximation of the solution
			* x1 - the second initial approximation of the solution
			* Return:
			* x - the resulted approximation of the solution
		*/
		enum { _maxiter = 50 };
		int n = 2;
		double x = 0;
		while ((fabs(x1 - x0) > error) && (n <= _maxiter)) { // fabs is float-abs
			double FnX0 = Fn(x0);
			double FnX1 = Fn(x1);
			if (FnX1 - FnX0 < .0001) break;
			x = x1 - (FnX1 * (x1 - x0)) / (FnX1 - FnX0);
			x0 = x1;
			x1 = x;
			n++;
		}
		return x;
	}
}