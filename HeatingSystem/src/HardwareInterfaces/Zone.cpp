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

namespace arduino_logger {
	Logger& zTempLogger();
	Logger& profileLogger();
}
using namespace arduino_logger;

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
	Zone::Zone(uint8_t & callTS_register, int reqTemp, UI_Bitwise_Relay & callRelay )
		: _relay(&callRelay)
		, _callTS_register(&callTS_register)
		, _currProfileTempRequest(reqTemp)
		, _finishedFastHeating(false)
		, _maxFlowTemp(65)
	{}

#endif
	void Zone::initialise(Answer_R<client_data_structures::R_Zone> zoneRecord, volatile uint8_t& callTS_register, UI_Bitwise_Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController) {
		_relay = &callRelay;
		_thermalStore = &thermalStore;
		_mixValveController = &mixValveController;
		_callTS_register = &callTS_register;
		_zoneRecord = zoneRecord;
		_recordID = zoneRecord.id();
		_maxFlowTemp = zoneRecord.rec().maxFlowTemp;
		_ttStartDateTime = clock_().now();
		_ttEndDateTime = _ttStartDateTime;
		logger() << "Set tt_start/end to: " << _ttStartDateTime << L_endl;
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

	bool Zone::changeCurrTempRequest(int8_t val) {
		if (val != _currProfileTempRequest) {
			// Adjust current TT.
			bool doingCurrentProfile = startDateTime() <= clock_().now();
			auto profileDate = doingCurrentProfile ? clock_().now() : nextEventTime();
			auto profileInfo = _sequencer->getProfileInfo(id(), profileDate);
			auto currTT = profileInfo.currTT;
			currTT.setTemp(val);
			_sequencer->updateTT(profileInfo.currTT_ID, currTT);
			_currProfileTempRequest = val;
			cancelPreheat();
			preHeatForNextTT();
			setFlowTemp();
			return true;
		} else return false;
	}

	bool Zone::isDHWzone() const {
		return (_relay->recordID() == R_Gas);
	}

	void Zone::setMode(int mode) { if (isDHWzone()) _thermalStore->setMode(mode); }

	bool Zone::isCallingHeat() const {
		return _relay->logicalState();
	}

	int8_t Zone::getCurrTemp() const { 
		if (isDHWzone()) return _thermalStore->currDeliverTemp();
		else return (getFractionalCallSensTemp() + 128) >> 8;
	}

	int16_t Zone::getFractionalCallSensTemp() const {
		if (isDHWzone()) {
			return _thermalStore->currDeliverTemp() << 8;
		}
		else {
			return ((*_callTS_register)<<8) + *(_callTS_register+1);
		}
	}

	int8_t Zone::modifiedCallTemp(int8_t callTemp) const {
		if (callTemp < MIN_ON_TEMP) callTemp = MIN_ON_TEMP;
		return callTemp;
	}

	int8_t Zone::maxUserRequestTemp(bool setLimit) const {
		//auto limitRequest = _currProfileTempRequest;
		auto limitRequest = _currProfileTempRequest < 19 ? 19 : _currProfileTempRequest;
		if (!setLimit) return 90;
		else if (getCurrTemp() < limitRequest) return limitRequest;
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

	void Zone::startNextProfile() {
		if (!advancedToNextProfile()) {
			setProfileTempRequest(nextTempRequest());
			setTTStartTime(nextEventTime());
			cancelPreheat();
			preHeatForNextTT();
			setFlowTemp();
		}
	}

	void Zone::revertToScheduledProfile() {
		if (advancedToNextProfile()) {
			refreshProfile();
			preHeatForNextTT();
			setFlowTemp();
		}
	}
	bool Zone::advancedToNextProfile() const { return startDateTime() > clock_().now(); };


	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		//decltype(millis()) executionTime[7] = { millis() };

		int16_t fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp. msb is temp is degrees;
		double ratio;
		bool isDHW = isDHWzone();
		auto outsideTemp = isDHW ? int8_t(20) : _thermalStore->getOutsideTemp();
		bool backBoilerIsOn = _relay->recordID() == R_DnSt && _thermalStore->backBoilerIsHeating();
		bool towelRadOn = _thermalStore->demandZone() == R_FlTR || _thermalStore->demandZone() == R_HsTR;
		bool giveDHW_priority = !isDHW && towelRadOn && _thermalStore->tooCoolRequestOrigin() == NO_OF_MIX_VALVES;
#ifdef ZPSIM
		giveDHW_priority = false;
#endif
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
		//executionTime[1] = millis();
		int16_t tempError = (fractionalZoneTemp - (_preheatCallTemp << 8)) / 16; // 1/16 degrees +ve = Temp too high
		uint16_t zoneTemp = fractionalZoneTemp / 256;
		uint8_t flowTemp;
		if (isDHW) {
			auto needHeat = _thermalStore->needHeat(_preheatCallTemp, nextTempRequest());
			ratio = MAX_RATIO;
			tempError = needHeat ? -16 : 16;
			flowTemp = _thermalStore->getGasFlowTemp();
			if (_thermalStore->demandZone() >= 0 || backBoilerIsOn) {
				_minsInPreHeat = PREHEAT_ENDED; // prevent TC calculation
				_minsCooling = 0; // prevent delay measurement
			}
		} else {
			flowTemp = isCallingHeat() ? _mixValveController->flowTemp() : _callFlowTemp;
			ratio = nextAveragedRatio(fractionalZoneTemp, flowTemp, tempError) / RATIO_DIVIDER;
			if (_thermalStore->dumpHeat()) tempError = -16; // turn zone on to dump heat
		}
		//executionTime[2] = millis();

		if (_minsInPreHeat != PREHEAT_ENDED && _minsCooling == MEASURING_DELAY && fractionalZoneTemp >= _startCallTemp + 256 / 8) {
			if (_minsInPreHeat > 127) _minsInPreHeat = 127;
			zoneRecord().rec().autoDelay = int8_t(_minsInPreHeat);
			profileLogger() << L_time << zoneRecord().rec().name << F(" Save new delay: ") << _minsInPreHeat << L_endl;
			_minsCooling = 0;
		}
		//executionTime[3] = millis();

		int16_t requestedFlowTemp = 0;
		if (tempError < -8L) {
			requestedFlowTemp = _maxFlowTemp;
			if (_minsInPreHeat != PREHEAT_ENDED) ++_minsInPreHeat;
		} else {
			if (isDHW) {
				requestedFlowTemp = MIN_FLOW_TEMP;
			} else if (outsideTemp >= _preheatCallTemp) {
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
		//executionTime[4] = millis();

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
		//executionTime[5] = millis();

		logTemps(_preheatCallTemp, fractionalZoneTemp, requestedFlowTemp, flowTemp, ratio);

		//executionTime[6] = millis();
		//logger() << "setFlowTemp() Lambdas: " << executionTime[1] - executionTime[0] << L_endl;
		//logger() << "setFlowTemp() GetFlowTemp: " << executionTime[2] - executionTime[1] << L_endl;
		//logger() << "setFlowTemp() Preheat: " << executionTime[3] - executionTime[2] << L_endl;
		//logger() << "setFlowTemp() CalcTemp: " << executionTime[4] - executionTime[3] << L_endl;
		//logger() << "setFlowTemp() ControlZ: " << executionTime[5] - executionTime[4] << L_endl;
		//logger() << "setFlowTemp() Log: " << executionTime[6] - executionTime[5] << L_endl;

		return (tempError < 0);
	}

	ProfileInfo Zone::refreshProfile(bool reset) { // resets to original profile for UI.
		// Lambdas
		auto isTimeForNextEvent = [this]() -> bool { return clock_().now() >= nextEventTime(); };
		auto advancedToNextProfile = [this]() -> bool { return startDateTime() > clock_().now(); };
		auto doReset = [reset, this](int currTemp, bool doingCurrent)-> bool {
			bool doReset = reset || (doingCurrent && currTemp != _currProfileTempRequest);
			if (doReset) cancelPreheat();
			return doReset;
		};

		// Algorithm
		bool doingCurrentProfile = reset || !advancedToNextProfile();
		auto profileDate = doingCurrentProfile ? clock_().now() : nextEventTime();
		auto profileInfo = _sequencer->getProfileInfo(id(), profileDate);
		auto currProfileTemp = profileInfo.currTT.temp();
		if (doReset(currProfileTemp, doingCurrentProfile) || isTimeForNextEvent()) {
			saveThermalRatio();			
			setNextEventTime(profileInfo.nextEvent);
			_nextProfileTempRequest = profileInfo.nextTT.time_temp.temp();
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
		// decltype(millis()) executionTime[8] = { millis() };
		//logger() << L_time << "preHeatForNextTT" << L_endl;
		using namespace Date_Time;
		auto & zoneRec = _zoneRecord.rec();
		auto nextTempReq = nextTempRequest();
		auto outsideTemp = isDHWzone() ? uint8_t(20) : _thermalStore->getOutsideTemp();
		auto currTemp_fractional = getFractionalCallSensTemp();
		//logger() << L_time << "\tini_OK" << L_endl;
		
		// Lambdas
		auto maxPredictedAchievableTemp = [this, outsideTemp, &zoneRec](int maxFlowTemp) -> int {
			if (isDHWzone()) {
				return _thermalStore->calcCurrDeliverTemp(45, float(outsideTemp), float(maxFlowTemp), float(maxFlowTemp), float(maxFlowTemp));
			} else
				return int(outsideTemp + 0.5 + (maxFlowTemp - outsideTemp) * zoneRec.autoRatio / RATIO_DIVIDER) ;
		};

		const auto max_PredictedAchievableTemp = maxPredictedAchievableTemp(_maxFlowTemp);
		//logger() << L_time << "\tmax_PredictedAchievableTemp_OK" << L_endl;

		auto calculatePreheatTime = [this, max_PredictedAchievableTemp](int currTemp_fractional, int nextTempReq) -> int {
			double minsToAdd = 0.;
			double finalDiff_fractional = (nextTempReq * 256.) - currTemp_fractional;
			double limitTemp = max_PredictedAchievableTemp;
			
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
			bool needToStart;
			bool gotDifferentProfileTemp = false;
			auto info = refreshProfile(false);
			auto preheatCallTemp = info.currTT.temp();

			if (preheatCallTemp * 256 > currTemp_fractional) {
				_minsToPreheat = calculatePreheatTime(currTemp_fractional, preheatCallTemp);// is already heating. Don't need to check for pre-heat
				if (isDHWzone()) {
					if (_minsToPreheat < 60) _minsToPreheat = -_minsToPreheat;
				}
			} else {
				int32_t minsToFutureEvent = clock_().now().minsTo(info.nextEvent);
				auto minsToNextEvent = int16_t(minsToFutureEvent);
				do {
					_minsToPreheat = calculatePreheatTime(currTemp_fractional, info.nextTT.temp());
					if (!gotDifferentProfileTemp && info.nextTT.temp() != _currProfileTempRequest) {
						_nextProfileTempRequest = info.nextTT.temp();
						gotDifferentProfileTemp = true;
					}
					if (_minsToPreheat > 0) {
						profileLogger() << "\t" << zoneRec.name << " " << info.nextTT.temp() << "deg. Preheat mins " << _minsToPreheat << " Delay: " << zoneRec.autoDelay << L_endl;
						_minsToPreheat += zoneRec.autoDelay + 5;
					}
					needToStart = minsToFutureEvent <= _minsToPreheat;
					if (needToStart) break;
					info = _sequencer->getProfileInfo(id(), info.nextEvent);
					minsToFutureEvent = clock_().now().minsTo(info.nextEvent);
				} while (minsToFutureEvent < 60 * 24);
				if (needToStart) {
					preheatCallTemp = info.nextTT.temp();
					saveThermalRatio();
					profileLogger() << zoneRec.name << " Preheat " << _minsToPreheat
						<< " mins from " << L_fixed << currTemp_fractional << L_dec << " to " << int(info.nextTT.temp())
						<< " by " << info.nextEvent
						<< L_endl;
				} else if (isDHWzone())
					if (minsToNextEvent < 60) _minsToPreheat = -minsToNextEvent;
					else _minsToPreheat = minsToNextEvent;
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
		//executionTime[1] = millis();
		//logger() << L_time << "\tLambdas_OK" << L_endl;
		profileLogger() << L_endl << L_time << zoneRec.name << " Get ProfileInfo. Curr Temp: " << L_fixed << currTemp_fractional << L_dec << " CurrStart: " << startDateTime() << L_endl;
		//executionTime[2] = millis();
		//executionTime[3] = millis()-1;
		//executionTime[4] = millis()-2;
		//executionTime[5] = millis()-3;
		//logger() << L_time << "\tprofileLog_OK" << L_endl;

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
			//executionTime[3] = millis();
			bool amLate = _preheatCallTemp == currTempRequest() ? true : false;
			DateTime targetTime;
			if (amLate) targetTime = startDateTime(); else targetTime = nextEventTime();
			auto missedTargetTime = targetTime.minsTo(clock_().now());
			if (missedTargetTime) {
				profileLogger() << "\tMissed Target Time by " << missedTargetTime << " Aiming for " << targetTime << L_endl;
				//executionTime[4] = millis();
				if (resultQuality >= zoneRec.autoQuality) {
					saveNewTimeConst(currTemp_fractional, maxPredictedAchievableTemp(_maxFlowTemp) * 256, resultQuality);
				} else {
					saveNewTimeDelay();
				}
				//executionTime[5] = millis();
			}
			cancelPreheat();
			_startCallTemp = currTemp_fractional;
		}
		//executionTime[6] = millis();
		if (newPreheatTemp > _preheatCallTemp) {
			profileLogger() << "\tStart Preheat " << zoneRec.name << L_endl;
			_startCallTemp = currTemp_fractional;
			if (_minsCooling == DELAY_COOLING_TIME) _minsCooling = MEASURING_DELAY;
		} 
		_minsToPreheat = calculatePreheatTime(currTemp_fractional, currTempRequest());
		if (isDHWzone()) {
			if (currTempRequest() < nextTempRequest()) _minsToPreheat = static_cast<int16_t>(clock_().now().minsTo(nextEventTime()));
			if (_minsToPreheat < 60) _minsToPreheat = -_minsToPreheat;
		}
		_preheatCallTemp = newPreheatTemp;
		//executionTime[7] = millis();
		//logger() << "preHeatForNextTT() Lambdas: " << int(executionTime[1] - executionTime[0]) << "\t" << L_endl;
		//logger() << "preHeatForNextTT() Heading: " << int(executionTime[2] - executionTime[1]) << "\t" << L_endl;
		//logger() << "preHeatForNextTT() FinishedLog: " << int(executionTime[3] - executionTime[2]) << "\t" << L_endl;
		//logger() << "preHeatForNextTT() MissedLog: " << int(executionTime[4] - executionTime[3]) << "\t" << L_endl;
		//logger() << "preHeatForNextTT() SaveTC: " << int(executionTime[5] - executionTime[4]) << "\t" << L_endl;
		//logger() << "preHeatForNextTT() DoneFinished: " << int(executionTime[6] - executionTime[5]) << "\t" << L_endl;
		//logger() << "preHeatForNextTT() StartHeating: " << int(executionTime[7] - executionTime[6]) << "\t" << L_endl;
	}

}