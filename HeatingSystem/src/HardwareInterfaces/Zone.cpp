#include "Zone.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include "..\Assembly\Sequencer.h"
#include "..\Assembly\HeatingSystem_Queries.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_Spell.h"
#include "..\Client_DataStructures\Data_TimeTemp.h"
#include "ThermalStore.h"
#include "MixValveController.h"
#include "RDB.h"
#include <Clock.h>

Logger& zTempLogger();

namespace HardwareInterfaces {

	using namespace Assembly;
	using namespace GP_LIB;
	using namespace Date_Time;
	using namespace client_data_structures;

	Sequencer* Zone::_sequencer;

	//***************************************************
	//              Zone Dynamic Class
	//***************************************************
#ifdef ZPSIM
	Zone::Zone(UI_TempSensor & ts, int reqTemp, UI_Bitwise_Relay & callRelay )
		: _callTS(&ts)
		, _currProfileTempRequest(reqTemp)
		, _relay(&callRelay)
		, _finishedFastHeating(false)
		, _maxFlowTemp(65)
	{}

#endif
	void Zone::initialise(Answer_R<client_data_structures::R_Zone> zoneRecord, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController) {
		_callTS = &callTS;
		_mixValveController = &mixValveController;
		_relay = &callRelay;
		_thermalStore = &thermalStore;
		_zoneRecord = zoneRecord;
		_recordID = zoneRecord.id();
		_maxFlowTemp = zoneRecord.rec().maxFlowTemp;
		uint8_t timeC;
		if (_zoneRecord.rec().autoQuality == 0) timeC = _zoneRecord.rec().manHeatTime;
		else timeC = _zoneRecord.rec().autoTimeC;
		_timeConst = uint16_t(_getExpCurve.uncompressTC(timeC));
		_accumulationPeriod = _timeConst / ACCUMULATION_PERIOD_DIVIDER;
		if (isDHWzone()) _zoneRecord.rec().autoRatio = RATIO_DIVIDER;
		refreshProfile();
		_offset_preheatCallTemp = _currProfileTempRequest + offset();
	}

	void Zone::offsetCurrTempRequest(int8_t val) {
		_offsetT = (val - _currProfileTempRequest);
	}

	bool Zone::isDHWzone() const {
		return (_relay->recordID() == R_Gas);
	}

	bool Zone::isCallingHeat() const {
		return _relay->logicalState();
	}

	int8_t Zone::getCurrTemp() const { return _callTS ? ((getFractionalCallSensTemp() + 128) >> 8) : 0; }

	int16_t Zone::getFractionalCallSensTemp() const {
		if (isDHWzone()) {
			return _thermalStore->currDeliverTemp() << 8;
		}
		else {
			return _callTS->get_fractional_temp();
		}
	}

	int8_t Zone::modifiedCallTemp(int8_t callTemp) const {
		callTemp += _offsetT;
		//if (callTemp == 246) {
		//	logger() << L_time << F("Zone::modifiedCallTemp. callTemp: ") << callTemp << F(" Offset: ") << _offsetT << L_endl;
		//}		
		if (callTemp < MIN_ON_TEMP) callTemp = MIN_ON_TEMP;
		return callTemp;
	}

	int8_t Zone::maxUserRequestTemp() const {
		if (getCurrTemp() < _currProfileTempRequest) return _currProfileTempRequest;
		else return getCurrTemp() + 1;
	}

	int16_t Zone::measuredThermalRatio() const {
		if (_callFlowTemp < MIN_FLOW_TEMP) return RATIO_DIVIDER;
		else return int16_t(RATIO_DIVIDER * (getFractionalCallSensTemp()/256. - _thermalStore->getOutsideTemp()) / (_mixValveController->flowTemp() - _thermalStore->getOutsideTemp()));
	}

	uint8_t Zone::averageThermalRatio() {
		return _averagePeriod < _accumulationPeriod ? zoneRecord().rec().autoRatio : (_rollingAccumulatedRatio + _accumulationPeriod/2) / _accumulationPeriod;
	}

