#include "debgSwitch.h"
#include "Zone_Run.h"
//#include <algorithm>
#include "D_Factory.h"
#include "TempSens_Run.h"
#include "MixValve_Run.h"
#include "Relay_Run.h"
#include "ThrmStore_Run.h"
#include "Event_Stream.h"
//#include "Zone_Sequence.h"
#include "DailyProg_Stream.h"
#include "A_Stream_Utilities.h"
#include "TimeTemp_Stream.h"
#include "ThrmStore_Run.h"
#include <math.h>

using namespace Utils;

#if defined DEBUG_INFO
	#include <iostream>
	#include <string>
	using namespace std;
#endif

volatile long Zone_Run::z_period = 1;


struct TTvalStruct {
	S1_byte temp;
	DTtype endTime;
	S1_byte nextTemp;
};

Zone_Run::Zone_Run() 
	: _currTempRequest(18),
	_nextTemp(0),
	_callFlowTemp(45),
	_tempRatio(25),
	_aveError(0),
	_aveFlow(45 * AVERAGING_PERIOD),
	_sequence(0),
	_getExpCurve(128)
	{}

S1_byte Zone_Run::getFlowSensTemp() const {
	return f->tempSensorR(getVal(zFlowTempSensr)).getSensTemp();
}

S2_byte Zone_Run::getFractionalCallSensTemp() const {
	if (isDHWzone()) {
		return f->thermStoreR().currDeliverTemp() << 8;
	} else return f->tempSensorR(getVal(zCallTempSensr)).getFractionalSensTemp();
}

bool Zone_Run::isDHWzone() const {
	return (getVal(zCallRelay) == f->thermStoreR().getVal(GasRelay));
}

#if defined (TEMP_SIMULATION)
void Zone_Run::setcurrTempRequest(S1_byte myTemp) {
	_currTempRequest = myTemp;
	z_period = 1;
	_aveError = 0;
	_aveFlow = getFlowSensTemp() * AVERAGING_PERIOD;
	_callFlowTemp = getFlowSensTemp();
}
#endif

bool Zone_Run::check() { // called every second. setZFlowTemp, mixCall & switches relays
	#if defined (ZPSIM)
	bool debug;
	U1_byte index = epD().index();
	switch (index) {
	case 0:// downstairs
		debug = true;
		break;
	case 1: // upstairs
		debug = true;
		break;
	case 2: // flat
		debug = true;
		break;
	case 3: // DHW
		debug = true;
		break;
	}
	#endif
	loadDTSequence();
	bool needHeat = setZFlowTemp();

	return needHeat;
}

// *********************************************************************
// ******************* Zone_Run Control Routines *************************************
// *********************************************************************

