//#define TEST_EXP_CURVE

#ifdef TEST_EXP_CURVE
	#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
	#include <catch.hpp>
	#include <iostream>
	using namespace std;
#endif

#include <CurveMatching.h>
#include <Logging.h>
//#include <Arduino.h>

//#define LOG_SD;
//#define debug

#ifdef debug
#include <iostream>
	using namespace std;
#endif

namespace GP_LIB {
	//************************** Public GetExpCurveConsts Functions **************************
	void GetExpCurveConsts::firstValue(int currValue) {
		_xy.firstRiseValue = 0; // Start temp. Zero indicates not started timing.
		_xy.midRiseValue = 0; // 1st Temp - 
		_xy.midRiseTime = 0; // 1st Time point - <=0 indicates not recorded 1st rise, -1 == have fixed start time.
		_xy.lastRiseValue = currValue; // Current Temp. 0 indicates not recording
		_xy.lastRiseTime = 0;	// Current Time, averaged at Current Temp.
		_twiceMidRiseValue = 0; // last temp at doubled time
		_twiceMidRiseTime = 0; // last doubled time
		_lastRiseStartTime = 0; // Start time - not required. Scratch pad used to record time first reached current temp.
		_timeSinceStart = 0;
#ifdef debug
		cout << "Started at " << currValue << '\n';
#endif
#ifdef LOG_SD
		if (currValue == 0) logger() << "GetExpCurveConsts::stop\n"; else logger() << "GetExpCurveConsts::firstValue\n";
#endif
	}

	bool GetExpCurveConsts::nextValue(int currValue) {
		_currValue = currValue;
		_timeSinceStart += 10;
		bool hasRecorded = false;
		if (needsStarting()) {
			hasRecorded = startTiming();
		} else {
			if (periodIsDoublePreviousPeriod()) {
				shuffleRecordsAlong();
			}
			if (hasRisenEnough()) {
				recordCurrent();
				hasRecorded = true;
			} else {
				averageTimeAtThisValue();
			}
		}
#ifdef debug
		cout << "\n\t_timeSinceStart : " << _timeSinceStart << " TryValue : " << currValue << '\n'; // Start temp. Zero indicates not started timing.
		cout << "\tfirstRiseValue : " << _xy.firstRiseValue << '\n'; // Start temp. Zero indicates not started timing.
		cout << "\tmidRiseTime : " << _xy.midRiseTime << '\n'; // 1st Time point - <=0 indicates not recorded 1st rise, -1 == have fixed start time.
		cout << "\tmidRiseValue : " << _xy.midRiseValue << '\n'; // 1st Temp - 
		cout << "\ttwiceMidRiseTime : " << _twiceMidRiseTime << '\n'; // last doubled time
		cout << "\ttwiceMidRiseValue : " << _twiceMidRiseValue << '\n'; // last temp at doubled time
		cout << "\tlastRiseStartTime : " << _lastRiseStartTime << '\n'; // Start time - not required. Scratch pad used to record time first reached current temp.
		cout << "\tlastRiseTime : " << _xy.lastRiseTime << '\n';
		cout << "\tlastRiseValue : " << _xy.lastRiseValue << '\n'; // Current Temp. not_recording indicates not recording
#endif
		if (hasRecorded) logger() << "\tMC_Time: " << _timeSinceStart << " [0]:" << L_fixed << _xy.firstRiseValue << " [1]:" << _xy.midRiseValue << " [2]:" << _xy.lastRiseValue << L_endl;
		return hasRecorded;
	}

	GetExpCurveConsts::CurveConsts GetExpCurveConsts::matchCurve() {
		CurveConsts result;
		result.resultOK = false;
		if (_xy.midRiseValue != 0 && _xy.lastRiseValue != _xy.midRiseValue) {
			result.resultOK = true;
			result.limit = calcNewLimit(result.resultOK);
			result.timeConst = calcNewTimeConst(result.limit, result.resultOK);
		} else {
			if (_xy.lastRiseValue == 0) _xy.lastRiseValue = _currValue;
			if (_xy.firstRiseValue == 0) _xy.firstRiseValue = _xy.lastRiseValue;
			result.limit = 0x7FFF;
		}
		result.range = _currValue - _xy.firstRiseValue;
		result.period = _timeSinceStart;
		return result;
	}

	//************************** Private GetExpCurveConsts Functions **************************

