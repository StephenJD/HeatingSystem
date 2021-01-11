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
Logger& profileLogger();

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
		_timeConst = uint16_t(uncompressTC(timeC));
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
		if (callTemp < MIN_ON_TEMP) callTemp = MIN_ON_TEMP;
		return callTemp;
	}

	int8_t Zone::maxUserRequestTemp() const {
		if (getCurrTemp() < _currProfileTempRequest) return _currProfileTempRequest;
		else return getCurrTemp() + 1;
	}

	void Zone::nextAveragedRatio(int16_t currFractionalTemp) const {
		if (_averagePeriod > 0) {			
			if (_averagePeriod >= REQ_ACCUMULATION_PERIOD) {
				int16_t measuredRatio;
				if (_mixValveController->zoneHasControl(_relay->recordID())) {
					measuredRatio = int16_t(RATIO_DIVIDER * (currFractionalTemp / 256. - _thermalStore->getOutsideTemp()) / (_mixValveController->flowTemp() - _thermalStore->getOutsideTemp()));
				} else {
					measuredRatio = int16_t(RATIO_DIVIDER * (currFractionalTemp / 256. - _thermalStore->getOutsideTemp()) / (_callFlowTemp - _thermalStore->getOutsideTemp()));
				}
				_rollingAccumulatedRatio -= averageThermalRatio();
				_rollingAccumulatedRatio += measuredRatio;
			} else ++_averagePeriod;
		}
	}

	uint8_t Zone::averageThermalRatio() const {
		return _rollingAccumulatedRatio / REQ_ACCUMULATION_PERIOD;
	}

	void Zone::saveThermalRatio() {
		if (_averagePeriod >= REQ_ACCUMULATION_PERIOD) {
			zoneRecord().rec().autoRatio = averageThermalRatio();
			profileLogger() << L_time << "Save new ratio for " << zoneRecord().rec().name << " : " << zoneRecord().rec().autoRatio / RATIO_DIVIDER << L_endl;
			zoneRecord().update();
		}
		_averagePeriod = 0;
	}

	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		int16_t fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp. msb is temp is degrees;
		double ratio;
		bool isDHW = isDHWzone();

		// lambdas

		auto logTemps = [this, isDHW](uint16_t currTempReq, int16_t fractionalZoneTemp, int16_t reqFlow, double ratio) {
			zTempLogger() << L_time << L_tabs 
				<< zoneRecord().rec().name 
				<< F("Req/Is:") << currTempReq << fractionalZoneTemp / 256.
				<< F("Flow Req/Is:") << reqFlow	<< (isCallingHeat() ? _mixValveController->flowTemp() : fractionalZoneTemp / 256)
				<< F("Ave Ratio:") << ratio 
				<< F("Is:") << (fractionalZoneTemp / 256. - _thermalStore->getOutsideTemp()) / (reqFlow - _thermalStore->getOutsideTemp())
				<< F("AvePer:") << _averagePeriod
				<< F("CoolPer:") << (int)_minsCooling
				<< F("Change:") << int(fractionalZoneTemp - _startCallTemp)/16
				<< F("Outside:") << _thermalStore->getOutsideTemp() 
				<< (isDHW ? (_thermalStore->heatingRequestOrigin() == -1 ? F("DHW") : F("Mix")) :  (_mixValveController->zoneHasControl(_relay->recordID()) ? F("Master") : F("Slave")))
				<< L_endl;
		};

		auto startMeasuringRatio = [this](int tempError, int zoneTemp) -> bool {
			bool backBoilerIsOn = _relay->recordID() == R_DnSt && _thermalStore->backBoilerIsHeating();
			if (backBoilerIsOn) {
				saveThermalRatio();
				return false;
			} else {
				return
				_averagePeriod == 0
				&& abs(tempError) < 16
				&& _mixValveController->flowTemp() > zoneTemp
				&& _offset_preheatCallTemp > _thermalStore->getOutsideTemp();
			}
		};

		// Algorithm
		if (isDHW) {
			_thermalStore->needHeat(_offset_preheatCallTemp, nextTempRequest());
			ratio = 1;
		} else {
			if (_callTS->hasError()) {
				logger() << L_endl << L_time << F("Zone::setZFlowTemp\tCall TS Error for zone ") << zoneNames(_recordID) << L_endl;
				return false;
			}
			nextAveragedRatio(fractionalZoneTemp);
			ratio = averageThermalRatio() / RATIO_DIVIDER;
		}

		uint16_t zoneTemp = fractionalZoneTemp / 256;
		int16_t requestedFlowTemp = 0;
		int16_t tempError = (fractionalZoneTemp - (_offset_preheatCallTemp << 8)) / 16; // 1/16 degrees +ve = Temp too high
		if (_thermalStore->dumpHeat()) tempError = -10; // turn zone on to dump heat
			
		if (_minsCooling == MEASURING_DELAY && fractionalZoneTemp >= _startCallTemp + 256 / 8) {
			zoneRecord().rec().autoDelay = uint8_t(_minsInPreHeat);
			zoneRecord().update();
			profileLogger() << L_time << F("Save new delay for ") << zoneRecord().rec().name << F(" : ") << _minsInPreHeat << L_endl;
			_minsCooling = 0;
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
				profileLogger() << L_time << F("Ratio too low. Save new ratio for ") << zoneRecord().rec().name << F(" : ") << ratio << L_endl;
				zoneRecord().update();
				_rollingAccumulatedRatio = REQ_ACCUMULATION_PERIOD * _zoneRecord.rec().autoRatio;
			}
			if (requestedFlowTemp <= zoneTemp) { requestedFlowTemp = zoneTemp; }
			else if (requestedFlowTemp >= _maxFlowTemp) requestedFlowTemp = _maxFlowTemp;
			else if (startMeasuringRatio(tempError, zoneTemp)) {
				_averagePeriod = 1;
			}
			if (_callFlowTemp == _maxFlowTemp && requestedFlowTemp < _maxFlowTemp) _finishedFastHeating = true;
			logTemps(_offset_preheatCallTemp, fractionalZoneTemp, requestedFlowTemp, ratio);
		}
		if (requestedFlowTemp <= MIN_FLOW_TEMP && _minsCooling < DELAY_COOLING_TIME) ++_minsCooling; // ensure sufficient time cooling before measuring heating delay 
		else if (_minsCooling != MEASURING_DELAY) _minsCooling = 0;

		if ( !isDHW && _mixValveController->amControlZone(uint8_t(requestedFlowTemp), _maxFlowTemp, _relay->recordID())) { // I am controlling zone, so set flow temp
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
		auto currTemp_fractional = getFractionalCallSensTemp();
		
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
			
		auto saveNewPreheatParameters = [this, &zoneRec, outsideTemp](int finalTemp, int limit, int timePeriod, int quality) {
			timePeriod -= zoneRec.autoDelay;
			auto timeConst = uint16_t(-double(timePeriod) / log(double(limit - finalTemp) / double(limit - _startCallTemp)));
			auto changeInTC = timeConst - _timeConst;
			//if (abs(changeInTC) > _timeConst / 5) { // unexpected large change in TC. Probably spurious, but reduce existing quality to allow recalculation.
			//	_timeConst += changeInTC / 5;
			//	quality = (quality + 1) / 2;
			//	profileLogger()() << "\tLarge change in TC. Limited to 20%\n";
			//} else 
				_timeConst = timeConst;
			auto compressedTimeConst = compressTC(_timeConst);
			zoneRec.autoTimeC = compressedTimeConst;
			zoneRec.autoQuality = quality;
			_zoneRecord.update();
			profileLogger() << "\tSaved: New Limit: "
			<< limit/256.
			<< " MaxFlowT: " << _maxFlowTemp
			<< " New TC: " << _timeConst << " Compressed TC: " << compressedTimeConst
			<< " New Quality: " << quality
			<< " New Delay: " << zoneRec.autoDelay
			<< " OS Temp: " << outsideTemp
			<< L_endl;
			_averagePeriod = 0;
			_minsCooling = 0; // start timing cooling period prior to new delay
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
				profileLogger() << "\t" << minsToAdd << " mins preheat for " << zoneRec.name << " : " << info.nextEvent << L_endl << L_endl;
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
		profileLogger() << L_time << "Get ProfileInfo for " << zoneRec.name << " Curr Temp: " << L_fixed << currTemp_fractional << L_dec << " CurrStart: " << _ttStartDateTime << L_endl;
		
		auto currTempReq = currTempRequest();
		auto newPreheatTemp = getPreheatTemp();
		if (_finishedFastHeating) {
			auto resultQuality = (currTemp_fractional - _startCallTemp) / 25;
			profileLogger() << L_time << zoneRec.name 
				<< " Preheat OK. Req " << _offset_preheatCallTemp 
				<< " is: " << L_fixed  << currTemp_fractional 
				<< L_dec << " Range: " << resultQuality
				<< " Ratio: " << zoneRec.autoRatio / RATIO_DIVIDER
				<< " Delay: " << _minsCooling
				<< L_endl;
			auto missedTargetTime = _ttStartDateTime.minsTo(clock_().now());
			profileLogger() << "\tMissed Target Time by " << missedTargetTime << " Aiming for " << _ttStartDateTime << L_endl;
			if (abs(missedTargetTime) > 10 && resultQuality >= zoneRec.autoQuality) {
				saveNewPreheatParameters(currTemp_fractional, maxPredictedAchievableTemp(_maxFlowTemp)*256, _minsInPreHeat, resultQuality);
			}
			_finishedFastHeating = false;
			_minsInPreHeat = 0;
			_minsCooling = 0;
			_startCallTemp = currTemp_fractional;
		}
		if (newPreheatTemp > _offset_preheatCallTemp) {
			profileLogger() << "\tFirst Temp Registered to pre-heat " << zoneRec.name << " temp is: " << L_fixed << currTemp_fractional << L_endl;
			_startCallTemp = currTemp_fractional;
			if (_minsCooling == DELAY_COOLING_TIME) _minsCooling = MEASURING_DELAY;
			_minsInPreHeat = 0;
		} 
		_offset_preheatCallTemp = newPreheatTemp;
	}

}