bool Zone_Run::setZFlowTemp() { // Called every second. sets flow temps. Returns true if needs heat.
	U1_byte	zoneRelay = getVal(zCallRelay);
	if (isDHWzone()) {
		bool needHeat;
		if(temp_sense_hasError) {
			needHeat = false;
			//logToSD("Zone_Run::setZFlowTemp for DHW\tTS Error.");
		} else {
			needHeat = f->thermStoreR().needHeat(_currTempRequest + getVal(zOffset), _nextTemp + getVal(zOffset));
			//logToSD("Zone_Run::setZFlowTemp for DHW\t NeedHeat?",needHeat);
		} 
		f->relayR(zoneRelay).setRelay(needHeat);
		return needHeat;
	}
	
	S2_byte fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp.msb is temp is degrees
	if (temp_sense_hasError) {
		//if (i2C->getThisI2CFrequency(lcd()->i2cAddress()) == 0) {
		  //logToSD("Zone_Run::setZFlowTemp\tCall TS Error for zone", epD().index());
		//}
		return false;
	}

	long myFlowTemp = 0;
	S2_byte fractionalOutsideTemp = f->thermStoreR().getFractionalOutsideTemp(); // msb is temp is degrees
	U1_byte myMaxFlowTemp = getVal(zMaxFlowT);
	S1_byte currTempReq = modifiedCallTemp(_currTempRequest);
	long tempError = (fractionalZoneTemp - (currTempReq<<8))/16; // 1/16 degrees +ve = Temp too high

	// Integrate tempError and FlowTemp every second over AVERAGING_PERIOD
	_aveError += tempError;
	_aveFlow += getFlowSensTemp();
	if (z_period <= 0){ // Adjust everything at end of AVERAGING_PERIOD. Decremented by ThrmSt_Run::check()
		#if defined (ZPSIM)
		bool debug;
		switch (epD().index()) {
		case 0:// downstairs
			debug = true;
			break;
		case 1: // upstairs
			debug = true;
			break;
		case 2: // flat
			debug = true;
			break;
		case 3: // DHW
			debug = true;
			break;
		}
		#endif
		
		float aveFractionalZoneTemp;
		long aveFractionalFlowTemp;
		aveFractionalZoneTemp = (float) _aveError*16.0F / AVERAGING_PERIOD  + currTempReq * 256; // _aveError is in 1/16 degrees. AveError and _period zeroed when temp request changed.
		aveFractionalFlowTemp = (long) 0.5F + (_aveFlow / AVERAGING_PERIOD) * 256;
		_aveError = 0;
		_aveFlow = 0;

		// Recalc _tempRatio
		if (aveFractionalZoneTemp - fractionalOutsideTemp > 1) _tempRatio = U1_byte(0.5F + (aveFractionalFlowTemp - aveFractionalZoneTemp) * 10 / (aveFractionalZoneTemp - fractionalOutsideTemp));		
		if (_tempRatio < 5) _tempRatio = 5;
		if (_tempRatio > 40) _tempRatio = 40;
			
		S2_byte myBoostFlowLimit, myTheoreticalFlow;

		do {
			myBoostFlowLimit = S2_byte(aveFractionalFlowTemp/256 + (getVal(zWarmBoost) * _tempRatio) / 10);
			S2_byte room_flow_diff = currTempReq - ((fractionalOutsideTemp + 128)>>8);
			myTheoreticalFlow = ((_tempRatio * room_flow_diff)+5) / 10 + currTempReq; // flow for required temperature
		
		} while (tempError < 0 && myTheoreticalFlow < MIN_FLOW_TEMP && ++_tempRatio);
		// calc required flow temp due to current temp error
		long flowBoostDueToError = (-tempError * _tempRatio) ;
		if (tempError > 7L) {
			myFlowTemp = MIN_FLOW_TEMP;
			// logToSD("Zone_Run::setZFlowTemp\tToo Cool: FlowTemp, MaxFlow,TempError, Zone:",(int)epDI().index(),myFlowTemp, myMaxFlowTemp,0,0,0,0,tempError * 16);
		} else if (tempError < -8L) {
			myFlowTemp = myMaxFlowTemp;
			logToSD("Zone_Run::setZFlowTemp\tToo Cool: FlowTemp, MaxFlow,TempError, Zone:",(int)epDI().index(),myFlowTemp, myMaxFlowTemp,0,0,0,0,tempError * 16);
		} else {
			myFlowTemp = static_cast<long>((myMaxFlowTemp + MIN_FLOW_TEMP)/2. -  tempError * (myMaxFlowTemp - MIN_FLOW_TEMP) / 16.);
			//U1_byte errorDivider = flowBoostDueToError > 16 ? 10 : 40; //40; 
			//myFlowTemp = myTheoreticalFlow + (flowBoostDueToError + errorDivider/2) / errorDivider; // rounded to nearest degree
			logToSD("Zone_Run::setZFlowTemp\tIn Range FlowTemp, MaxFlow,TempError, Zone:",(int)epDI().index(),myFlowTemp, myMaxFlowTemp,0,0,0,0,tempError * 16);
		}

		// check limits
		if (myFlowTemp > myMaxFlowTemp) myFlowTemp = myMaxFlowTemp;
		//else if (myFlowTemp > myBoostFlowLimit) myFlowTemp = myBoostFlowLimit;
		else if (myFlowTemp < MIN_FLOW_TEMP) {myFlowTemp = MIN_FLOW_TEMP; tempError = 0;}

		U1_byte myMixV = getVal(zMixValveID);
		if (f->mixValveR(myMixV).amControlZone(U1_byte(myFlowTemp),myMaxFlowTemp,zoneRelay)) { // I am controlling zone, so set flow temp
		} else { // not control zone
			f->relayR(zoneRelay).setRelay(tempError < 0); // too cool
		}
	} else {// keep same flow temp and reported tempError until AVERAGING_PERIOD is up to stop rapid relay switching.	
		myFlowTemp = _callFlowTemp;			
		tempError = f->relayR(getVal(zCallRelay)).getRelayState() ? -1 : 0;
	}		

	if (f->thermStoreR().dumpHeat()) {// turn zone on to dump heat
		tempError = -1; 
		myFlowTemp = myMaxFlowTemp;
	}
	_callFlowTemp = (U1_byte) myFlowTemp;
	return (tempError < 0);
}