	void Zone::saveThermalRatio() {
		if (_averagePeriod >= _accumulationPeriod) {
			zoneRecord().rec().autoRatio = (_rollingAccumulatedRatio + _accumulationPeriod/2) / _accumulationPeriod;
			logger() << "Save new ratio: " << zoneRecord().rec().autoRatio << L_endl;
			zoneRecord().update();
		}
		_averagePeriod = 0;
		_rollingAccumulatedRatio = 0;
	}

	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		// lambdas

		auto logTemps = [this](uint16_t currTempReq, int16_t fractionalZoneTemp, int16_t reqFlow, double ratio) {
			zTempLogger() << L_time << L_tabs 
				<< zoneRecord().rec().name 
				<< F("Req/Is:") << currTempReq << fractionalZoneTemp / 256.
				<< F("Flow Req/Is:") << reqFlow	<< _mixValveController->flowTemp()
				<< (_averagePeriod < _accumulationPeriod? F("Prev Ratio/Is:") : F("Curr Ratio/Is:")) << ratio << measuredThermalRatio() / RATIO_DIVIDER
				<< F("Ave:") << ((_rollingAccumulatedRatio + _accumulationPeriod / 2) / _accumulationPeriod) / RATIO_DIVIDER
				<< F("Outside:") << _thermalStore->getOutsideTemp() 
				<< L_endl;
		};

		// Algorithm
		if (isDHWzone()) {
			bool needHeat;
			needHeat = _thermalStore->needHeat(_offset_preheatCallTemp, nextTempRequest());
			//logger() << F("Zone::setZFlowTemp for DHW\t NeedHeat?") << needHeat << L_endl;
			_relay->set(needHeat);
			if (_callFlowTemp == _maxFlowTemp && !needHeat) 
				_finishedFastHeating = true;
			_callFlowTemp = needHeat ? _maxFlowTemp : 30;
			return needHeat;
		}

		int16_t fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp. msb is temp is degrees
		uint16_t roomTemp = getCurrTemp();

		if (_averagePeriod != 0) {
			++_averagePeriod;
			auto measuredRatio = measuredThermalRatio();
			_rollingAccumulatedRatio -= _rollingAccumulatedRatio / _accumulationPeriod;
			_rollingAccumulatedRatio += measuredRatio;
			if (_averagePeriod < _accumulationPeriod && abs(_rollingAccumulatedRatio / _accumulationPeriod - measuredRatio) < 2) {
				if (roomTemp == _offset_preheatCallTemp)
					_averagePeriod = _accumulationPeriod;
			}
		}
		auto ratio = averageThermalRatio() / RATIO_DIVIDER;


		if (_callTS->hasError()) {
			logger() << L_endl << L_time << F("Zone::setZFlowTemp\tCall TS Error for zone ") << zoneNames(_recordID) << L_endl;
			return false;
		}

		int16_t myFlowTemp = 0;
		int16_t tempError = (fractionalZoneTemp - (_offset_preheatCallTemp << 8)) / 16; // 1/16 degrees +ve = Temp too high
		if (_thermalStore->dumpHeat()) {// turn zone on to dump heat
			tempError = -10;
		}
		auto logger_RelayStatus = _relay->logicalState();

		if (tempError < -8L) {
			myFlowTemp = _maxFlowTemp;
			logTemps(_offset_preheatCallTemp, fractionalZoneTemp, myFlowTemp, ratio);
		} else {
			auto errorOffset = tempError / ERROR_DIVIDER / ratio; // gives theoretical room-temp offset of twice the error
			myFlowTemp = uint16_t( _thermalStore->getOutsideTemp() - errorOffset + (fractionalZoneTemp/256. - _thermalStore->getOutsideTemp()) / ratio);
			if (myFlowTemp <= roomTemp) { myFlowTemp = roomTemp; }
			else if (myFlowTemp >= _maxFlowTemp) myFlowTemp = _maxFlowTemp;
			else {
				if (_averagePeriod == 0 && abs(tempError) < 16 && _mixValveController->flowTemp() > roomTemp && _offset_preheatCallTemp > _thermalStore->getOutsideTemp()) {
					_averagePeriod = 1;
					_rollingAccumulatedRatio = _accumulationPeriod * zoneRecord().rec().autoRatio;
				}
			}
			if (_callFlowTemp == _maxFlowTemp && myFlowTemp < _maxFlowTemp) _finishedFastHeating = true;
			logTemps(_offset_preheatCallTemp, fractionalZoneTemp, myFlowTemp, ratio);
		}

