#include "StdAfx.h"
#include "GetExpCurveConsts.h"

//#define TEST_EXP_CURVE

#ifdef TEST_EXP_CURVE
	#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
	#include "catch.hpp"
#endif

//#define LOG_SD;
//#define debug
#ifdef debug
	#include <iostream>
	using namespace std;
#endif

//************************** Public GetExpCurveConsts Functions **************************
void GetExpCurveConsts::firstValue(int currValue) {
	_xy.firstRiseValue = 0; // Start temp. Zero indicates not started timing.
	_xy.midRiseValue = 0; // 1st Temp - 
	_xy.midRiseTime = 0; // 1st Time point - <=0 indicates not recorded 1st rise, -1 == have fixed start time.
	_xy.lastRiseValue = currValue; // Current Temp. not_recording indicates not recording
	_xy.lastRiseTime = 0;	// Current Time, averaged at Current Temp.
	_twiceMidRiseValue = 0; // last temp at doubled time
	_twiceMidRiseTime = 0; // last doubled time
	_lastRiseStartTime = 0; // Start time - not required. Scratch pad used to record time first reached current temp.
	_timeSinceStart = 0;
#ifdef debug
	cout << "Started at " << currValue << '\n';
#endif
#ifdef LOG_SD
	if (currValue == not_recording) logToSD("GetExpCurveConsts::stop"); else logToSD("GetExpCurveConsts::firstValue");
#endif
}

void GetExpCurveConsts::nextValue(int currValue) {
	//if (epDI().index() == 0) {
	//	int debug = epDI().index();
	//}
	_currValue = currValue;
	if (needsStarting()) {
		startTiming();
	} else {
		_timeSinceStart += 10;
		if (isSameVal()) {
			averageTimeAtThisValue();
			if (!isFirstRise() && periodIsDoublePreviousPeriod()) {
				shuffleRecordsAlong();
			}
		} else {
			adjustStartingTime();
			if (hasRisenEnough()) {
				if (isFirstRise()) {
					createFirstRecord();
				} else if (periodIsDoublePreviousPeriod()) {
					shuffleRecordsAlong();
				}
				recordCurrent();
			} 
		}
	}
#ifdef debug
	cout << "\n_timeSinceStart : " << _timeSinceStart << " TryValue : " << currValue << '\n'; // Start temp. Zero indicates not started timing.
	cout << "firstRiseValue : " << _xy.firstRiseValue << '\n'; // Start temp. Zero indicates not started timing.
	cout << "midRiseTime : " << _xy.midRiseTime << '\n'; // 1st Time point - <=0 indicates not recorded 1st rise, -1 == have fixed start time.
	cout << "midRiseValue : " << _xy.midRiseValue << '\n'; // 1st Temp - 
	cout << "twiceMidRiseTime : " << _twiceMidRiseTime << '\n'; // last doubled time
	cout << "twiceMidRiseValue : " << _twiceMidRiseValue << '\n'; // last temp at doubled time
	cout << "lastRiseStartTime : " << _lastRiseStartTime << '\n'; // Start time - not required. Scratch pad used to record time first reached current temp.
	cout << "lastRiseTime : " << _xy.lastRiseTime << '\n';
	cout << "lastRiseValue : " << _xy.lastRiseValue << '\n'; // Current Temp. not_recording indicates not recording
#endif
}

GetExpCurveConsts::CurveConsts GetExpCurveConsts::matchCurve() {
	CurveConsts result;
	result.resultOK = false;
	if (_xy.midRiseValue != 0 && _xy.lastRiseValue != _xy.midRiseValue) {
		result.resultOK = true;
		result.limit = calcNewLimit(result.resultOK);
		result.timeConst = calcNewTimeConst(result.limit,result.resultOK);
	}
	return result;
}

//************************** Private GetExpCurveConsts Functions **************************

U2_byte GetExpCurveConsts::calcNewLimit(bool & OK) const {
	double result = SecantMethodForEquation(LimitTemp(_xy), _currValue+1, _currValue+2, 0.1);
	if (result < _currValue || result > 0xEFFF) OK = false;
	return static_cast<S2_byte>(result);
}