// ************* diary functions ******************
void Zone_Run::loadDTSequence() {
	U1_ind firstDT = getVal(E_firstDT);
	U1_byte dtCount = getVal(E_NoOfDTs); 
	U1_byte arrSize = dtCount*2;
	delete _sequence;
	_sequence = new startTime[arrSize];

	populateSequence(firstDT, dtCount, arrSize); // with start and end times
	sortSequence(arrSize);

//#if defined DEBUG_INFO
//	cout << "Sorted Zone:" << int(epD().index()) << '\n';
//	for (U1_ind t = 0; t < arrSize; ++t) {
//		cout << (int)t << ": " << DateTime_Stream::getTimeDateStr(_sequence[t].start) << " : " << (int)_sequence[t].dt << '\n';
//	}
//#endif
	removeNestedDTs(arrSize);
	removePreNowDTs(arrSize);
	// reduce to final sequence
	sortSequence(arrSize);
//#if defined DEBUG_INFO
//	cout << "loadDTSequence for Zone:" << int(epD().index()) << '\n';
//	for (U1_ind t = 0; t < arrSize; ++t) {
//		cout << (int)t << ": " << DateTime_Stream::getTimeDateStr(_sequence[t].start) << " Yr:" << (int)_sequence[t].start.getYr() << " : " << (int)_sequence[t].dt << '\n';
//	}
//#endif
}

void Zone_Run::populateSequence(U1_ind firstDT, U1_byte dtCount, U1_byte arrSize) {
	U1_ind e = dtCount;
	DTtype dtNow = currentTime(true);
	for (U1_ind t = 0; t < dtCount; ++t) {
		U1_byte dtInd = firstDT + t;
		U1_ind thisDP = f->dateTimesS(dtInd).dpIndex();
		_sequence[t].start = f->dateTimesS(dtInd).getDateTime(dtInd);
		if (_sequence[t].start == DTtype(0L)) { // move spares to end
			_sequence[t].start = DTtype::JUDGEMEMT_DAY; 
			_sequence[t].dt = SPARE_DT;
		} else if (f->dailyProgS(thisDP).getDPtype() >= E_dpDayOff) { // make special DT's -ve and create +ve paired DT at end of period
			S2_byte period = f->dailyProgS(thisDP).getVal(E_period);
			DTtype expiryDate = f->dateTimesS(dtInd).addDateTime(period);
			if (expiryDate <= dtNow) { // recycle expired special DT's
				f->dateTimesS(dtInd).recycleDT();
				_sequence[t].start = DTtype::JUDGEMEMT_DAY;
				_sequence[t].dt = SPARE_DT;
			} else {
				_sequence[t].dt = -dtInd-1;
				_sequence[e].start = expiryDate;
				_sequence[e].dt = dtInd;
				++e;
			}
		} else {
			_sequence[t].dt = dtInd;
		}
	}
	for (; e < arrSize; ++e) {
		_sequence[e].start = DTtype::JUDGEMEMT_DAY;
		_sequence[e].dt = SPARE_DT;
	}
}

