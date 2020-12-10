#include "Zone.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "ThermalStore.h"
#include "MixValveController.h"
#include "RDB.h"
#include <Clock.h>

Logger& zTempLogger();

namespace HardwareInterfaces {

	using namespace Assembly;
	using namespace GP_LIB;
	//***************************************************
	//              Zone Dynamic Class
	//***************************************************
#ifdef ZPSIM
	Zone::Zone(UI_TempSensor & ts, int reqTemp, UI_Bitwise_Relay & callRelay )
		: _callTS(&ts)
		, _currProfileTempRequest(reqTemp)
		, _relay(&callRelay)
		, _isHeating(reqTemp > _callTS->get_temp() ? true : false)
		, _maxFlowTemp(65)
	{}
#endif
	void Zone::initialise(int zoneID, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController, int8_t maxFlowTemp, RelationalDatabase::RDB<TB_NoOfTables> & db) {
		_callTS = &callTS;
		_mixValveController = &mixValveController;
		_relay = &callRelay;
		_thermalStore = &thermalStore;
		_recordID = zoneID;
		_maxFlowTemp = maxFlowTemp;
		_db = &db;
		using namespace RelationalDatabase;
		using namespace client_data_structures;
		auto zones = _db->tableQuery(TB_Zone);
		_zoneRecord = zones[_recordID];
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

	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		// lambdas
		auto zoneNames = [](int id) -> const char * {
			switch (id) {
			case 0: return "US  ";
			case 1: return "DS  ";
			case 2: return "DHW ";
			case 3: return "Flat";
			default: return "";
			}
		};

		auto logTemps = [this, zoneNames](const __FlashStringHelper * msg, long currTempReq, short fractionalZoneTemp, long myFlowTemp, long tempError, bool logger_RelayStatus) {
			zTempLogger() << L_time << L_tabs << F("Zone::setZFlowTemp") << zoneNames(_recordID) << msg 
			<< F("Req Temp:") << currTempReq
			<< F("ZoneTemp:") << fractionalZoneTemp / 256.
			<< F("ReqFlowTemp:") << myFlowTemp
			<< F("TempError:") << tempError / 16. << (logger_RelayStatus ? F(" On") : F(" Off")) << L_endl;
		};
		
		if (isDHWzone()) {
			bool needHeat;
			needHeat = _thermalStore->needHeat(currTempRequest(), nextTempRequest());
			//logger() << F("Zone::setZFlowTemp for DHW\t NeedHeat?") << needHeat << L_endl;
			_relay->set(needHeat);
			return needHeat;
		}

		auto fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp.msb is temp is degrees
		if (_callTS->hasError()) {
			logger() << L_endl << L_time << F("Zone::setZFlowTemp\tCall TS Error for zone ") << zoneNames(_recordID) << L_endl;
			return false;
		}

		long myFlowTemp = 0;
		//auto fractionalOutsideTemp = f->thermStoreR().getFractionalOutsideTemp(); // msb is temp is degrees
		long currTempReq = modifiedCallTemp(_currProfileTempRequest);
		long tempError = (fractionalZoneTemp - (currTempReq << 8)) / 16; // 1/16 degrees +ve = Temp too high
		if (_thermalStore->dumpHeat()) {// turn zone on to dump heat
			tempError = -10;
		}
		auto logger_RelayStatus = _relay->logicalState();
		if (tempError > 7L) {
			myFlowTemp = MIN_FLOW_TEMP;
			logTemps(F("Too Warm."), currTempReq, fractionalZoneTemp, myFlowTemp, tempError, logger_RelayStatus);
		}
		else if (tempError < -8L) {
			myFlowTemp = _maxFlowTemp;
			logTemps(F("Too Cool."), currTempReq, fractionalZoneTemp, myFlowTemp, tempError, logger_RelayStatus);
		}
		else {
			myFlowTemp = static_cast<long>((_maxFlowTemp + MIN_FLOW_TEMP) / 2. - tempError * (_maxFlowTemp - MIN_FLOW_TEMP) / 16.);
			//U1_byte errorDivider = flowBoostDueToError > 16 ? 10 : 40; //40; 
			//myFlowTemp = myTheoreticalFlow + (flowBoostDueToError + errorDivider/2) / errorDivider; // rounded to nearest degree
			logTemps(F("In Range."), currTempReq, fractionalZoneTemp, myFlowTemp, tempError, logger_RelayStatus);
		}

		// check limits
		if (myFlowTemp > _maxFlowTemp) myFlowTemp = _maxFlowTemp;
		//else if (myFlowTemp > myBoostFlowLimit) myFlowTemp = myBoostFlowLimit;
		else if (myFlowTemp < MIN_FLOW_TEMP) { myFlowTemp = MIN_FLOW_TEMP; tempError = 0; }

		if (_mixValveController->amControlZone(uint8_t(myFlowTemp), _maxFlowTemp, _relay->recordID())) { // I am controlling zone, so set flow temp
		}
		else { // not control zone
			_relay->set(tempError < 0); // too cool
		}

		_callFlowTemp = (int8_t)myFlowTemp;
		return (tempError < 0);
	}