	uint16_t GetExpCurveConsts::calcNewLimit(bool& OK) const {
		double result = SecantMethodForEquation(LimitTemp(_xy), _currValue + 1, _currValue + 2, 0.1);
		if (result < _currValue) OK = false;
		if (result > 0xEFFF) result = 0xEFFF;
		return static_cast<int16_t>(result);
	}

	uint16_t GetExpCurveConsts::calcNewTimeConst(double limit, bool& OK) const {
		double newTC = (-_xy.lastRiseTime / log((limit - _xy.lastRiseValue) / (limit - _xy.firstRiseValue)));
		if (newTC <= 0 || newTC > 3736) OK = false;
#ifdef LOG_SD
		logToSD("GetExpCurveConsts::calcNewTimeConst: Temps*10/Times", long(_xy.firstRiseValue / 25.6),
			long(_xy.midRiseValue / 25.6), _xy.midRiseTime,
			long(_twiceMidRiseValue / 25.6), _twiceMidRiseTime,
			long(_xy.lastRiseValue / 25.6), _xy.lastRiseTime);
#endif
		return static_cast<uint16_t>(newTC);
	}

	int16_t GetExpCurveConsts::_currValue;
	bool GetExpCurveConsts::needsStarting() const { return _xy.firstRiseValue == 0; }

	bool GetExpCurveConsts::hasRisenEnough() const { return _currValue >= _xy.lastRiseValue + _min_rise; }

	bool GetExpCurveConsts::periodIsDoublePreviousPeriod() {
		return _timeSinceStart >= 2 * _twiceMidRiseTime;
	}

	bool GetExpCurveConsts::startTiming() {
		if (_xy.lastRiseValue && hasRisenEnough()) {
			_xy.firstRiseValue = _xy.lastRiseValue;
			_xy.midRiseValue = _currValue;
			_xy.midRiseTime = _timeSinceStart;
			_xy.lastRiseValue = _currValue;
			_xy.lastRiseTime = _timeSinceStart;
			_lastRiseStartTime = _timeSinceStart;
			_twiceMidRiseValue = _currValue;
			_twiceMidRiseTime = _timeSinceStart;
#ifdef LOG_SD
			logToSD("GetExpCurveConsts::startTiming");
#endif
			return true;
		}
		return false;
	}

	void GetExpCurveConsts::averageTimeAtThisValue() {
#ifdef debug
		cout << "\nDo Average\n";
#endif
		_xy.lastRiseTime = (_timeSinceStart + _lastRiseStartTime) / 2;
#ifdef LOG_SD
		logToSD("GetExpCurveConsts::averageTimeAtThisValue\tAveTime:", 0L, long(_xy.lastRiseTime), 0L);
#endif
	}

	void GetExpCurveConsts::shuffleRecordsAlong() {
#ifdef debug
		cout << "\nDo Shuffle\n";
#endif
		_xy.midRiseValue = _twiceMidRiseValue;
		_xy.midRiseTime = _twiceMidRiseTime;
		_twiceMidRiseValue = _xy.lastRiseValue;
		_twiceMidRiseTime = _xy.lastRiseTime;
#ifdef LOG_SD
		logToSD("GetExpCurveConsts::shuffleRecordsAlong\tTemp[2]*10,Time:", 0L, long(_twiceMidRiseValue / 25.6), long(_twiceMidRiseTime));
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
		logToSD("GetExpCurveConsts::recordCurrent\tTemp[3]*10,Time:", 0L, long(_xy.lastRiseValue / 25.6), long(_xy.lastRiseTime));
#endif
	}

}

#ifdef TEST_EXP_CURVE
using namespace GP_LIB;

TEST_CASE("Start_Requires non-zero firstValue", "[GetExpCurveConsts]") {
	cout << "\n\n**** Start_Requires non-zero firstValue ****\n\n";
	GetExpCurveConsts h1(2);
	h1.firstValue(0);
	CHECK(h1.getXY().lastRiseValue == 0);
	h1.nextValue(18);
	h1.nextValue(20);
	h1.nextValue(22);
	CHECK(h1.getXY().firstRiseValue == 0);
	CHECK(h1.getXY().lastRiseValue == 0);
	auto result = h1.matchCurve();
	CHECK(result.resultOK == false);
}