void Zone_Run::removeNestedDTs(U1_byte arrSize) {
	// Set starts for all weekly and end entries between a DP and the next matching -DP to Judgement day to remove them from the sequence
	S1_ind prevWkly;
	nestedSpecials nestSpec;
	U1_ind t = 0;
	while (t < arrSize) {
		//cout << "t:" << (int)t << "\n";
		S1_ind thisDT = _sequence[t].dt;
		if (thisDT == SPARE_DT) return;
		if (thisDT >= 0) { // weekly or end of special
			S1_ind nextDT = nestSpec.addDT(thisDT,prevWkly);
			if (nextDT == -1) { // prev DT ends after this new weekly or special end
				_sequence[t].start = DTtype::JUDGEMEMT_DAY;
				_sequence[t].dt = SPARE_DT;
			} else { // current special is ended
				if (nextDT == _sequence[t+1].dt && _sequence[t].start == _sequence[t+1].start) { // two DTs ending at the same time
					_sequence[t].start = DTtype::JUDGEMEMT_DAY;
					_sequence[t].dt = SPARE_DT;
				} else {
					_sequence[t].dt = nextDT;
					recycleDT(prevWkly,t,nestSpec);
				}
			}
		} else { // new Special start
			nestSpec.addDT(thisDT,prevWkly);
			_sequence[t].dt = -thisDT - 1; // reset start DP to non-negated value
		}
		++t;
	}
}

void Zone_Run::recycleDT(S1_ind prevWkly, S1_ind t, nestedSpecials & nestSpec) {
	if (prevWkly != SPARE_DT) { // recycle weekly DT?
		DTtype dtNow = currentTime(true);
		S1_ind thisDT = _sequence[t].dt;
		if (_sequence[t].start <= dtNow) {  
			f->dateTimesS(prevWkly).recycleDT();
		} else if (f->dateTimesS(prevWkly).dpIndex() == f->dateTimesS(thisDT).dpIndex()) { // adjacent duplicate weekly DPs
			f->dateTimesS(thisDT).recycleDT();
			nestSpec.addDT(prevWkly,prevWkly);
			_sequence[t].start = DTtype::JUDGEMEMT_DAY;
			_sequence[t].dt = SPARE_DT;
		}
	}
}

void Zone_Run::removePreNowDTs(U1_byte arrSize) {
	DTtype dtNow = currentTime(true);
	//startTime debug = _sequence[t+1];
	for (U1_ind t = 0; t < arrSize-1; ++t) {
		if (_sequence[t].start != DTtype::JUDGEMEMT_DAY && _sequence[t+1].start <= dtNow) {
			_sequence[t].start = DTtype::JUDGEMEMT_DAY;
			_sequence[t].dt = SPARE_DT;
		}
		//debug = _sequence[t+1];
	}
}

Zone_Run::nestedSpecials::nestedSpecials():end(0) {dtArr[0] = SPARE_DT;};

S1_ind Zone_Run::nestedSpecials::addDT(S1_ind dt, S1_ind & prevWkly) {
	// the DP array will contain only one +ve weekly and 0 or more -ve special starts.
	S1_ind nextDT = -1; prevWkly = SPARE_DT;
	int i = end-1;
	if (dt >= 0) { // if +ve it is either a replacement weekly DP or the end of a -ve special.
		if (dtArr[i] == -dt-1) { // prev DT is start of this special
			--end;
			nextDT = dtArr[end-1] >= 0 ? dtArr[end-1] : -dtArr[end-1] - 1; // Prev DT
		} else { // search backwards for a -ve entry ...
			while (i>=0 && dtArr[i] != -dt-1) {
				--i;
			}
			if (i >= 0) { // found the special start, so remove it.
				//cout << "Remove:" << int(dt) << '\n';
				for (; i<end-1; ++i) {
					dtArr[i] = dtArr[i+1];
				}
				--end;
			} else { // this is a replacement weekly DP
				//cout << "Replace Weekly with:" << int(dt) << '\n';
				prevWkly = dtArr[0];  // recycle old weekly?
				dtArr[0] = dt;
				nextDT = dt >= 0 ? dt : -dt-1;
				if (end == 0) ++end;
				else if (dtArr[end-1] < 0) nextDT = -1; // new weekly, but prev special not expired
			}
		}
	} else { // this is a new special start
		//cout << "New Spec:" << int(dt) << '\n';
		dtArr[end] = dt;
		++end;
	}
	return nextDT;
}

