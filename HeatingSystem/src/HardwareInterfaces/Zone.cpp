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
		if (isDHWzone()) _zoneRecord.rec().autoRatio = uint8_t(MAX_RATIO * RATIO_DIVIDER);
		refreshProfile(true);
		_preheatCallTemp = _currProfileTempRequest;
		_rollingAccumulatedRatio = REQ_ACCUMULATION_PERIOD * _zoneRecord.rec().autoRatio;
		_startCallTemp = getFractionalCallSensTemp();
	}

	void Zone::changeCurrTempRequest(int8_t val) {
		// Adjust current TT.
		bool doingCurrentProfile = _ttStartDateTime <= clock_().now();
		auto profileDate = doingCurrentProfile ? clock_().now() : nextEventTime();
		auto profileInfo = _sequencer->getProfileInfo(id(), profileDate);
		auto currTT = profileInfo.currTT;
		currTT.setTemp(val);
		_sequencer->updateTT(profileInfo.currTT_ID, currTT);
		_currProfileTempRequest = val;
		cancelPreheat();
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
		if (callTemp < MIN_ON_TEMP) callTemp = MIN_ON_TEMP;
		return callTemp;
	}

	int8_t Zone::maxUserRequestTemp() const {
		if (getCurrTemp() < _currProfileTempRequest) return _currProfileTempRequest;
		else return getCurrTemp() + 1;
	}

	void Zone::saveThermalRatio() {
		if (_averagePeriod >= REQ_ACCUMULATION_PERIOD) {
			zoneRecord().rec().autoRatio = _rollingAccumulatedRatio / REQ_ACCUMULATION_PERIOD;
			profileLogger() << zoneRecord().rec().name << " Save new ratio: " << zoneRecord().rec().autoRatio / RATIO_DIVIDER << L_endl;
			zoneRecord().update();
		}
		_averagePeriod = 0;
	}

	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		int16_t fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp. msb is temp is degrees;
		double ratio;
		bool isDHW = isDHWzone();
		auto outsideTemp = isDHW ? int8_t(20) : _thermalStore->getOutsideTemp();
		bool backBoilerIsOn = _relay->recordID() == R_DnSt && _thermalStore->backBoilerIsHeating();
		bool towelRadOn = _thermalStore->heatingDemandFrom() == R_FlTR || _thermalStore->heatingDemandFrom() == R_HsTR;
		bool giveDHW_priority = !isDHW && _thermalStore->gasBoilerIsHeating() && towelRadOn;
		// lambdas

		auto startMeasuringRatio = [this, outsideTemp, isDHW, backBoilerIsOn](int tempError, int zoneTemp, int flowTemp ) -> bool {
			if (backBoilerIsOn) {
				saveThermalRatio();
				return false;
			} else {
				return
				_averagePeriod == 0
				&& abs(tempError) < 16
				//&& flowTemp > zoneTemp
				&& _preheatCallTemp > outsideTemp;
			}
		};

		auto measuredThermalRatio = [this](int16_t currFractionalTemp, int flowTemp) -> uint8_t {
			uint16_t measuredRatio;
			//if (_mixValveController->zoneHasControl(_relay->recordID())) {
			measuredRatio = uint16_t(RATIO_DIVIDER * (currFractionalTemp / 256. - _thermalStore->getOutsideTemp()) / (flowTemp - _thermalStore->getOutsideTemp()));
			//} else {
			//	measuredRatio = uint8_t(RATIO_DIVIDER * (currFractionalTemp / 256. - _thermalStore->getOutsideTemp()) / (_callFlowTemp - _thermalStore->getOutsideTemp()));
			//}
			if (measuredRatio > 255) measuredRatio = 255;
			return uint8_t(measuredRatio);
		};

		auto nextAveragedRatio = [this, measuredThermalRatio](int16_t currFractionalTemp, int flowTemp, int error) -> uint8_t {
			if (_averagePeriod > 0) {
				if (_averagePeriod == REQ_ACCUMULATION_PERIOD) {
					auto measuredRatio = measuredThermalRatio(currFractionalTemp, flowTemp);
					auto currentAverage = _rollingAccumulatedRatio / REQ_ACCUMULATION_PERIOD;
					if (measuredRatio > currentAverage && flowTemp <= MIN_FLOW_TEMP && error > 1) {
						_averagePeriod = 0; // There's outside heat coming from somewhere!
						profileLogger() << L_time << zoneRecord().rec().name << F(" Outside heat coming from somewhere!") << L_endl;
					} else {
						_rollingAccumulatedRatio -= currentAverage;
						_rollingAccumulatedRatio += measuredRatio;
					}
					return currentAverage;
				} else ++_averagePeriod;
			}
			return zoneRecord().rec().autoRatio;
		};

		auto logTemps = [this, isDHW, outsideTemp, backBoilerIsOn, measuredThermalRatio, giveDHW_priority](uint16_t currTempReq, int16_t fractionalZoneTemp, int16_t reqFlow, int flowTemp, double ratio) {
			const __FlashStringHelper* controlledBy;
			if (isDHW) {
				controlledBy = _thermalStore->principalLoad();
			} else if (giveDHW_priority) {
				controlledBy = F("DHW Priority");
			} else if (backBoilerIsOn) {
				controlledBy = F("Back Boiler");
			} else {
				controlledBy = _mixValveController->zoneHasControl(_relay->recordID()) ? F("Master") : F("Slave");
			}
			zTempLogger() << L_time << L_tabs
				<< zoneRecord().rec().name
				<< F("PreReq/Is:") << currTempReq << fractionalZoneTemp / 256.
				<< F("Flow Req/Is:") << reqFlow << flowTemp
				<< F("Used Ratio:") << ratio
				<< F("Is:") << measuredThermalRatio(fractionalZoneTemp, flowTemp) / RATIO_DIVIDER
				<< F("Ave:") << (_rollingAccumulatedRatio / REQ_ACCUMULATION_PERIOD) / RATIO_DIVIDER
				<< F("AvePer:") << _averagePeriod
				<< F("CoolPer:") << (int)_minsCooling
				<< F("Error:") << (fractionalZoneTemp - _preheatCallTemp * 256) / 256.
				<< F("Outside:") << outsideTemp
				<< F("PreheatMins:") << _minsInPreHeat
				<< controlledBy
				<< L_endl;
		};

		// Algorithm
		int16_t tempError = (fractionalZoneTemp - (_preheatCallTemp << 8)) / 16; // 1/16 degrees +ve = Temp too high
		uint16_t zoneTemp = fractionalZoneTemp / 256;
		uint8_t flowTemp;
		if (isDHW) {
			auto needHeat = _thermalStore->needHeat(_preheatCallTemp, nextTempRequest());
			ratio = MAX_RATIO;
			tempError = needHeat ? -16 : 16;
			flowTemp = _callTS->get_temp();
			if (_thermalStore->heatingDemandFrom() >= 0 || backBoilerIsOn) {
				_minsInPreHeat = PREHEAT_ENDED; // prevent TC calculation
				_minsCooling = 0; // prevent delay measurement
			}
		} else {
			if (_callTS->hasError()) {
				logger() << L_endl << L_time << F("Zone::setZFlowTemp\tCall TS Error for zone ") << zoneNames(_recordID) << L_endl;
				return false;
			}
			flowTemp = isCallingHeat() ? _mixValveController->flowTemp() : _callFlowTemp;
			ratio = nextAveragedRatio(fractionalZoneTemp, flowTemp, tempError) / RATIO_DIVIDER;
			if (_thermalStore->dumpHeat()) tempError = -16; // turn zone on to dump heat
		}
			
		if (_minsInPreHeat != PREHEAT_ENDED && _minsCooling == MEASURING_DELAY && fractionalZoneTemp >= _startCallTemp + 256 / 8) {
			if (_minsInPreHeat > 127) _minsInPreHeat = 127;
			zoneRecord().rec().autoDelay = int8_t(_minsInPreHeat);
			profileLogger() << L_time << zoneRecord().rec().name << F(" Save new delay: ") << _minsInPreHeat << L_endl;
			_minsCooling = 0;
		}

		int16_t requestedFlowTemp = 0;
		if (tempError < -8L) {
			requestedFlowTemp = _maxFlowTemp;
			if (_minsInPreHeat != PREHEAT_ENDED) ++_minsInPreHeat;
		} else {
			if (isDHW) {
				requestedFlowTemp = MIN_FLOW_TEMP;
			} else {
				auto errorOffset = tempError / ERROR_DIVIDER / ratio; // gives theoretical room-temp offset of twice the error
				requestedFlowTemp = uint16_t( outsideTemp - errorOffset + (fractionalZoneTemp/256. - outsideTemp) / ratio);
				if (tempError > 8L && requestedFlowTemp > MIN_FLOW_TEMP) { // too warm and out of range, ratio too low
					ratio = (fractionalZoneTemp / 256. - outsideTemp - tempError / ERROR_DIVIDER) / (MIN_FLOW_TEMP - outsideTemp);
					if (ratio > MAX_RATIO) ratio = MAX_RATIO;
					zoneRecord().rec().autoRatio = uint8_t(0.5 + ratio * RATIO_DIVIDER);
					profileLogger() << zoneRecord().rec().name << F(" Ratio too low. Save new ratio: ") << ratio << L_endl;
					zoneRecord().update();
					_rollingAccumulatedRatio = REQ_ACCUMULATION_PERIOD * _zoneRecord.rec().autoRatio;
				}
				if (requestedFlowTemp <= zoneTemp) { requestedFlowTemp = zoneTemp; }
				else if (requestedFlowTemp >= _maxFlowTemp) requestedFlowTemp = _maxFlowTemp;
				else if (startMeasuringRatio(tempError, zoneTemp, flowTemp)) {
					_averagePeriod = 1;
				}
			}
			if (_minsInPreHeat != PREHEAT_ENDED && _callFlowTemp == _maxFlowTemp && requestedFlowTemp < _maxFlowTemp) _finishedFastHeating = true;
		}
		
		if (requestedFlowTemp <= MIN_FLOW_TEMP) {
			if (_minsCooling < DELAY_COOLING_TIME) ++_minsCooling; // ensure sufficient time cooling before measuring heating delay 
		} else if (_minsCooling != MEASURING_DELAY) _minsCooling = 0;
		
		if (giveDHW_priority) {
			tempError = 16;
			requestedFlowTemp = MIN_FLOW_TEMP;
		}

		if ( isDHW || !_mixValveController->amControlZone(uint8_t(requestedFlowTemp), _maxFlowTemp, _relay->recordID())) { 
			_relay->set(tempError < 0); // not control zone, so just set relay
		}

		_callFlowTemp = (int8_t)requestedFlowTemp;
		logTemps(_preheatCallTemp, fractionalZoneTemp, requestedFlowTemp, flowTemp, ratio);

		return (tempError < 0);
	}

	ProfileInfo Zone::refreshProfile(bool reset) { // resets to original profile for UI.
		// Lambdas
		auto isTimeForNextEvent = [this]() -> bool { return clock_().now() >= _ttEndDateTime; };
		auto notAdvancedToNextProfile = [this, reset]() -> bool { return reset || _ttStartDateTime <= clock_().now(); };
		auto doReset = [reset, this](int currTemp, bool doingCurrent)-> bool {
			bool doReset = reset || (doingCurrent && currTemp != _currProfileTempRequest);
			if (doReset) cancelPreheat();
			return doReset;
		};

		// Algorithm
		bool doingCurrentProfile = notAdvancedToNextProfile();
		auto profileDate = doingCurrentProfile ? clock_().now() : nextEventTime();
		auto profileInfo = _sequencer->getProfileInfo(id(), profileDate);
		auto currProfileTemp = profileInfo.currTT.temp();
		if (doReset(currProfileTemp, doingCurrentProfile) || isTimeForNextEvent()) {
			saveThermalRatio();			
			setNextEventTime(profileInfo.nextEvent);
			setNextProfileTempRequest(profileInfo.nextTT.time_temp.temp());
			setTTStartTime(clock_().now());
			setProfileTempRequest(currProfileTemp);
		}

		return profileInfo;
	}

	void Zone::cancelPreheat() {
		_minsInPreHeat = PREHEAT_ENDED;
		_finishedFastHeating = false;
		_minsCooling = 0;
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
				return _thermalStore->calcCurrDeliverTemp(45, float(outsideTemp), float(maxFlowTemp), float(maxFlowTemp), float(maxFlowTemp));
			} else
				return int(outsideTemp + 0.5 + (maxFlowTemp - outsideTemp) * zoneRec.autoRatio / RATIO_DIVIDER) ;
		};

		auto calculatePreheatTime = [this, maxPredictedAchievableTemp](int currTemp_fractional, int nextTempReq) -> int {
			double minsToAdd = 0.;
			double finalDiff_fractional = (nextTempReq * 256.) - currTemp_fractional;
			double limitTemp = maxPredictedAchievableTemp(_maxFlowTemp);
			
			double limitDiff_fractional = limitTemp * 256. - currTemp_fractional;	
			double logRatio = 1. - (finalDiff_fractional / limitDiff_fractional);
			minsToAdd = -_timeConst * log(logRatio);
			if (minsToAdd < 0.) minsToAdd = 0.;
			return static_cast<int>(minsToAdd);
		};
			
		auto saveNewTimeConst = [this, &zoneRec, outsideTemp](int finalTemp, int limit, int quality) {
			int timePeriod = _minsInPreHeat - zoneRec.autoDelay;
			_timeConst = uint16_t(-double(timePeriod) / log(double(limit - finalTemp) / double(limit - _startCallTemp)));
			auto compressedTimeConst = compressTC(_timeConst);
			zoneRec.autoTimeC = compressedTimeConst;
			zoneRec.autoQuality = quality;
			_zoneRecord.update();
			profileLogger() << "\tNew Pre-heat saved. Limit: "
			<< limit/256.
			<< " MaxFlowT: " << _maxFlowTemp
			<< " TC: " << _timeConst << " Compressed TC: " << compressedTimeConst
			<< " Quality: " << quality
			<< " Delay: " << (int)zoneRec.autoDelay
			<< " OS Temp: " << outsideTemp
			<< L_endl;
			_averagePeriod = 0;
			_minsCooling = 0; // start timing cooling period prior to new delay
		};

		auto saveNewTimeDelay = [calculatePreheatTime, &zoneRec, this]() {
			auto minsToAdd = calculatePreheatTime(_startCallTemp, _preheatCallTemp);
			auto newDelay = _minsInPreHeat - minsToAdd;
			if (newDelay > 127) newDelay = 127;
			else if (newDelay < -127) newDelay = -127;
			zoneRec.autoDelay = int8_t(newDelay);
			_zoneRecord.update();
			profileLogger() << "\tNew Delay saved: " << zoneRec.autoDelay << L_endl;
		};

		auto getPreheatTemp = [this, calculatePreheatTime, &zoneRec, currTemp_fractional]() -> int {
			int minsToAdd;
			bool needToStart;
			auto info = refreshProfile(false);
			//if (getCallFlowT() == _maxFlowTemp) return _preheatCallTemp;// is already heating. Don't need to check for pre-heat
			auto preheatCallTemp = info.currTT.temp();
			do {
				minsToAdd = calculatePreheatTime(currTemp_fractional, info.nextTT.temp());
				if (minsToAdd > 0) {
					profileLogger() << "\t" << zoneRec.name << " Preheat mins " << minsToAdd << " Delay: " << zoneRec.autoDelay << L_endl;
					minsToAdd += zoneRec.autoDelay + 5;
				}
				needToStart = clock_().now().minsTo(info.nextEvent) <= minsToAdd;
				if (needToStart) break;
				info = _sequencer->getProfileInfo(id(), info.nextEvent);
			} while (clock_().now().minsTo(info.nextEvent) < 60*24);
			if (needToStart) {
				preheatCallTemp = info.nextTT.temp();
				saveThermalRatio();
				profileLogger() << zoneRec.name << " Preheat " << minsToAdd  
				<< " mins from " << L_fixed << currTemp_fractional << L_dec << " to " << int(info.nextTT.temp())
				<< " by " << info.nextEvent
				<< L_endl;
			}
			return preheatCallTemp;
		};

		auto finishedPreheat = [this](int newPreheatTemp) -> bool {
			auto finishedEarly = newPreheatTemp < _preheatCallTemp && _minsInPreHeat > 0;
			return _finishedFastHeating || finishedEarly;
		};

		// Algorithm
		/*
		 If the next profile temp is higher than the current, then calculate the pre-heat time.
		 If the current time is >= the pre-heat time, change the currTempRequest to next profile-temp and start recording temps / times.
		 When the actual temp reaches the required temp, stop recording and do a curve-match.
		 If the temp-range of the curve-match is >= to the preious best, then save the new curve parameters.
		*/
		profileLogger() << L_endl << L_time << zoneRec.name << " Get ProfileInfo. Curr Temp: " << L_fixed << currTemp_fractional << L_dec << " CurrStart: " << _ttStartDateTime << L_endl;
		
		auto currTempReq = currTempRequest();
		auto newPreheatTemp = getPreheatTemp();
		if (finishedPreheat(newPreheatTemp)) {
			auto resultQuality = (currTemp_fractional - _startCallTemp) / 25;
			profileLogger() << zoneRec.name 
				<< " Preheat OK. Req " << _preheatCallTemp 
				<< " is: " << L_fixed  << currTemp_fractional 
				<< L_dec << " Range: " << resultQuality
				<< " Ratio: " << zoneRec.autoRatio / RATIO_DIVIDER
				<< " Delay: " << zoneRec.autoDelay
				<< L_endl;
			bool amLate = _preheatCallTemp == currTempRequest() ? true : false;
			DateTime targetTime;
			if (amLate) targetTime = _ttStartDateTime; else targetTime = _ttEndDateTime;
			auto missedTargetTime = targetTime.minsTo(clock_().now());
			if (missedTargetTime) {
				profileLogger() << "\tMissed Target Time by " << missedTargetTime << " Aiming for " << targetTime << L_endl;
				if (resultQuality >= zoneRec.autoQuality) {
					saveNewTimeConst(currTemp_fractional, maxPredictedAchievableTemp(_maxFlowTemp) * 256, resultQuality);
				} else {
					saveNewTimeDelay();
				}
			}
			cancelPreheat();
			_startCallTemp = currTemp_fractional;
		}
		if (newPreheatTemp > _preheatCallTemp) {
			profileLogger() << "\tStart Preheat " << zoneRec.name << L_endl;
			_startCallTemp = currTemp_fractional;
			if (_minsCooling == DELAY_COOLING_TIME) _minsCooling = MEASURING_DELAY;
			_minsInPreHeat = 0;
		} 
		_preheatCallTemp = newPreheatTemp;
	}

}