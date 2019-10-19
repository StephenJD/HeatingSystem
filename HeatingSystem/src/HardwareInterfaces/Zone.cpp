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
		_recID = zoneID;
		_maxFlowTemp = maxFlowTemp;
	}

	void Zone::offsetCurrTempRequest(uint8_t val) {
		_offsetT = (val - _currProfileTempRequest);
	}

	bool Zone::isDHWzone() const {
		return (_relay->recID() == R_Gas);
	}

	uint8_t Zone::getCurrTemp() const { return _callTS ? _callTS->get_temp() : 0; }

	int16_t Zone::getFractionalCallSensTemp() const {
		if (isDHWzone()) {
			return _thermalStore->currDeliverTemp() << 8;
		}
		else return _callTS->get_fractional_temp();
	}

	uint8_t Zone::modifiedCallTemp(uint8_t callTemp) const {
		callTemp += _offsetT;
		if (callTemp <= MIN_ON_TEMP) return callTemp;
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

	bool Zone::setFlowTemp() { // Called every second. sets flow temps. Returns true if needs heat.
		if (isDHWzone()) {
			bool needHeat;
			if (I2C_Temp_Sensor::hasError()) {
				needHeat = false;
				//logger().log("Zone_Run::setZFlowTemp for DHW\tTS Error.");
			}
			else {
				needHeat = _thermalStore->needHeat(currTempRequest() + _offsetT, nextTempRequest() + _offsetT);
				//logger().log("Zone_Run::setZFlowTemp for DHW\t NeedHeat?",needHeat);
			}
			_relay->setRelay(needHeat);
			return needHeat;
		}

		auto fractionalZoneTemp = getFractionalCallSensTemp(); // get fractional temp.msb is temp is degrees
		if (I2C_Temp_Sensor::hasError()) {
			//if (i2C->getThisI2CFrequency(lcd()->i2cAddress()) == 0) {
			  //logger().log("Zone_Run::setZFlowTemp\tCall TS Error for zone", epD().index());
			//}
			return false;
		}

		long myFlowTemp = 0;
		//auto fractionalOutsideTemp = f->thermStoreR().getFractionalOutsideTemp(); // msb is temp is degrees
		auto currTempReq = modifiedCallTemp(_currProfileTempRequest);
		long tempError = (fractionalZoneTemp - (currTempReq << 8)) / 16; // 1/16 degrees +ve = Temp too high
		if (_thermalStore->dumpHeat()) {// turn zone on to dump heat
			tempError = -10;
		}
		// Integrate tempError and FlowTemp every second over AVERAGING_PERIOD
		//_aveError += tempError;
		//_aveFlow += getFlowSensTemp();
//		if (z_period <= 0) { // Adjust everything at end of AVERAGING_PERIOD. Decremented by ThrmSt_Run::check()
//#if defined (ZPSIM)
//			bool debug;
//			switch (_recID) {
//			case Z_DownStairs:// downstairs
//				debug = true;
//				break;
//			case Z_UpStairs: // upstairs
//				debug = true;
//				break;
//			case Z_Flat: // flat
//				debug = true;
//				break;
//			case Z_DHW: // DHW
//				debug = true;
//				break;
//			}
//#endif

			//float aveFractionalZoneTemp;
			//long aveFractionalFlowTemp;
			//aveFractionalZoneTemp = (float)_aveError*16.0F / AVERAGING_PERIOD + currTempReq * 256; // _aveError is in 1/16 degrees. AveError and _period zeroed when temp request changed.
			//aveFractionalFlowTemp = (long) 0.5F + (_aveFlow / AVERAGING_PERIOD) * 256;
			//_aveError = 0;
			//_aveFlow = 0;

			// Recalc _tempRatio
			//if (aveFractionalZoneTemp - fractionalOutsideTemp > 1) _tempRatio = U1_byte(0.5F + (aveFractionalFlowTemp - aveFractionalZoneTemp) * 10 / (aveFractionalZoneTemp - fractionalOutsideTemp));
			//if (_tempRatio < 5) _tempRatio = 5;
			//if (_tempRatio > 40) _tempRatio = 40;

			//S2_byte myBoostFlowLimit, myTheoreticalFlow;

			//do {
			//	myBoostFlowLimit = S2_byte(aveFractionalFlowTemp / 256 + (getVal(zWarmBoost) * _tempRatio) / 10);
			//	S2_byte room_flow_diff = currTempReq - ((fractionalOutsideTemp + 128) >> 8);
			//	myTheoreticalFlow = ((_tempRatio * room_flow_diff) + 5) / 10 + currTempReq; // flow for required temperature

			//} while (tempError < 0 && myTheoreticalFlow < MIN_FLOW_TEMP && ++_tempRatio);
			
			// calc required flow temp due to current temp error
			//long flowBoostDueToError = (-tempError * _tempRatio);
			if (tempError > 7L) {
				myFlowTemp = MIN_FLOW_TEMP;
				// logger().log("Zone_Run::setZFlowTemp\tToo Cool: FlowTemp, MaxFlow,TempError, Zone:",(int)epDI().index(),myFlowTemp, myMaxFlowTemp,0,0,0,0,tempError * 16);
			}
			else if (tempError < -8L) {
				myFlowTemp = _maxFlowTemp;
				logger().log("Zone_Run::setZFlowTemp\tToo Cool: ReqFlowTemp, MaxFlow,TempError, Zone:", _recID, myFlowTemp, _maxFlowTemp, 0, 0, 0, 0, tempError * 16);
			}
			else {
				myFlowTemp = static_cast<long>((_maxFlowTemp + MIN_FLOW_TEMP) / 2. - tempError * (_maxFlowTemp - MIN_FLOW_TEMP) / 16.);
				//U1_byte errorDivider = flowBoostDueToError > 16 ? 10 : 40; //40; 
				//myFlowTemp = myTheoreticalFlow + (flowBoostDueToError + errorDivider/2) / errorDivider; // rounded to nearest degree
				logger().log("Zone_Run::setZFlowTemp\tIn Range ReqFlowTemp, MaxFlow,TempError, Zone:", _recID, myFlowTemp, _maxFlowTemp, 0, 0, 0, 0, tempError * 16);
			}

			// check limits
			if (myFlowTemp > _maxFlowTemp) myFlowTemp = _maxFlowTemp;
			//else if (myFlowTemp > myBoostFlowLimit) myFlowTemp = myBoostFlowLimit;
			else if (myFlowTemp < MIN_FLOW_TEMP) { myFlowTemp = MIN_FLOW_TEMP; tempError = 0; }

			if (_mixValveController->amControlZone(uint8_t(myFlowTemp), _maxFlowTemp, _relay->recID())) { // I am controlling zone, so set flow temp
			}
			else { // not control zone
				_relay->setRelay(tempError < 0); // too cool
			}
		//}
		//else {// keep same flow temp and reported tempError until AVERAGING_PERIOD is up to stop rapid relay switching.	
		//	myFlowTemp = _callFlowTemp;
		//	tempError = _relay->getRelayState() ? -1 : 0;
		//}


		_callFlowTemp = (uint8_t)myFlowTemp;
		return (tempError < 0);
	}

}