void Zone_Run::sortSequence(U1_byte count) {
	// bubble sort startTimes in increasing order
	for(int i = 1; i < count; ++i) {
		for(startTime * j = _sequence; j < _sequence + count-i; ++j) {
			if(*(j+1) < *j) {
				startTime temp = *j;
				*j = *(j+1);
				*(j+1) = temp;
			}   
		}    
	}
}

U1_byte Zone_Run::getNthDT(ZdtData & zd, S1_byte position) {
	if (!_sequence) loadDTSequence();
	// If position = -2 returns viewedDTposition. If position = 127 returns noOf. 
	if (position == VIEWED_DT) position = zd.viewedDTposition;
	int t = 0;
	while (t < position && _sequence[t].dt != SPARE_DT) ++t;
	if (position == 127) return t;
	else {
		zd.expiryDate = _sequence[t+1].start;
		zd.nextDT = _sequence[t+1].dt;
		return _sequence[t].dt;
	}
}

S1_byte Zone_Run::findDT(DTtype originalFromDate, DTtype originalToDate) const {
	int t = 0;
	while (_sequence[t].dt != SPARE_DT && _sequence[t].start != originalFromDate) ++t;
	if (_sequence[t].start == originalFromDate && _sequence[t+1].start == originalToDate)
		return _sequence[t].dt;
	else return -1;
}

U1_ind Zone_Run::currentWeeklyDT(ZdtData & zd, DTtype thisDate) const {
	U1_byte nextDT = 255;
	DTtype nextDateTime;
	for (U1_byte d = zd.firstDT; d < zd.lastDT ; d++){ // find weekly DT with latest start date upto thisDate
		DTtype tryDateTime = f->dateTimesS(d).getDateTime(d);
		if (tryDateTime <= thisDate  && f->dateTimesS(d).getDPtype() == E_dpWeekly) { 
			if (tryDateTime > nextDateTime) {
				nextDateTime = tryDateTime;
				nextDT = d;
			}
		}
	}
	return nextDT;
}

// ***********************************************************************
bool Zone_Run::isHeating() const {
	return f->relayR(getVal(zCallRelay)).getRelayState();
}

bool Zone_Run::checkProgram(bool force) { // returns true if currentTT has expired.
	bool returnVal = false;
	if (f) {

		U1_count noOfZones = f->numberOf[0].getVal(noZones);
		for (int i = 0; i < noOfZones; ++i) {
			Zone_Run & z = f->zoneR(i);
		
			if (force || z.ttExpired())	{
				z.refreshCurrentTT(*f);
				returnVal = true;
			}
			logToSD("Zone_Run::checkProgram\tFlowT, ProgCall, AutoCall, OfS, OtS, Req, Is. Zone:",i,
				z.getCallFlowT(),
				z.getCurrProgTempRequest(),
				z.getCurrTempRequest(),
				(signed char)z.getVal(zOffset),
				f->thermStoreR().getOutsideTemp(),
				z.modifiedCallTemp(z.getCurrProgTempRequest()),
				z.getFractionalCallSensTemp(),
				z.isHeating());
		}
		logToSD("Zone_Run::checkProgram\tGroundT, Top, Mid, Lower, Gas ", 0,
			f->thermStoreR()._groundT,
			f->thermStoreR().getTopTemp(),
			f->tempSensorR(f->thermStoreR().getVal(MidDhwTS)).getSensTemp(),
			f->tempSensorR(f->thermStoreR().getVal(LowerDhwTS)).getSensTemp(),
			f->tempSensorR(f->thermStoreR().getVal(GasTS)).getSensTemp());
	}

	return returnVal;
}

