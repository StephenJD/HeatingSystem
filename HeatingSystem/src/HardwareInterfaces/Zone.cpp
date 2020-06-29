#include "Zone.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include "ThermalStore.h"
#include "MixValveController.h"

namespace HardwareInterfaces {

	using namespace Assembly;
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
	void Zone::initialise(int zoneID, UI_TempSensor & callTS, UI_Bitwise_Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController, int8_t maxFlowTemp) {
		_callTS = &callTS;
		_mixValveController = &mixValveController;
		_relay = &callRelay;
		_thermalStore = &thermalStore;
		_recordID = zoneID;
		_maxFlowTemp = maxFlowTemp;
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

	int8_t Zone::getCurrTemp() const { return _callTS ? (getFractionalCallSensTemp() >> 8) : 0; }

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
		if (callTemp == 246) {
			logger() << L_time << F("Zone::modifiedCallTemp. callTemp: ") << callTemp << F(" Offset: ") << _offsetT << L_endl;
		}		
		if (callTemp < MIN_ON_TEMP) callTemp = MIN_ON_TEMP;
		//int8_t modifiedTemp;
		//int8_t weatherSetback = epDI().getWeatherSetback();
		//// if Outside temp + WeatherSB > CallTemp then set to 2xCallT-OSTemp-WeatherSB, else set to CallT
		//int8_t outsideTemp = f->thermStoreR().getOutsideTemp();
		//if (outsideTemp + weatherSetback > callTemp) {
		//	modifiedTemp = 2 * callTemp - outsideTemp - weatherSetback;
		//	if (modifiedTemp < (getFractionalCallSensTemp() >> 8) && modifiedTemp < callTemp) modifiedTemp = callTemp - 1;
		//	callTemp = modifiedTemp;
		//}

		return callTemp;
	}

	int8_t Zone::maxUserRequestTemp() const {
		if (getCurrTemp() < _currProfileTempRequest) return _currProfileTempRequest;
		else return getCurrTemp() + 1;
	}

	bool Zone::setFlowTemp() { // Called every minute. Sets flow temps. Returns true if needs heat.
		// lambdas
		auto logTemps = [this](const __FlashStringHelper * msg, long currTempReq, short fractionalZoneTemp, long myFlowTemp, long tempError, bool logger_RelayStatus) {
			logger() << L_endl << L_time << F("Zone_Run::setZFlowTemp\t") << msg << F(" Zone: ") << _recordID
			<< F(" Req Temp: ") << currTempReq
			<< F(" fractionalZoneTemp: ") << fractionalZoneTemp / 256.
			//<< fractionalZoneTemp
			//<< F("\n\t dbl-fractionalZoneTemp: ") << double(fractionalZoneTemp)
			//<< F("\n\t dbl-fractionalZoneTemp/256: ") 	
			<< F(" ReqFlowTemp: ") << myFlowTemp
			<< F(" TempError: ") << tempError / 16. << (logger_RelayStatus ? F(" On") : F(" Off")) << L_endl;
		};
		
		if (isDHWzone()) {
			bool needHeat;
			if (UI_TempSensor::hasError()) {
				needHeat = false;
				//logger() << F("Zone_Run::setZFlowTemp for DHW\tTS Error.\n");
			}
			else {
				needHeat = _thermalStore->needHeat(currTempRequest(), nextTempRequest());
				//logger() << F("Zone_Run::setZFlowTemp for DHW\t NeedHeat?") << needHeat << L_endl;
			}
			_relay->set(needHeat);
			return needHeat;
		}

		auto fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp.msb is temp is degrees
		if (UI_TempSensor::hasError()) {
			logger() << L_endl << L_time << F("Zone_Run::setZFlowTemp\tCall TS Error for zone") << _recordID << L_endl;
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
			logTemps(F("Too Warm"), currTempReq, fractionalZoneTemp, myFlowTemp, tempError, logger_RelayStatus);
		}
		else if (tempError < -8L) {
			myFlowTemp = _maxFlowTemp;
			logTemps(F("Too Cool"), currTempReq, fractionalZoneTemp, myFlowTemp, tempError, logger_RelayStatus);
		}
		else {
			myFlowTemp = static_cast<long>((_maxFlowTemp + MIN_FLOW_TEMP) / 2. - tempError * (_maxFlowTemp - MIN_FLOW_TEMP) / 16.);
			//U1_byte errorDivider = flowBoostDueToError > 16 ? 10 : 40; //40; 
			//myFlowTemp = myTheoreticalFlow + (flowBoostDueToError + errorDivider/2) / errorDivider; // rounded to nearest degree
			logTemps(F("In Range"), currTempReq, fractionalZoneTemp, myFlowTemp, tempError, logger_RelayStatus);
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

}