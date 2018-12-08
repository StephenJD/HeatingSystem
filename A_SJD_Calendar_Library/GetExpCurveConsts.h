#pragma once
#include "A__Typedefs.h"
#include <math.h>

/// <summary>
/// Obtains the exponential curve Limit and Time-constant that matches three values obtained over a period of time. 
/// The constructor argument sets the minimum valid value rise since the previous value.
/// Values must be provided at regular time intervals, the time constant is in multiples of this interval.
/// The values used for the match are the first and last valid values and an approximatly mid-period value.
/// </summary>

class GetExpCurveConsts {
public:
	// Nested Type
	struct CurveConsts {
		U2_byte limit;
		U2_byte timeConst;
		bool resultOK;
	};
	struct XY_Values; // only required public for testing purposes

	GetExpCurveConsts(int min_rise) : _min_rise(S2_byte(min_rise)) {} //stop();

	// ************** Queries **************
	/// <summary>
	/// Call to obtain curve constants.
	/// May be called repeatedly until best result obtained.
	/// Can be called at any time after the final value is submitted.
	/// </summary>
	/// <returns></returns>
	CurveConsts matchCurve();
	XY_Values getXY() const { return _xy; } // only required for testing purposes

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
	void nextValue(int currValue);

	struct XY_Values { // only required public for testing purposes
		S2_byte firstRiseValue;
		S2_byte midRiseValue;
		S2_byte midRiseTime;
		S2_byte lastRiseValue;
		S2_byte lastRiseTime;
	};
private:
	friend class LimitTemp;
	
	// ************** Queries **************
	bool needsStarting() const;
	bool isSameVal() const;
	bool isFirstRise() const;
	bool hasRisenEnough() const;
	bool periodIsDoublePreviousPeriod();

	// ************** Modifiers **************
	void startTiming();
	void averageTimeAtThisValue();
	void adjustStartingTime();
	void createFirstRecord();
	void shuffleRecordsAlong();
	void recordCurrent();
	U2_byte calcNewLimit(bool & OK) const;
	U2_byte calcNewTimeConst(double limit, bool & OK) const;

	// ************** Data **************
	static S2_byte _currValue;
	XY_Values _xy;
	S2_byte _twiceMidRiseValue;
	S2_byte _twiceMidRiseTime;
	S2_byte _lastRiseStartTime;
	S2_byte _timeSinceStart;
	S2_byte _min_rise;
};

class LimitTemp { // functor for determining Limit of exponential curve
public:
	LimitTemp(const GetExpCurveConsts::XY_Values & xy) // double T0, double T1, double T2, int t1, int t2) 
		: T0(xy.firstRiseValue),
		T1(xy.midRiseValue),
		T2(xy.lastRiseValue),
		t1(xy.midRiseTime),
		t2(xy.lastRiseTime){}
	double operator()(double TL) const {
		if (TL < T2) return 1;
		return t1 * log((TL-T2)/(TL-T0)) - t2 * log((TL-T1)/(TL-T0));
	}
private:
	double T0, T1, T2; // values at time 0, t1 and t2.
	double t1, t2; // times
};

template<typename functor>
double SecantMethodForEquation(const functor & Fn, double x0, double x1, double error) {
	/* 
		* Secant method for solving equation F(x) = 0
		* Input:
		* x0 - the first initial approximation of the solution
		* x1 - the second initial approximation of the solution
		* Return:
		* x - the resulted approximation of the solution
	*/	
	enum {_maxiter = 50};
	int n = 2;
	double x;
	while ((fabs(x1-x0) > error ) && ( n <= _maxiter )) {
		double FnX0 = Fn(x0);
		double FnX1 = Fn(x1);
		if (FnX1 - FnX0 < .0001) break;
		x = x1 - (FnX1 * (x1 - x0)) / (FnX1 -FnX0);
		x0 = x1;
		x1 = x;
		n++;
	}
	return x;
}