bool Zone_Run::ttExpired() { // called once every 10 mins. modifies _currTempRequest to allow for heat-up and cool-down
	// returns true when actual programmed time has expired.
#if defined DEBUG_INFO
	if (epDI().index() == 2) {
		cout << "****  Has ttExpired?  ****\n\n";
	}
#endif
	S1_byte currTempReq = modifiedCallTemp(_currProgTempRequest);
	S2_byte nextTempReq = modifiedCallTemp(_nextTemp);
	if (nextTempReq == currTempReq) {
		refreshCurrentTT(*f);
		nextTempReq = modifiedCallTemp(_nextTemp);
	}
	S2_byte currTemp = getFractionalCallSensTemp();
	double outsideDiff; 
	if (isDHWzone()) {
		outsideDiff = 0;
		if (currTemp/256 < nextTempReq) nextTempReq += THERM_STORE_HYSTERESIS;
	} else {
		outsideDiff = nextTempReq - f->thermStoreR().getOutsideTemp();
	}
	int minsToAdd = 0;
	double fractFinalTempDiff = (nextTempReq * 256) - currTemp; //- 128; // aim for 0.5 degree below requested temp
	if (nextTempReq > currTempReq && currTemp/256 < nextTempReq) { // if next temp is higher allow warm-up time
		double limitTemp = getVal(zAutoFinalTmp); // fractFinalTempDiff +  * 25.6 * (1. - outsideDiff/25.) ;
		if (limitTemp < nextTempReq + 2) limitTemp = nextTempReq + 2;
		limitTemp *= 256;
		double fractStartTempDiff = limitTemp - currTemp; 
		if (epDI().getAutoMode()) {
			double logRatio = 1. - (fractFinalTempDiff / fractStartTempDiff);
			minsToAdd = int(-epDI().getAutoConstant() * log(logRatio));
		} else {
			minsToAdd = int(fractFinalTempDiff * getVal(zManHeat) * 2 + 128) / 256; // mins per 1/2 degree
		}
	}
	int hrsToAdd = (minsToAdd / 60);
	DTtype changeTime = _ttEndDateTime.addDateTime(-(hrsToAdd * 8 + (minsToAdd % 60) / 10));
	if (currentTime() >= changeTime && _currTempRequest != _nextTemp) {
		_currTempRequest = _nextTemp;
		_getExpCurve.firstValue(currTemp);
		logToSD("Zone_Run::ttExpired\tPre-Heat: TimeToAdd(mins), CurrTempReq, AutoLimit, AutoConst. Zone:",(int)epDI().index(), (long)minsToAdd, (long)_currTempRequest, (long)getVal(zAutoFinalTmp), (long)epDI().getAutoConstant()) ;
	} else {
		_getExpCurve.nextValue(currTemp);
		if ((currTemp+128)/256 >= nextTempReq) {
			GetExpCurveConsts::CurveConsts curveMatch = _getExpCurve.matchCurve();
			if (curveMatch.resultOK) { // within 0.5 degrees and enough temps recorded. recalc TL
				logToSD("Zone_Run::ttExpired\tNewLimit, New TC. Zone:",(int)epDI().index(), long(curveMatch.limit), long(curveMatch.timeConst));
				//double newAuto = 0.5 + (newLimit - _currTempRequest) / (10 * (1-outsideDiff/25.));
				setVal(zAutoFinalTmp,curveMatch.limit / 256);
				epDI().setAutoConstant(curveMatch.timeConst);
				_getExpCurve.firstValue(0); // prevent further updates
			} 
		}
	}
	return currentTime() >= _ttEndDateTime;
}