TEST_CASE("Match attempt does not stop recording", "[GetExpCurveConsts]") {
	cout << "\n\n**** Match attempt does not stop recording ****\n\n";
	GetExpCurveConsts h1(2), h2(2);
	h1.firstValue(18);
	h2.firstValue(20);
	CHECK(h1.getXY().lastRiseValue == 18);
	CHECK(h2.getXY().lastRiseValue == 20);
	auto result1 = h1.matchCurve();
	auto result2 = h2.matchCurve();
	CHECK(result1.resultOK == false);
	CHECK(result2.resultOK == false);
	h1.nextValue(20);
	h2.nextValue(22);
	CHECK(h1.getXY().firstRiseValue == 18);
	CHECK(h2.getXY().firstRiseValue == 20);
	CHECK(h1.getXY().midRiseValue == 20);
	CHECK(h2.getXY().midRiseValue == 22);
	CHECK(result1.resultOK == false);
	CHECK(result2.resultOK == false);
}

TEST_CASE("Full_Record", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Full_Records ****\n\n";
	GetExpCurveConsts h1(2);
	h1.firstValue(18);
	h1.nextValue(20); 
	h1.nextValue(22);
	CHECK(h1.getXY().firstRiseValue == 18);
	CHECK(h1.getXY().midRiseValue == 20);
	CHECK(h1.getXY().midRiseTime == 10);
	CHECK(h1.getXY().lastRiseValue == 22);
	CHECK(h1.getXY().lastRiseTime == 20);
	auto result1 = h1.matchCurve();
	CHECK(result1.resultOK == true);
}
TEST_CASE("Average Mid/Final-Time", "[GetExpCurveConsts]") {
	cout << "\n\n**** Adjust Mid-Time ****\n\n";
	GetExpCurveConsts h1(2);
	h1.firstValue(18); // start timing,
	h1.nextValue(19); // t=10
	h1.nextValue(20); // t=20,
	h1.nextValue(20); // t=30, adj LastRiseTime: 25
	h1.nextValue(20); // t=40, adj LastRiseTime: 30
	h1.nextValue(21); // t=50, adj LastRiseTime: 35
	h1.nextValue(21); // t=60, adj LastRiseTime: 40
	h1.nextValue(22); // t=70 [1] = 20@40; [2] = 22@70
	h1.nextValue(22); // t=80, adj LastRiseTime: 75
	h1.nextValue(22); // t=90, adj LastRiseTime: 80
	h1.nextValue(23); // t=100, adj LastRiseTime: 85
	h1.nextValue(24); // t=110, recorded, but not shuffled.
	h1.nextValue(24); // t=120, adj LastRiseTime: 115
	h1.nextValue(24); // t=130, adj LastRiseTime: 120
	h1.nextValue(25); // t=140, adj LastRiseTime: 125
	h1.nextValue(25); // t=150, adj LastRiseTime: 130
	CHECK(h1.getXY().midRiseValue == 20);
	CHECK(h1.getXY().midRiseTime == 40);
	CHECK(h1.getXY().lastRiseValue == 24);
	CHECK(h1.getXY().lastRiseTime == 130);
}