		if (_mixValveController->amControlZone(uint8_t(myFlowTemp), _maxFlowTemp, _relay->recordID())) { // I am controlling zone, so set flow temp
		}
		else { // not control zone
			_relay->set(tempError < 0); // too cool
		}

		_callFlowTemp = (int8_t)myFlowTemp;

		return (tempError < 0);
	}

	ProfileInfo Zone::refreshProfile(bool reset) { // resets to original profile for UI.
		auto profileDate = nextEventTime();
		if (reset || _currProfileTempRequest != _nextProfileTempRequest) {
			profileDate = clock_().now();
			reset = true;
		}
		auto profileInfo = _sequencer->getProfileInfo(id(), profileDate);
		if (profileInfo.currTT.temp() != _currProfileTempRequest) saveThermalRatio();
		setNextEventTime(profileInfo.nextEvent);
		setNextProfileTempRequest(profileInfo.nextTT.time_temp.temp());
		if (reset) setProfileTempRequest(profileInfo.currTT.temp());
		return profileInfo;
	}

	void Zone::preHeatForNextTT() { // must be called once every 10 mins to record temp changes.
		// Modifies _currProfileTempRequest to allow for heat-up for next event
		// returns true when actual programmed time has expired.

		using namespace Date_Time;
		auto & zoneRec = _zoneRecord.rec();
		auto nextTempReq = nextTempRequest();
		auto currTemp_fractional = getFractionalCallSensTemp();
		auto outsideTemp = isDHWzone() ? uint8_t(20) : _thermalStore->getOutsideTemp();
		
		//if (clock_().now().asInt() == 0 && zoneRec.autoQuality > 1) { // reduce quality by 10% each day.
		//	zoneRec.autoQuality -= uint8_t(zoneRec.autoQuality * 0.1 + .8);
		//	zoneAnswer.update();
		//	logger() << "autoQuality reduced to " << zoneRec.autoQuality << " for " << zoneRec.name;
		//}

		// Lambdas
		auto maxPredictedAchievableTemp = [this, outsideTemp, &zoneRec]() -> int {
			if (isDHWzone()) {
				return _thermalStore->calcCurrDeliverTemp(45, outsideTemp, _maxFlowTemp, _maxFlowTemp, _maxFlowTemp);
			} else
				return int(outsideTemp + (_maxFlowTemp - outsideTemp) * zoneRec.autoRatio / RATIO_DIVIDER) ;
		};

		auto calculatePreheatTime = [this, currTemp_fractional, outsideTemp, &zoneRec
			, maxPredictedAchievableTemp](int nextTempReq) -> int {
			int minsToAdd = 0;
			double finalDiff_fractional = (nextTempReq * 256.) - currTemp_fractional;
			double limitTemp = maxPredictedAchievableTemp();
			
			double limitDiff_fractional = limitTemp * 256. - currTemp_fractional;	
			double logRatio = 1. - (finalDiff_fractional / limitDiff_fractional);
			minsToAdd = int(-_timeConst * log(logRatio));
			return minsToAdd;
		};
			
		auto offerCurrTempToCurveMatch = [this, currTemp_fractional, &zoneRec]() {
			if (_getExpCurve.nextValue(currTemp_fractional)) { // Only registers if _getExpCurve.firstValue was non-zero
				logger() << L_time << zoneRec.name << " Preheat. Record Temp:\t" << L_fixed << currTemp_fractional << L_endl;
			}
		};

		auto canUpdatePreheatParameters = [&zoneRec](int quality)->bool {return zoneRec.autoQuality > 0 && quality > 0; };
		
		auto saveNewPreheatParameters = [this, &zoneRec, outsideTemp, maxPredictedAchievableTemp](int currTempReq, GetExpCurveConsts::CurveConsts curveMatch, int quality) {
			if (curveMatch.resultOK) {
				auto compressedTimeConst = _getExpCurve.compressTC(curveMatch.timeConst);
				zoneRec.autoTimeC = compressedTimeConst;
				zoneRec.autoQuality = quality;
				_zoneRecord.update();
				logger() << "\tSaved: New Limit: " << long(maxPredictedAchievableTemp())
				<< " New TC: " << curveMatch.timeConst << " Compressed TC: " << compressedTimeConst
				<< " New Quality: " << quality << " OS Temp: " << outsideTemp << L_endl;
				_timeConst = curveMatch.timeConst;
				_accumulationPeriod = _timeConst / ACCUMULATION_PERIOD_DIVIDER;
				_averagePeriod = 0;
				_rollingAccumulatedRatio = 0;
			}
		};

		auto getPreheatTemp = [this, calculatePreheatTime]() -> int {
			int minsToAdd;
			bool needToStart;
			auto info = refreshProfile(false);
			if (getCallFlowT() == _maxFlowTemp) return _offset_preheatCallTemp;// is already heating. Don't need to check for pre-heat
			auto preheatCallTemp = info.currTT.temp() + offset();
			do {
				minsToAdd = calculatePreheatTime(info.nextTT.temp() + offset());
				if (minsToAdd < 10) minsToAdd = 10; // guaruntee we always start recording a pre-heat
				logger() << "\t" << minsToAdd << " mins preheat for: " << info.nextEvent << L_endl << L_endl;
				needToStart = clock_().now().minsTo(info.nextEvent) <= minsToAdd;
				if (needToStart) break;
				info = _sequencer->getProfileInfo(id(), info.nextEvent);
			} while (clock_().now().minsTo(info.nextEvent) < 60*24);
			if (needToStart) {
				preheatCallTemp = info.nextTT.temp() + offset();
				saveThermalRatio();
			}
			return preheatCallTemp;
		};

		// Algorithm
		/*
		 If the next profile temp is higher than the current, then calculate the pre-heat time.
		 If the current time is >= the pre-heat time, change the currTempRequest to next profile-temp and start recording temps / times.
		 When the actual temp reaches the required temp, stop recording and do a curve-match.
		 If the temp-range of the curve-match is >= to the preious best, then save the new curve parameters.
		*/
		logger() << L_time << "Get ProfileInfo for " << zoneRec.name << L_endl;
		
		auto currTempReq = currTempRequest();
		auto newPreheatTemp = getPreheatTemp();

		offerCurrTempToCurveMatch();
		if (_finishedFastHeating) {
			_finishedFastHeating = false;
			GetExpCurveConsts::CurveConsts curveMatch = _getExpCurve.matchCurve(maxPredictedAchievableTemp()*256);
			auto resultQuality = (curveMatch.range + 12) / 25; // round up
			if (canUpdatePreheatParameters(resultQuality)) {
				logger() << L_time << zoneRec.name << " Preheat OK. Req " << currTempReq << " is: " << L_fixed  << currTemp_fractional << L_dec << " Range: " << resultQuality << L_endl;
				if (resultQuality >= zoneRec.autoQuality) {
					saveNewPreheatParameters(currTempReq, curveMatch, resultQuality);
				}
			}
			_getExpCurve.firstValue(0); // prevent further updates
		}
		if (newPreheatTemp > _offset_preheatCallTemp) {
			logger() << "\tFirst Temp Registered to pre-heat " << zoneRec.name << " temp is: " << L_fixed << currTemp_fractional << L_endl;
			_getExpCurve.firstValue(currTemp_fractional);
		} 
		_offset_preheatCallTemp = newPreheatTemp;
	}

}