void Zone_Run::refreshCurrentTT(FactoryObjects & fact) { // called when the day and time gets to the next Temp change or when prog changed. 
	//Sets new _ttEndDateTime and ttTemp.
	U1_byte index = epDI().index();
#if defined DEBUG_INFO
	if (index == 2) {
		debugDTs(index,"****  refreshCurrentTT  ****");
		cout << debugDPs("");
	}
#endif

	loadDTSequence(); // force recycle of duplicate weekly DTs
	fact.zoneS(index).setZdata();
	U1_ind gotCurrDT = Zone_Stream::currZdtData.currDT; 
	U1_ind gotCurrDP = Zone_Stream::currZdtData.currDP;
	E_dpTypes currDPType = Zone_Stream::currZdtData.currDPtype;
	U1_ind nextDT = Zone_Stream::currZdtData.nextDT == 127 ? gotCurrDT : Zone_Stream::currZdtData.nextDT;
	U1_ind nextDP = fact.dateTimesS(nextDT).dpIndex();

	TTvalStruct ttResult;
	_ttEndDateTime = Zone_Stream::currZdtData.expiryDate;
	if (currDPType == E_dpInOut) {
		_currTempRequest = fact.dailyPrograms[gotCurrDP].getVal(E_temp) & ~FULL_WEEK;
		ttResult = timeTempForDP(nextDP, _ttEndDateTime);
		_nextTemp = ttResult.nextTemp;
	} else { // set CurrTT to TT Temperature
		ttResult = timeTempForDP(gotCurrDP, currentTime(true));
		_currTempRequest = ttResult.temp;
		DTtype farEnough = currentTime();
		farEnough.addDay(5);
		while (ttResult.endTime > currentTime() && ttResult.endTime < farEnough) { // look for next TT with different temperature
			if (ttResult.endTime >= _ttEndDateTime) { // a new DT starts before this TT expires
				gotCurrDP = nextDP;
				ttResult = timeTempForDP(gotCurrDP, _ttEndDateTime);
				ttResult.endTime = _ttEndDateTime;
				ttResult.nextTemp = ttResult.temp;
			}
			if (_nextTemp != ttResult.nextTemp || gotCurrDP == nextDP) break;
			ttResult = timeTempForDP(gotCurrDP, ttResult.endTime);
		}
		_ttEndDateTime = ttResult.endTime;
		_nextTemp = ttResult.nextTemp;
	}
	_currProgTempRequest = _currTempRequest;

	logToSD("Zone_Run::refreshCurrentTT\tcurrTReq, nextTR. Zone: ",index, _currTempRequest,_nextTemp);

#if defined DEBUG_INFO
		std::cout << "Z:" << (int)index << " CurrDT:" << (int)gotCurrDT << " CurrDP:" << (int) gotCurrDP << " Expires:" << DateTime_Stream::getTimeDateStr(_ttEndDateTime) << '\n';
		std::cout << "    CurrTemp:" << (int)_currTempRequest << " NextTemp:" << (int) _nextTemp << '\n';
#endif
}

TTvalStruct Zone_Run::timeTempForDP(U1_ind dp, DTtype checkTime) const {
	// Is there another TT in the current DP?
	//if (dp == 127) return {0,0,0};
	E_dpTypes currDPType = f->dailyProgS(dp).getDPtype();
	TTvalStruct retVal;
	if (currDPType == E_dpInOut) { // only called if trying to get only next temp from an E_dpInOut
		retVal.temp = f->dailyPrograms[dp].getVal(E_temp) & ~FULL_WEEK;
		return retVal;
	}

	U1_ind gotCurrTT;
	// Get DP for correct day
	S2_byte currDayNo;
	if (currDPType == E_dpDayOff) { // pretend today is dayOff day.
		currDayNo = f->dailyProgS(dp).getDayOff(); // day no of regular day off
		dp = currentWeeklyDT(Zone_Stream::currZdtData, checkTime);
	} else { 
		currDayNo = checkTime.getDayNoOfDate();
	}

	S2_byte nextTT = 0; // has to allow -ve vals
	S2_byte nextDayNo;
	U1_ind gotCurrDP = findDPforDOW(dp, currDayNo);
	// Set new _ttEndDateTime and ttTemp.

	DTtype endDateTime(checkTime);
	nextDayNo = nextIndex(0,currDayNo,6, 1);
	gotCurrTT = findTTforTime(gotCurrDP, checkTime.getTime(), nextDayNo, nextTT);
	if (gotCurrTT == 255) { // gotCurrDP is yesturday
		currDayNo = nextIndex(0,currDayNo,6, -1);
		nextDayNo = nextIndex(0,currDayNo,6, 1);
		gotCurrDP = findDPforDOW(gotCurrDP, currDayNo);
		gotCurrTT = findTTforTime(gotCurrDP, DTtype::MIDNIGHT, nextDayNo, nextTT);
		endDateTime.addDay(-1);
	}
		
	// Check next TT is before expiry of DT
	if (nextDayNo < 0) { // nextTT is day after gotCurrDP
		endDateTime.addDay(1);
	}	
	endDateTime.setTime(f->timeTempS(U1_byte(nextTT)).getTime());

	retVal.temp = f->timeTempS(gotCurrTT).getTemp();
	retVal.endTime = endDateTime;
	retVal.nextTemp = f->timeTempS((U1_ind)nextTT).getTemp();
	return retVal; 
}