TEST_CASE("Shift_Records", "[GetExpCurveConsts]") {
	cout << "\n\n**** Test Shift_Records ****\n\n";
	GetExpCurveConsts h1(2);
	h1.firstValue(18); // start timing
	h1.nextValue(20); // t=10
	h1.nextValue(22); // t=20 Full Record
	h1.nextValue(24); // t=30 [2] = 24
	CHECK(h1.getXY().lastRiseValue == 24);
	CHECK(h1.getXY().lastRiseTime == 30);
	h1.nextValue(26); // t=40, Shuffled [1] = 22, [2] = 26
	CHECK(h1.getXY().midRiseValue == 22);
	CHECK(h1.getXY().midRiseTime == 20);
	CHECK(h1.getXY().lastRiseValue == 26);
	CHECK(h1.getXY().lastRiseTime == 40);
	h1.nextValue(28); // t=50
	CHECK(h1.getXY().midRiseValue == 22);
	CHECK(h1.getXY().midRiseTime == 20);
	CHECK(h1.getXY().lastRiseValue == 28);
	CHECK(h1.getXY().lastRiseTime == 50);
	h1.nextValue(30); // t=60, Shuffled
	CHECK(h1.getXY().midRiseValue == 24);
	CHECK(h1.getXY().midRiseTime == 30);
	CHECK(h1.getXY().lastRiseValue == 30);
	CHECK(h1.getXY().lastRiseTime == 60);
	h1.nextValue(32); // t=70,
	h1.nextValue(34); // t=80,
	h1.nextValue(36); // t=90,
	h1.nextValue(38); // t=100, Shuffled
	CHECK(h1.getXY().midRiseValue == 28);
	CHECK(h1.getXY().midRiseTime == 50);
	CHECK(h1.getXY().lastRiseValue == 38);
	CHECK(h1.getXY().lastRiseTime == 100);
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
	GetExpCurveConsts::CurveConsts r = h1.matchCurve();
	CHECK(r.resultOK == false);
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-20. / C)));
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	CHECK((r.limit >= TL - 128 && r.limit <= TL + 128));
	CHECK((r.timeConst >= C - 5 && r.timeConst <= C + 5));
	cout << "Limit [" << TL / 256 << "] : " << r.limit / 256.0 << " TC [" << C << "] : " << r.timeConst << endl;
	
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-30. / C)));
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	CHECK((r.limit >= TL - 128 && r.limit <= TL + 128));
	CHECK((r.timeConst >= C - 5 && r.timeConst <= C + 5));
	cout << "Limit [" << TL / 256 << "] : " << r.limit / 256.0 << " TC [" << C << "] : " << r.timeConst << endl;

	h1.nextValue(T0 + (TL - T0) * (1 - exp(-40. / C)));
	h1.nextValue(T0 + (TL - T0) * (1 - exp(-50. / C)));
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	CHECK((r.limit >= TL - 128 && r.limit <= TL + 128));
	CHECK((r.timeConst >= C - 5 && r.timeConst <= C + 5));
	cout << "Limit [" << TL / 256 << "] : " << r.limit / 256.0 << " TC [" << C << "] : " << r.timeConst << endl;
}

TEST_CASE("Actual Data", "[GetExpCurveConsts]") {
	cout << "\n\n**** Check actual data ****\n\n";
	GetExpCurveConsts h1(128);
	h1.firstValue(17.37*256);
	h1.nextValue(18 * 256);
	h1.nextValue(18.87 * 256);
	h1.getXY().midRiseTime = 120;
	h1.getXY().lastRiseTime = 360;
	GetExpCurveConsts::CurveConsts r = h1.matchCurve();
	CHECK(r.resultOK == true);
	cout << "DS Limit: " << r.limit/256 << endl;
	cout << "DS TC:    " << r.timeConst << endl;
	cout << "DS CompTC:" << (int)h1.compressTC(r.timeConst) << endl;

	h1.firstValue(16.62*256);
	h1.nextValue(17.4 * 256); //= 4864
	h1.nextValue(18 * 256); //= 4992
	h1.getXY().midRiseTime = 150;
	h1.getXY().lastRiseTime = 290;
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	cout << "US Limit: " << r.limit / 256 << endl;
	cout << "US TC:    " << r.timeConst << endl;
	cout << "US CompTC:" << (int)h1.compressTC(r.timeConst) << endl;

	h1.firstValue(11.12*256);
	h1.nextValue(15.5 * 256); //= 4864
	h1.nextValue(17.5 * 256); //= 4992
	h1.getXY().midRiseTime = 570;
	h1.getXY().lastRiseTime = 1110;
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	cout << "Flat Limit: " << r.limit / 256 << endl;
	cout << "Flat TC:    " << r.timeConst << endl;
	cout << "Flat CompTC:" << (int)h1.compressTC(r.timeConst) << endl;

	h1.firstValue(31*256);
	h1.nextValue(43 * 256); //= 4864
	h1.nextValue(49 * 256); //= 4992
	h1.getXY().midRiseTime = 30;
	h1.getXY().lastRiseTime = 60;
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	cout << "DHW1 Limit: " << r.limit / 256 << endl;
	cout << "DHW1 TC:    " << r.timeConst << endl;
	cout << "DHW1 CompTC:" << (int)h1.compressTC(r.timeConst) << endl;
	
	h1.firstValue(40*256);
	h1.nextValue(42 * 256); //= 4864
	h1.nextValue(44 * 256); //= 4864
	h1.nextValue(46 * 256); //= 4864
	h1.nextValue(48 * 256); //= 4864
	h1.nextValue(50 * 256); //= 4992
	r = h1.matchCurve();
	CHECK(r.resultOK == true);
	cout << "DHW2 Limit: " << r.limit / 256 << endl;
	cout << "DHW2 TC:    " << r.timeConst << endl;
	cout << "DHW2 CompTC:" << (int)h1.compressTC(r.timeConst) << endl;
}
#endif