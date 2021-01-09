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
		if (_zoneRecord.rec().autoQuality == 0) timeC = _zoneRecord.rec().autoDelay;
		else timeC = _zoneRecord.rec().autoTimeC;
		_timeConst = uint16_t(_getExpCurve.uncompressTC(timeC));
		if (isDHWzone()) _zoneRecord.rec().autoRatio = RATIO_DIVIDER;
		refreshProfile(true);
		_offset_preheatCallTemp = _currProfileTempRequest + offset();
		_rollingAccumulatedRatio = REQ_ACCUMULATION_PERIOD * _zoneRecord.rec().autoRatio;
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

	int8_t Zone::getCurrTemp() const { 
		if (isDHWzone()) return  _thermalStore->currDeliverTemp();
		else return _callTS ? _callTS->get_temp() : 0; 
	}

	int16_t Zone::getReliableFractionalCallSensTemp() const {
		if (isDHWzone()) {
			return _thermalStore->currDeliverTemp() << 8;
		}
		else {
			_callTS->readTemperature();
			auto currTemp = _callTS->get_fractional_temp();
			auto re_readTemp = currTemp;
			auto re_read_count = 20;
			auto sameReadingCount = 0;
			do {
				currTemp = re_readTemp;
				_callTS->readTemperature();
				re_readTemp = _callTS->get_fractional_temp();
				if (!_callTS->hasError() && re_readTemp == currTemp) ++sameReadingCount;
				else sameReadingCount = 0;
				--re_read_count;
			} while (re_read_count > 0 && sameReadingCount < 5);

			return currTemp;
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

	void Zone::nextAveragedRatio(int16_t currFractionalTemp) const {
		if (_averagePeriod > 0 && /*isCallingHeat() &&*/ _mixValveController->zoneHasControl(_relay->recordID())) {
			++_averagePeriod;
			if (_averagePeriod > REQ_ACCUMULATION_PERIOD) {
				auto measuredRatio = int16_t(RATIO_DIVIDER * (currFractionalTemp / 256. - _thermalStore->getOutsideTemp()) / (_mixValveController->flowTemp() - _thermalStore->getOutsideTemp()));
				_rollingAccumulatedRatio -= averageThermalRatio();
				_rollingAccumulatedRatio += measuredRatio;
			}
		}
	}

	uint8_t Zone::averageThermalRatio() const {
		return _rollingAccumulatedRatio / REQ_ACCUMULATION_PERIOD;
	}

	void Zone::saveThermalRatio() {
		if (_averagePeriod >= REQ_ACCUMULATION_PERIOD) {
			zoneRecord().rec().autoRatio = averageThermalRatio();
			logger() << "Save new ratio for " << zoneRecord().rec().name << " : " << zoneRecord().rec().autoRatio / RATIO_DIVIDER << L_endl;
			zoneRecord().update();
		}
		_averagePeriod = 0;
	}

	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		// lambdas

		auto logTemps = [this](uint16_t currTempReq, int16_t fractionalZoneTemp, int16_t reqFlow, double ratio) {
			zTempLogger() << L_time << L_tabs 
				<< zoneRecord().rec().name 
				<< F("Req/Is:") << currTempReq << fractionalZoneTemp / 256.
				<< F("Flow Req/Is:") << reqFlow	<< (isCallingHeat() ? _mixValveController->flowTemp() : fractionalZoneTemp / 256)
				<< F("Ave Ratio:") << ratio 
				<< F("Is:") << (fractionalZoneTemp / 256. - _thermalStore->getOutsideTemp()) / (_mixValveController->flowTemp() - _thermalStore->getOutsideTemp())
				<< F("Outside:") << _thermalStore->getOutsideTemp() << "AvePer" << _averagePeriod
				<< L_endl;
		};

		auto startMeasuringRatio = [this](int tempError, int roomTemp) -> bool {
			bool backBoilerIsOn = _relay->recordID() == R_DnSt && _thermalStore->backBoilerIsHeating();
			if (backBoilerIsOn) {
				saveThermalRatio();
				return false;
			} else {
				return
				_averagePeriod == 0
				&& abs(tempError) < 16
				&& _mixValveController->flowTemp() > roomTemp
				&& _offset_preheatCallTemp > _thermalStore->getOutsideTemp();
			}
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

		int16_t fractionalZoneTemp = getReliableFractionalCallSensTemp(); // get fractional temp. msb is temp is degrees
		uint16_t roomTemp = fractionalZoneTemp / 256;

		nextAveragedRatio(fractionalZoneTemp);
		auto ratio = averageThermalRatio() / RATIO_DIVIDER;

		if (_callTS->hasError()) {
			logger() << L_endl << L_time << F("Zone::setZFlowTemp\tCall TS Error for zone ") << zoneNames(_recordID) << L_endl;
			return false;
		}

		int16_t requestedFlowTemp = 0;
		int16_t tempError = (fractionalZoneTemp - (_offset_preheatCallTemp << 8)) / 16; // 1/16 degrees +ve = Temp too high
		if (_thermalStore->dumpHeat()) {// turn zone on to dump heat
			tempError = -10;
		}
		auto logger_RelayStatus = _relay->logicalState();

		if (_minsInDelay == 0 && fractionalZoneTemp >= _startCallTemp + 256 / 8) {
			_minsInDelay = uint8_t(_minsInPreHeat);
		}

		if (tempError < -8L) {
			requestedFlowTemp = _maxFlowTemp;
			++_minsInPreHeat;
			logTemps(_offset_preheatCallTemp, fractionalZoneTemp, requestedFlowTemp, ratio);
		} else {
			auto errorOffset = tempError / ERROR_DIVIDER / ratio; // gives theoretical room-temp offset of twice the error
			requestedFlowTemp = uint16_t( _thermalStore->getOutsideTemp() - errorOffset + (fractionalZoneTemp/256. - _thermalStore->getOutsideTemp()) / ratio);
			if (tempError > 8L && requestedFlowTemp > MIN_FLOW_TEMP) { // too warm and out of range, ratio too low
				ratio = (fractionalZoneTemp / 256. - _thermalStore->getOutsideTemp() - tempError/ERROR_DIVIDER) / (MIN_FLOW_TEMP - _thermalStore->getOutsideTemp());
				zoneRecord().rec().autoRatio = ratio * RATIO_DIVIDER;
				logger() << "Ratio too low. Save new ratio for " << zoneRecord().rec().name << " : " << ratio << L_endl;
				zoneRecord().update();
				_rollingAccumulatedRatio = REQ_ACCUMULATION_PERIOD * _zoneRecord.rec().autoRatio;
			}
			if (requestedFlowTemp <= roomTemp) { requestedFlowTemp = roomTemp; }
			else if (requestedFlowTemp >= _maxFlowTemp) requestedFlowTemp = _maxFlowTemp;
			else if (startMeasuringRatio(tempError, roomTemp)) {
				_averagePeriod = 1;
			}
			if (_callFlowTemp == _maxFlowTemp && requestedFlowTemp < _maxFlowTemp) _finishedFastHeating = true;
			logTemps(_offset_preheatCallTemp, fractionalZoneTemp, requestedFlowTemp, ratio);
		}

		if (_mixValveController->amControlZone(uint8_t(requestedFlowTemp), _maxFlowTemp, _relay->recordID())) { // I am controlling zone, so set flow temp
		}
		else { // not control zone
			_relay->set(tempError < 0); // too cool
		}

		_callFlowTemp = (int8_t)requestedFlowTemp;

		return (tempError < 0);
	}

	ProfileInfo Zone::refreshProfile(bool reset) { // resets to original profile for UI.
		// Lambdas
		auto isTimeForNextEvent = [this]() -> bool { return clock_().now() >= _ttEndDateTime; };
		auto notAdvancedToNextProfile = [this, reset]() -> bool { return reset || _ttStartDateTime <= clock_().now(); };
		
		// Algorithm
		auto profileDate = notAdvancedToNextProfile() ? clock_().now() : nextEventTime();
		auto profileInfo = _sequencer->getProfileInfo(id(), profileDate);
	
		if (reset || isTimeForNextEvent()) {
			saveThermalRatio();			
			setNextEventTime(profileInfo.nextEvent);
			setNextProfileTempRequest(profileInfo.nextTT.time_temp.temp());
			setTTStartTime(clock_().now());
			setProfileTempRequest(profileInfo.currTT.temp());
		}

		return profileInfo;
	}

	void Zone::preHeatForNextTT() { // must be called once every 10 mins to record temp changes.
		// Modifies _currProfileTempRequest to allow for heat-up for next event
		// returns true when actual programmed time has expired.

		using namespace Date_Time;
		auto & zoneRec = _zoneRecord.rec();
		auto nextTempReq = nextTempRequest();
		auto outsideTemp = isDHWzone() ? uint8_t(20) : _thermalStore->getOutsideTemp();
		auto currTemp_fractional = getReliableFractionalCallSensTemp();
		
		// Lambdas
		auto maxPredictedAchievableTemp = [this, outsideTemp, &zoneRec](int maxFlowTemp) -> int {
			if (isDHWzone()) {
				return _thermalStore->calcCurrDeliverTemp(45, outsideTemp, float(maxFlowTemp), float(maxFlowTemp), float(maxFlowTemp));
			} else
				return int(outsideTemp + 0.5 + (maxFlowTemp - outsideTemp) * zoneRec.autoRatio / RATIO_DIVIDER) ;
		};

		auto calculatePreheatTime = [this, currTemp_fractional, outsideTemp, &zoneRec
			, maxPredictedAchievableTemp](int nextTempReq) -> int {
			int minsToAdd = 0;
			double finalDiff_fractional = (nextTempReq * 256.) - currTemp_fractional;
			double limitTemp = maxPredictedAchievableTemp(_maxFlowTemp);
			
			double limitDiff_fractional = limitTemp * 256. - currTemp_fractional;	
			double logRatio = 1. - (finalDiff_fractional / limitDiff_fractional);
			minsToAdd = int(-_timeConst * log(logRatio));
			return minsToAdd > 0 ? minsToAdd : 0;
		};
			
		auto offerCurrTempToCurveMatch = [this, currTemp_fractional, &zoneRec]() {
			if (_getExpCurve.nextValue(currTemp_fractional)) { // Only registers if _getExpCurve.firstValue was non-zero
				logger() << L_time << zoneRec.name << " Preheat. Record Temp:\t" << L_fixed << currTemp_fractional << L_endl;
			}
		};

		auto saveNewPreheatParameters = [this, &zoneRec, outsideTemp](int finalTemp, int limit, int timePeriod, int quality) {
			timePeriod -= _minsInDelay;
			auto timeConst = uint16_t(-double(timePeriod) / log(double(limit - finalTemp) / double(limit - _startCallTemp)));
			auto changeInTC = timeConst - _timeConst;
			//if (abs(changeInTC) > _timeConst / 5) { // unexpected large change in TC. Probably spurious, but reduce existing quality to allow recalculation.
			//	_timeConst += changeInTC / 5;
			//	quality = (quality + 1) / 2;
			//	logger() << "\tLarge change in TC. Limited to 20%\n";
			//} else 
				_timeConst = timeConst;
			auto compressedTimeConst = _getExpCurve.compressTC(_timeConst);
			zoneRec.autoTimeC = compressedTimeConst;
			zoneRec.autoQuality = quality;
			zoneRec.autoDelay = _minsInDelay;
			_zoneRecord.update();
			logger() << "\tSaved: New Limit: "
			<< limit/256.
			<< " MaxFlowT: " << _maxFlowTemp
			<< " New TC: " << _timeConst << " Compressed TC: " << compressedTimeConst
			<< " New Quality: " << quality
			<< " New Delay: " << _minsInDelay 
			<< " OS Temp: " << outsideTemp
			<< L_endl;
			_averagePeriod = 0;
		};

		auto getPreheatTemp = [this, calculatePreheatTime, zoneRec]() -> int {
			int minsToAdd;
			bool needToStart;
			auto info = refreshProfile(false);
			if (getCallFlowT() == _maxFlowTemp) return _offset_preheatCallTemp;// is already heating. Don't need to check for pre-heat
			auto preheatCallTemp = info.currTT.temp() + offset();
			do {
				minsToAdd = calculatePreheatTime(info.nextTT.temp() + offset()) + zoneRec.autoDelay + 5;
				//if (minsToAdd < 10) minsToAdd = 10; // guaruntee we always start recording a pre-heat
				logger() << "\t" << minsToAdd << " mins preheat for " << zoneRec.name << " : " << info.nextEvent << L_endl << L_endl;
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
		logger() << L_time << "Get ProfileInfo for " << zoneRec.name << " Curr Temp: " << L_fixed << currTemp_fractional << L_dec << " CurrStart: " << _ttStartDateTime << L_endl;
		
		auto currTempReq = currTempRequest();
		auto newPreheatTemp = getPreheatTemp();
		if (_finishedFastHeating) {
			auto resultQuality = (currTemp_fractional - _startCallTemp) / 25;
			logger() << L_time << zoneRec.name 
				<< " Preheat OK. Req " << _offset_preheatCallTemp 
				<< " is: " << L_fixed  << currTemp_fractional 
				<< L_dec << " Range: " << resultQuality
				<< " Ratio: " << zoneRec.autoRatio / RATIO_DIVIDER
				<< " Delay: " << _minsInDelay
				<< L_endl;
			auto missedTargetTime = _ttStartDateTime.minsTo(clock_().now());
			logger() << "\tMissed Target Time by " << missedTargetTime << " Aiming for " << _ttStartDateTime << L_endl;
			if (abs(missedTargetTime) > 10 && resultQuality >= zoneRec.autoQuality) {
				saveNewPreheatParameters(currTemp_fractional, maxPredictedAchievableTemp(_maxFlowTemp)*256, _minsInPreHeat, resultQuality);
			}
			_finishedFastHeating = false;
			_minsInPreHeat = 0;
			_startCallTemp = currTemp_fractional;
		}
		if (newPreheatTemp > _offset_preheatCallTemp) {
			logger() << "\tFirst Temp Registered to pre-heat " << zoneRec.name << " temp is: " << L_fixed << currTemp_fractional << L_endl;
			_startCallTemp = currTemp_fractional;
			_minsInPreHeat = 0;
			_minsInDelay = 0;
		} 
		_offset_preheatCallTemp = newPreheatTemp;
	}

}