U1_ind Zone_Run::modifiedCallTemp(U1_byte callTemp) const {
	callTemp += getVal(zOffset);
	if (callTemp <= MIN_ON_TEMP) return callTemp;
	S1_byte modifiedTemp;
	S1_byte weatherSetback = epDI().getWeatherSetback();
	// if Outside temp + WeatherSB > CallTemp then set to 2xCallT-OSTemp-WeatherSB, else set to CallT
	S1_byte outsideTemp = f->thermStoreR().getOutsideTemp();
	if (outsideTemp + weatherSetback > callTemp){
		modifiedTemp = 2 * callTemp - outsideTemp - weatherSetback;
		if (modifiedTemp < (getFractionalCallSensTemp()>>8) && modifiedTemp < callTemp) modifiedTemp = callTemp-1;
	} else modifiedTemp = callTemp;
	return modifiedTemp;
}

// *********************************************************************
// ******************* DP Routines *************************************
// *********************************************************************

S1_ind Zone_Run::getLastDP(E_dpTypes dpType) const { // returns zfirstDP-1 if no DPs of requested type exist
	S1_ind last = getVal(E_firstDP) + getVal(E_NoOfDPs) -1;
	while (last >= getVal(E_firstDP) && f->dailyProgS(last).getDPtype() < dpType) last--;
	return last;
}

U1_ind Zone_Run::findDPforDOW(S2_byte currDP, S2_byte dayNo) const {
	char tryDP = (U1_ind) currDP;
	// Set tryDP to first DP for this group by looking for Monday
	while (tryDP >= 0 && !(f->dailyProgS(tryDP).getDays() & MONDAY))
	{tryDP--;}
	//tryDP++;
	if (tryDP < 0) return tryDP;
	U1_ind findDay = MONDAY >> dayNo;
	bool gotDP = ((f->dailyProgS(tryDP).getDays() & findDay) > 0);
	while (!gotDP && !f->dailyProgS(tryDP).endOfGroup()){
		tryDP++;
		gotDP = ((f->dailyProgS(tryDP).getDays() & findDay) > 0);
	}
	if (!gotDP) {
		tryDP = currDP - 1;
		gotDP = ((f->dailyProgS(tryDP).getDays() & findDay) > 0);
		while (!gotDP && !f->dailyProgS(tryDP).startOfGroup()){
			tryDP--;
			gotDP = ((f->dailyProgS(tryDP).getDays() & findDay) > 0);
		}
	}
	return tryDP;
}

// *********************************************************************
// ******************* TT & Temperature Routines *************************************
// *********************************************************************

U1_ind Zone_Run::findTTforTime(U1_ind currDP, DTtype myTime, S2_byte & nextDayNo, S2_byte &nextTT) const {
	// nextTT does not need to be set when calling this method.
	// looks in currDP for TT for myTime. Looks in next day's DP if necessary.
	// Returns TT for myTime and sets nextTT.
	// nextDayNo returns -1 if it is in next day.
	// returns 255 if firstTT is later than myTime.
	U1_ind currTT = 255;
	//E_dpTypes dpType = f->dailyPrograms[currDP].getDPtype();
	//if (dpType == E_dpWeekly) {
		nextTT = f->dailyPrograms[currDP].getVal(E_firstTT);
		U1_ind lastTT = nextTT + f->dailyPrograms[currDP].getVal(E_noOfTTs) -1;
		// now check to see how far into today we have got
		while (nextTT <= lastTT && f->timeTempS((U1_ind) nextTT).getTime() <= myTime){
			currTT = (U1_ind) nextTT;
			nextTT++;
		}
		if (nextTT > lastTT || f->timeTemps[nextTT].getVal(E_ttTime) == SPARE_TT) { // get first TT for next day if required
			currDP = findDPforDOW(currDP, nextDayNo);
			nextTT = f->dailyPrograms[currDP].getVal(E_firstTT);
			nextDayNo = -1;
		}
	//}
	return currTT;
}