U2_byte GetExpCurveConsts::calcNewTimeConst(double limit, bool & OK) const {
	double newTC = (-_xy.lastRiseTime / log((limit - _xy.lastRiseValue)/(limit - _xy.firstRiseValue)));
	if (newTC <= 0 || newTC > 3736) OK = false;
#ifdef LOG_SD
	logToSD("GetExpCurveConsts::calcNewTimeConst: Temps*10/Times",long(_xy.firstRiseValue/25.6),
	long(_xy.midRiseValue/25.6), _xy.midRiseTime,
	long(_twiceMidRiseValue/25.6),	_twiceMidRiseTime,
	long(_xy.lastRiseValue/25.6), _xy.lastRiseTime);
#endif
	return static_cast<U2_byte>(newTC);
}

S2_byte GetExpCurveConsts::_currValue;
bool GetExpCurveConsts::needsStarting() const {return _xy.firstRiseValue == 0 && _currValue >= _xy.lastRiseValue + _min_rise;}
bool GetExpCurveConsts::isSameVal() const {return _xy.lastRiseValue == _currValue;}
bool GetExpCurveConsts::isFirstRise() const {return _xy.midRiseTime <= 0;}
bool GetExpCurveConsts::hasRisenEnough() const {return _currValue >= _xy.lastRiseValue + _min_rise;}

bool GetExpCurveConsts::periodIsDoublePreviousPeriod() {
	if (_twiceMidRiseValue == 0) {
		return _xy.lastRiseTime >= 2 * _xy.midRiseTime;
	} else {
		return _xy.lastRiseTime >= 2 * _twiceMidRiseTime;
	}
}

void GetExpCurveConsts::adjustStartingTime() {
	if (_xy.midRiseTime == 0) {
#ifdef debug
	cout << "\nAdjustStartingTime\n";
#endif
		_timeSinceStart -= _xy.lastRiseTime;
		_xy.midRiseTime = -1;
		_xy.lastRiseTime = 0;
	}
}

void GetExpCurveConsts::startTiming() {
	_xy.firstRiseValue = _currValue; // Now start timing
	_xy.lastRiseValue = _currValue;
	_timeSinceStart = 0;
#ifdef LOG_SD
	logToSD("GetExpCurveConsts::startTiming");
#endif
}

void GetExpCurveConsts::averageTimeAtThisValue() {
#ifdef debug
	cout << "\nDo Average\n";
#endif
	_xy.lastRiseTime = (_timeSinceStart + _lastRiseStartTime) / 2;
#ifdef LOG_SD
	logToSD("GetExpCurveConsts::averageTimeAtThisValue\tAveTime:", 0L, long(_xy.lastRiseTime),0L);
#endif
}

void GetExpCurveConsts::createFirstRecord() {
	_xy.midRiseValue = _currValue;
	if (_xy.lastRiseTime == 0) {_xy.lastRiseTime = _timeSinceStart;}
	_xy.midRiseTime = _xy.lastRiseTime;
#ifdef LOG_SD
	logToSD("GetExpCurveConsts::createFirstRecord\tTemp[1]*10,Time:",0L, long(_xy.midRiseValue/25.6), long(_xy.midRiseTime));
#endif
}

void GetExpCurveConsts::shuffleRecordsAlong() {
#ifdef debug
	cout << "\nDo Shuffle\n";
#endif
	if (_twiceMidRiseValue != 0) {
		_xy.midRiseValue = _twiceMidRiseValue;
		_xy.midRiseTime = _twiceMidRiseTime;
	}
	_twiceMidRiseValue = _xy.lastRiseValue;
	_twiceMidRiseTime = _xy.lastRiseTime;
#ifdef LOG_SD
	logToSD("GetExpCurveConsts::shuffleRecordsAlong\tTemp[2]*10,Time:",0L, long(_twiceMidRiseValue/25.6), long(_twiceMidRiseTime));
#endif
}