	auto Zone::zoneRecord() -> RelationalDatabase::Answer_R<client_data_structures::R_Zone> & {
		return _zoneRecord;
	}

	void Zone::preHeatForNextTT() { // must be called once every 10 mins to record temp changes.
		// Modifies _currProfileTempRequest to allow for heat-up for next event
		// returns true when actual programmed time has expired.

		using namespace Date_Time;
		auto & zoneAnswer = zoneRecord();
		auto & zoneRec = zoneAnswer.rec();
		auto currTempReq = currTempRequest();
		auto nextTempReq = nextTempRequest();
		auto currTemp_fractional = getFractionalCallSensTemp();
		auto outsideTemp = isDHWzone() ? uint8_t(20) : _thermalStore->getOutsideTemp();
		
		//if (clock_().now().asInt() == 0 && zoneRec.autoQuality > 1) { // reduce quality by 10% each day.
		//	zoneRec.autoQuality -= uint8_t(zoneRec.autoQuality * 0.1 + .8);
		//	zoneAnswer.update();
		//	logger() << "autoQuality reduced to " << zoneRec.autoQuality << " for " << zoneRec.name;
		//}

		// Lambdas
		auto maxPredictedAchievableTemp = [this, outsideTemp, &zoneRec]() -> double {
			/* Ratio is the thermal-resistance ratio of the heat-loss/heat-input.
			* The achievable room temperature differential is the resitance-ratio * input-outside differential
			*/
			return (_maxFlowTemp - outsideTemp) * zoneRec.autoRatio / 256. + outsideTemp;
		};

		auto ratioIsTooLow = [&zoneRec, nextTempReq](double limitTemp)->bool {
			return zoneRec.autoRatio != 0 && limitTemp < nextTempReq + 1;
		};

		auto getTimeConst = [&zoneRec, this]()->double {
			if (zoneRec.autoQuality == 0) {
				return _getExpCurve.uncompressTC(zoneRec.manHeatTime);
			} else {
				return _getExpCurve.uncompressTC(zoneRec.autoTimeC);
			}
		};

		auto usingLinearCurve = [&zoneRec]() {return zoneRec.autoQuality == 0 || zoneRec.autoRatio == 0; };

		auto calculatePreheatTime = [this, nextTempReq, currTemp_fractional, outsideTemp, &zoneRec
			, maxPredictedAchievableTemp, ratioIsTooLow, getTimeConst, usingLinearCurve]() -> int {
			int minsToAdd = 0;
			double finalDiff_fractional = (nextTempReq * 256.) - currTemp_fractional;
			double limitTemp = maxPredictedAchievableTemp();
			if (ratioIsTooLow(limitTemp)) {
				logger() << zoneRec.name << " ********  MAX FLOW TEMP IS TOO LOW ********* Ratio: "
					<< L_fixed << zoneRec.autoRatio << L_int << " Limit: " << limitTemp << " OS temp: " << outsideTemp << " Max: " << _maxFlowTemp << L_endl;
			} else {
				double limitDiff_fractional = limitTemp * 256. - currTemp_fractional;
				double timeConst = getTimeConst();
				if (usingLinearCurve()) {
					minsToAdd = static_cast<int>(timeConst * finalDiff_fractional / 4 / 256); // timeConst is mins per 4 deg.
				} else {
					double logRatio = 1. - (finalDiff_fractional / limitDiff_fractional);
					minsToAdd = int(-timeConst * log(logRatio));
				}
			}
			return minsToAdd;
		};
		
		auto timeToStartHeating = [this](int minsToAdd) -> bool {return clock_().now().minsTo(_ttEndDateTime) <= minsToAdd; };
		
		auto offerCurrTempToCurveMatch = [this, currTemp_fractional, &zoneRec]() {
			if (_getExpCurve.nextValue(currTemp_fractional)) { // Only registers if _getExpCurve.firstValue was non-zero
				logger() << L_time << zoneRec.name << " Preheat. Record Temp:\t" << L_fixed << currTemp_fractional << L_endl;
			}
		};

		auto reachedAcceptableTemp = [currTemp_fractional, currTempReq]() -> bool { return currTemp_fractional + 128 >= currTempReq * 256; };
		auto canUpdatePreheatParameters = [&zoneRec](int quality)->bool {return zoneRec.autoQuality > 0 && quality > 0; };
		
		auto saveNewPreheatParameters = [this, &zoneRec, &zoneAnswer, outsideTemp, currTempReq](GetExpCurveConsts::CurveConsts curveMatch, int quality) {
			logger() << " New Limit: " << L_fixed << long(curveMatch.limit)
			<< " New TC: " << L_int << long(curveMatch.timeConst)
			<< " New Quality: " << L_int << quality << " Old Quality: " << zoneRec.autoQuality << " OS Temp: " << outsideTemp << L_endl;

			double limit = (curveMatch.limit + 128.) / 256.;
			if (limit > _maxFlowTemp || limit < currTempReq) { // treat as linear curve. 			
				limit = outsideTemp; // Make ratio = 0 to indicate linear curve, and TC = minutes-per-4-degrees
				curveMatch.timeConst = static_cast<uint16_t>(curveMatch.period * 4L * 256L / curveMatch.range);
			}
			auto timeConst = _getExpCurve.compressTC(curveMatch.timeConst);
			double newRatio = (limit - outsideTemp) / (_maxFlowTemp - outsideTemp);
			zoneRec.autoRatio = static_cast<uint8_t>(newRatio * 256);
			zoneRec.autoTimeC = timeConst;
			zoneRec.autoQuality = quality;
			zoneAnswer.update();
			logger() << "\tSaved New TC " << curveMatch.timeConst << " Compressed TC: " << timeConst << " New Ratio: " << newRatio << L_endl;
		};

		// Algorithm
		/*
		 If the next profile temp is higher than the current, then calculate the pre-heat time.
		 If the current time is >= the pre-heat time, change the currTempRequest to next profile-temp and start recording temps / times.
		 When the actual temp reaches the required temp, stop recording and do a curve-match.
		 If the temp-range of the curve-match is >= to the preious best, then save the new curve parameters.
		*/
		if (nextTempReq > currTempReq) {
			int minsToAdd = calculatePreheatTime();
			if (minsToAdd < 10) minsToAdd = 10; // guaruntee we always start recording a pre-heat
			if (timeToStartHeating(minsToAdd)) {
				setProfileTempRequest(nextTempReq - offset());
				_getExpCurve.firstValue(currTemp_fractional);
				logger() << "\tFirst Temp Registered for " << zoneRec.name << " temp is: " << L_fixed << currTemp_fractional << L_endl;
				logger() << "\t\tPreheatTime: " << minsToAdd << " Compressed TC: " << zoneRec.autoTimeC << " Ratio: " << zoneRec.autoRatio << " OS Temp: " << outsideTemp << L_endl;
			}
		} else { // We are in pre-heat or nextReq lower than CurrReq
			offerCurrTempToCurveMatch();
			if (reachedAcceptableTemp()) {
				GetExpCurveConsts::CurveConsts curveMatch = _getExpCurve.matchCurve();
				auto resultQuality = (curveMatch.range + 12) / 25; // round up
				if (canUpdatePreheatParameters(resultQuality)) {
					logger() << L_time << zoneRec.name << " Preheat OK. Req " << currTempReq << " is: " << L_fixed  << currTemp_fractional << L_dec << " Range: " << resultQuality << L_endl;
					if (resultQuality >= zoneRec.autoQuality) {
						saveNewPreheatParameters(curveMatch, resultQuality);
					}
				}
				_getExpCurve.firstValue(0); // prevent further updates
			}
		}
	}
}