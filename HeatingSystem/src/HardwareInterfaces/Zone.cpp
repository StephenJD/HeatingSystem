#include "Zone.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "..\Client_DataStructures\Data_Zone.h"
#include "ThermalStore.h"
#include "MixValveController.h"
#include "RDB.h"

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

	auto Zone::zoneRecord() -> RelationalDatabase::Answer_R<client_data_structures::R_Zone> {
		using namespace RelationalDatabase;
		using namespace client_data_structures;
		auto zones = _db->tableQuery(TB_Zone);
		return zones[record()];
	}

	void Zone::preHeatForNextTT() { // must be called once every 10 mins to record temp changes.
		// Modifies _currProfileTempRequest to allow for heat-up for next event
		// returns true when actual programmed time has expired.
		using namespace Date_Time;
		auto zoneAnswer = zoneRecord();
		auto & zoneRec = zoneAnswer.rec();
		auto currTempReq = currTempRequest();
		auto nextTempReq = nextTempRequest();

		auto currTemp_fractional = getFractionalCallSensTemp();
		//double outsideDiff;
		//if (isDHWzone()) {
		//	outsideDiff = 0;
		//	if (currTemp_fractional / 256 < nextTempReq) nextTempReq += THERM_STORE_HYSTERESIS;
		//} else {
		//	outsideDiff = nextTempReq - f->thermStoreR().getOutsideTemp();
		//}
		// Part 1: When nextTempReq is > currTempReq:
		//	1. Calculate time to warm up.
		//	2. Calculate the pre-heat time from _ttEndDateTime.
		//  3. If now is after pre-heat time and CurrTempReq is not NextTempReq:
		//		4. Set CurrTempReq to the NextTempReq. This will start heating and prevent further pre-heat calculations.
		//		5. Register first Curve Temp.
		// Part 2: When CurrTemp is <= currTempReq:
		//	1. Record next temp point.
		// Part 3: When temp reaches required:
		//	1. Attempt curve-match
		//	2. If successful, save new constants.

		if (nextTempReq > currTempReq) { // if next profile temp is higher allow warm-up time
			int minsToAdd = 0;
			double finalDiff_fractional = (nextTempReq * 256.) - currTemp_fractional; //- 128; // aim for 0.5 degree below requested temp
			double limitTemp = zoneRec.autoFinalT; // 25 or 60 finalDiff_fractional +  * 25.6 * (1. - outsideDiff/25.) ;
			if (limitTemp < nextTempReq + 2.) limitTemp = nextTempReq + 2.;
			double limitDiff_fractional = limitTemp * 256. - currTemp_fractional;
			auto timeConst = _getExpCurve.uncompressTC(zoneRec.autoTimeC);
			if (zoneRec.autoMode) {
				double logRatio = 1. - (finalDiff_fractional / limitDiff_fractional);
				minsToAdd = int(-timeConst * log(logRatio));
			} else {
				minsToAdd = int(finalDiff_fractional * zoneRec.manHeatTime * 2 + 128) / 256; // mins per 1/2 degree
			}
			auto preheatTime = _ttEndDateTime;
			/*if (minsToAdd > 0)*/ preheatTime.addOffset({ hh,-(minsToAdd / 60) }).addOffset({ m10,-(minsToAdd % 60) / 10 });
			logger() << L_time << "Zone Preheat: " << zoneRec.name 
				<< " TempIs: " << currTemp_fractional/256 << " NextReq: " << nextTempReq 
				<< " HeatMins is : " << minsToAdd << " Start at: " << preheatTime << L_endl;
			if (clock_().now() >= preheatTime) {
				setProfileTempRequest(nextTempReq - offset());
				_getExpCurve.firstValue(currTemp_fractional);
				logger() << "\tFirst Temp Registered. AutoLimit: " << (long)limitTemp << " TConst: " << (long)timeConst << L_endl;
			}
		} else { // We are in pre-heat or nextReq lower than CurrReq
			_getExpCurve.nextValue(currTemp_fractional); // Only registers if _getExpCurve.firstValue was non-zero
			if (currTemp_fractional >= (currTempReq * 256)) {
				logger() << L_time << "Zone Preheat OK. Curve: " << zoneRec.name;
				GetExpCurveConsts::CurveConsts curveMatch = _getExpCurve.matchCurve();
				if (curveMatch.resultOK) { // within 0.5 degrees and enough temps recorded. recalc TL
					logger() << " New Limit: " << long(curveMatch.limit)
						<< " New TC: " << long(curveMatch.timeConst) << L_endl;
					zoneRec.autoFinalT = (curveMatch.limit + 128) / 256;
					auto timeConst = _getExpCurve.compressTC(curveMatch.timeConst);
					zoneRec.autoTimeC = timeConst > 255 ? 255 : uint8_t(timeConst);
					zoneAnswer.update();
				} else logger() << " No Curve Result\n";
				_getExpCurve.firstValue(0); // prevent further updates
			}
		}
	}
}