void GetExpCurveConsts::recordCurrent() {
	if (_xy.lastRiseValue == _xy.midRiseValue) {
		_xy.midRiseTime = _xy.lastRiseTime;
	}
	if (_xy.lastRiseValue == _twiceMidRiseValue) {
		_twiceMidRiseTime = _xy.lastRiseTime;
	}
	_xy.lastRiseValue = _currValue;
	_xy.lastRiseTime = _timeSinceStart; // averaged start time
	_lastRiseStartTime = _timeSinceStart; // actual start time
#ifdef LOG_SD
	logToSD("GetExpCurveConsts::recordCurrent\tTemp[3]*10,Time:",0L, long(_xy.lastRiseValue/25.6), long(_xy.lastRiseTime));
#endif
}

#ifdef TEST_EXP_CURVE

#include <iostream>
using namespace std;

TEST_CASE("Start_Stop", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Start_Stop ****\n\n";
	GetExpCurveConsts h1(128), h2(128);
	h1.firstValue(18);
	h2.firstValue(20);
	REQUIRE(h1.getXY().lastRiseValue == 18);
	REQUIRE(h2.getXY().lastRiseValue == 20);
	h1.matchCurve();
	h2.matchCurve();
	h1.firstValue(20);
	h2.firstValue(18);
	REQUIRE(h1.getXY().lastRiseValue == 20);
	REQUIRE(h2.getXY().lastRiseValue == 18);
}

TEST_CASE("Start_Timing", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Start_Timing ****\n\n";
	GetExpCurveConsts h1(128), h2(128);
	h1.firstValue(18 * 256);
	h2.firstValue(20 * 256);
	h1.nextValue(18.125 * 256);
	h1.nextValue(18.125 * 256);
	REQUIRE(h1.getXY().firstRiseValue == 0);
	h1.nextValue(18.4375 * 256);
	REQUIRE(h1.getXY().firstRiseValue == 0);
	h1.nextValue(18.5 * 256); // start timing
	REQUIRE(h1.getXY().firstRiseValue == 18.5 * 256);
	REQUIRE(h2.getXY().firstRiseValue == 0);
	h1.nextValue(18.5 * 256);
	h1.nextValue(18.5 * 256);
	h1.nextValue(18.5 * 256);
	REQUIRE(h1.getXY().lastRiseTime == 15); // averaged start
	REQUIRE(h2.getXY().firstRiseValue == 0);
}

TEST_CASE("First_Record", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test First_Record ****\n\n";
	GetExpCurveConsts h1(128), h2(128);
	h1.firstValue(18 * 256);
	h1.nextValue(18.5 * 256); // t==0 start timing, temp[0] = 4736
	h1.nextValue(18.5 * 256); // t==10, adj = 5
	h1.nextValue(18.5 * 256); // t==20, adj = 10
	h1.nextValue(18.5 * 256); // t==30, adj = 15
	REQUIRE(h1.getXY().lastRiseTime == 15);
	h1.nextValue(18.9375 * 256); // t=40, first rise, adjust start -15, t=>25
	REQUIRE(h1.getXY().midRiseTime == -1);
	h1.nextValue(18.9375 * 256); // t=35
	REQUIRE(h1.getXY().lastRiseValue == 18.5 * 256);
	h1.nextValue(19 * 256); // t=45 This is 1st Record, [1] = 4864
	REQUIRE(h1.getXY().midRiseTime == 45);
	REQUIRE(h1.getXY().midRiseValue == 19 * 256);
	REQUIRE(h1.matchCurve().resultOK == false);
}

