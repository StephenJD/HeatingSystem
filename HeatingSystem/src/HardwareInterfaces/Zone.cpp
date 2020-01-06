#include "Zone.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include "Temp_Sensor.h"
#include "ThermalStore.h"
#include "Relay.h"
#include "MixValveController.h"

namespace HardwareInterfaces {

	using namespace Assembly;
	//***************************************************
	//              Zone Dynamic Class
	//***************************************************
#ifdef ZPSIM
	Zone::Zone(I2C_Temp_Sensor & ts, int reqTemp)
		: _callTS(&ts)
		, _currProfileTempRequest(reqTemp)
		, _isHeating(reqTemp > _callTS->get_temp() ? true : false)
		, _maxFlowTemp(65)
	{}
#endif
	void Zone::initialise(int zoneID, I2C_Temp_Sensor & callTS, Relay & callRelay, ThermalStore & thermalStore, MixValveController & mixValveController, int8_t maxFlowTemp) {
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
		return _relay->getRelayState();
	}

	int8_t Zone::getCurrTemp() const { return _callTS ? (getFractionalCallSensTemp() >> 8) : 0; }

	int16_t Zone::getFractionalCallSensTemp() const {
		if (isDHWzone()) {
			return _thermalStore->currDeliverTemp() << 8;
		}
		else return _callTS->get_fractional_temp();
	}

	int8_t Zone::modifiedCallTemp(int8_t callTemp) const {
		callTemp += _offsetT;
		if (callTemp == 246) {
			logger() << L_time << "Zone::modifiedCallTemp. callTemp: " << callTemp << " Offset: " << _offsetT << L_endl;
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
		if (isDHWzone()) {
			bool needHeat;
			if (I2C_Temp_Sensor::hasError()) {
				needHeat = false;
				//logger() << "Zone_Run::setZFlowTemp for DHW\tTS Error.");
			}
			else {
				needHeat = _thermalStore->needHeat(currTempRequest(), nextTempRequest());
				//logger() << "Zone_Run::setZFlowTemp for DHW\t NeedHeat?",needHeat);
			}
			_relay->setRelay(needHeat);
			return needHeat;
		}

		auto fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp.msb is temp is degrees
		if (I2C_Temp_Sensor::hasError()) {
			logger() << L_endl << L_time << "Zone_Run::setZFlowTemp\tCall TS Error for zone" << _recordID << L_endl;
			return false;
		}

		long myFlowTemp = 0;
		//auto fractionalOutsideTemp = f->thermStoreR().getFractionalOutsideTemp(); // msb is temp is degrees
		long currTempReq = modifiedCallTemp(_currProfileTempRequest);
		long tempError = (fractionalZoneTemp - (currTempReq << 8)) / 16; // 1/16 degrees +ve = Temp too high
		if (_thermalStore->dumpHeat()) {// turn zone on to dump heat
			tempError = -10;
		}
		auto logger_RelayStatus = _relay->getRelayState();
		if (tempError > 7L) {
			myFlowTemp = MIN_FLOW_TEMP;
			logger() << L_endl << L_time << "Zone_Run::setZFlowTemp\tToo Warm. Zone: " << _recordID
				<< " Req Temp: " << currTempReq
				<< " fractionalZoneTemp: " << fractionalZoneTemp / 256.
				<< " ReqFlowTemp: " << myFlowTemp
				<< " TempError: " << L_fixed << tempError * 16 << (logger_RelayStatus? " On":" Off") << L_flush;
		}
		else if (tempError < -8L) {
			myFlowTemp = _maxFlowTemp;
			logger() << L_endl << L_time << "Zone_Run::setZFlowTemp\tToo Cool. Zone: " << _recordID
				<< " Req Temp: " << currTempReq
				<< " fractionalZoneTemp: " << fractionalZoneTemp / 256.
				<< " ReqFlowTemp: " << myFlowTemp
				<< " TempError: " << L_fixed << tempError * 16 << (logger_RelayStatus ? " On" : " Off") << L_flush;
		}
		else {
			myFlowTemp = static_cast<long>((_maxFlowTemp + MIN_FLOW_TEMP) / 2. - tempError * (_maxFlowTemp - MIN_FLOW_TEMP) / 16.);
			//U1_byte errorDivider = flowBoostDueToError > 16 ? 10 : 40; //40; 
			//myFlowTemp = myTheoreticalFlow + (flowBoostDueToError + errorDivider/2) / errorDivider; // rounded to nearest degree
			logger() << L_endl << L_time << "Zone_Run::setZFlowTemp\tIn Range. Zone: " << _recordID
				<< " Req Temp: " << currTempReq
				<< " fractionalZoneTemp: " << fractionalZoneTemp / 256.
				<< " ReqFlowTemp: " << myFlowTemp
				<< " TempError: " << L_fixed << tempError * 16 << (logger_RelayStatus ? " On" : " Off") << L_flush;
		}

		// check limits
		if (myFlowTemp > _maxFlowTemp) myFlowTemp = _maxFlowTemp;
		//else if (myFlowTemp > myBoostFlowLimit) myFlowTemp = myBoostFlowLimit;
		else if (myFlowTemp < MIN_FLOW_TEMP) { myFlowTemp = MIN_FLOW_TEMP; tempError = 0; }

		if (_mixValveController->amControlZone(uint8_t(myFlowTemp), _maxFlowTemp, _relay->recordID())) { // I am controlling zone, so set flow temp
		}
		else { // not control zone
			_relay->setRelay(tempError < 0); // too cool
		}

		_callFlowTemp = (int8_t)myFlowTemp;
		return (tempError < 0);
	}

}