TEST_CASE("Full_Records", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Full_Records ****\n\n";
	GetExpCurveConsts h1(128), h2(128);
	h1.firstValue(18 * 256);
	h1.nextValue(18.5 * 256); // t==0 start timing, temp[0] = 4736
	h1.nextValue(18.5 * 256); // t==10, adj = 5
	h1.nextValue(18.5 * 256); // t==20, adj = 10
	h1.nextValue(18.5 * 256); // t==30, adj = 15
	h1.nextValue(18.9375 * 256); // t=40, first rise, adjust start -15, t=>25
	h1.nextValue(18.9375 * 256); // t=35
	h1.nextValue(19 * 256); // t=45 This is 1st Record, [1] = 4864
	h1.nextValue(19 * 256); // t=55, ave = 50
	h1.nextValue(19 * 256); // t=65, ave = 55
	h1.nextValue(19.25 * 256); // t=75, 
	h1.nextValue(19.5 * 256); // t=85, recorded, but not shuffled, adjust [1] time.
	h1.nextValue(19.5 * 256); // t=95, ave 90. 4992
	h1.nextValue(19.5 * 256); // t=105, ave 95. 4992
	h1.nextValue(19.5 * 256); // t=115, ave 100. 4992
	h1.nextValue(19.5 * 256); // t=125, ave 105. 4992
	REQUIRE(h1.getXY().midRiseTime == 55);
	h1.nextValue(19.5 * 256); // t=135, ave 110. shuffle.
	REQUIRE(h1.getXY().midRiseValue == 19 * 256); //= 4864
	REQUIRE(h1.getXY().midRiseTime == 55);
	h1.nextValue(19.5 * 256); // t=145, ave 115
	h1.nextValue(19.75 * 256); // t=155
	h1.nextValue(20 * 256); // t=165, recorded, but not shuffled, adjust [2] time.
}

TEST_CASE("Shift_Records", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Shift_Records ****\n\n";
	GetExpCurveConsts h1(128);
	h1.firstValue(18 * 256);
	h1.nextValue(18.5 * 256); // t=0 start timing, temp[0] = 4736
	h1.nextValue(19 * 256); // t=10 This is 1st Record, [1] = 4864
	h1.nextValue(19.5 * 256); // t=20, T=4992. Recorded [2], adjust [1] time.
	h1.nextValue(20 * 256); // t=30, T=5120. Shuffled,
	h1.nextValue(20.5 * 256); // t=40,
	REQUIRE(h1.getXY().midRiseValue == 19 * 256); //= 4864
	REQUIRE(h1.getXY().midRiseTime == 10);
	REQUIRE(h1.getXY().lastRiseValue == 20.5 * 256); //= 4992
	REQUIRE(h1.getXY().lastRiseTime == 40);
	h1.nextValue(21 * 256); // t=50, Shuffled
	h1.nextValue(21.5 * 256); // t=60,
	h1.nextValue(22 * 256); // t=70,
	h1.nextValue(22.5 * 256); // t=80,
	h1.nextValue(23 * 256); // t=90, Shuffled
	REQUIRE(h1.getXY().midRiseValue == 20.5 * 256); //= 4864
	REQUIRE(h1.getXY().midRiseTime == 40);
	REQUIRE(h1.getXY().lastRiseValue == 23 * 256); //= 4992
	REQUIRE(h1.getXY().lastRiseTime == 90);
}


TEST_CASE("Curve Match", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Curve Match ****\n\n";
	GetExpCurveConsts h1(128);
	// T1 = T0 + (TL-T0) * (1-e(-t/C))
	int T0 = 18 * 256;
	int TL = 25 * 256;
	double C = 100;
	h1.firstValue(T0);
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-10. / C)));
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-20. / C)));
	GetExpCurveConsts::CurveConsts r = h1.matchCurve();
	REQUIRE(r.resultOK == false);
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-30. / C)));
	r = h1.matchCurve();
	REQUIRE(r.resultOK == true);
	REQUIRE((r.limit >= TL - 128 && r.limit <= TL + 128));
	REQUIRE((r.timeConst >= C - 5 && r.timeConst <= C + 5));
	cout << "Limit [" << TL / 256 << "] : " << r.limit / 256.0 << " TC [" << C << "] : " << r.timeConst;

	h1.nextValue(T0 + (TL - T0) * (1 - exp(-40. / C)));
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-50. / C)));
	r = h1.matchCurve();
	REQUIRE(r.resultOK == true);
	REQUIRE((r.limit >= TL - 128 && r.limit <= TL + 128));
	REQUIRE((r.timeConst >= C - 5 && r.timeConst <= C + 5));
	cout << "Limit [" << TL / 256 << "] : " << r.limit / 256.0 << " TC [" << C << "] : " << r.timeConst;
}
#endif