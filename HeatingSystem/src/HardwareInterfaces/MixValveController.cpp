#include "MixValveController.h"
#include "Relay.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"

namespace HardwareInterfaces {
	using namespace Assembly;

//#if defined (ZPSIM)
//	using namespace std;
//	ofstream MixValveController::lf("LogFile.csv");
//
//#endif

	void MixValveController::initialise(int index, int addr, Relay * relayArr, I2C_Temp_Sensor * tempSensorArr, int flowTempSens, int storeTempSens) {
		setAddress(addr);
		_index = index;
		_relayArr = relayArr;
		_tempSensorArr = tempSensorArr;
		_flowTempSens = flowTempSens;
		_storeTempSens = storeTempSens;
	}


	uint8_t MixValveController::testDevice(I2C_Helper & i2c, int addr) {
		unsigned long waitTime = 3000UL + *_timeOfReset_mS;
		//Serial.println("MixValveController.testDevice");
		uint8_t hasFailed;
		uint8_t dataBuffa[1] = { 55 };
		do hasFailed = _i2C->write_verify(_address, 7, 1, dataBuffa); // write request temp
		while (hasFailed && millis() < waitTime);
		if (hasFailed)logger().log(" MixValveController::testDevice failed. Addr:", _address);
		return hasFailed;
	}


	//////////////////////////////////////////////////////////////////////
	// Construction/Destruction
	//////////////////////////////////////////////////////////////////////

	uint8_t MixValveController::getStoreTemp() const {
		return _tempSensorArr[_storeTempSens].get_temp();
	}
//
//#if defined (ZPSIM)
//	void MixValveController::reportMixStatus() const {
//		//lf << "Control Relay:\t" << (int)_controlZoneRelay << "\tCallT:\t" << (int)_mixCallTemp;
//		//lf << "\tFlowT:\t" << (int)f->tempSensorR(_controlFlowSensor).getSensTemp();
//		//lf << "\tValvePos:\t" << (int)_valvePos << "\tOn_Time:\t" << (int)_onTime << "\tOnTime_Ratio\t" << (int)_onTimeRatio;
//		//lf << "\tState:\t" << (int)_state << "\tMotor:\t" << (int)motorState << endl;
//	}
//#endif

	uint8_t MixValveController::sendSetup() const {
		uint8_t errCode;
		errCode = writeToValve(Mix_Valve::flow_temp, _tempSensorArr[_flowTempSens].get_temp());
		errCode |= writeToValve(Mix_Valve::request_temp, _mixCallTemp);
		errCode |= writeToValve(Mix_Valve::ratio, 10);
		errCode |= writeToValve(Mix_Valve::temp_i2c_addr, _tempSensorArr[_flowTempSens].getAddress());
		errCode |= writeToValve(Mix_Valve::max_ontime, VALVE_FULL_TRANSIT_TIME);
		errCode |= writeToValve(Mix_Valve::wait_time, VALVE_WAIT_TIME);
		//errCode |= writeToValve(Mix_Valve::max_flow_temp, VALVE_WAIT_TIME);
		return errCode;
	}

	bool MixValveController::check() {
		writeToValve(Mix_Valve::flow_temp, _tempSensorArr[_flowTempSens].get_temp());
		return true;
	}

	const char * relayName(int id) { // only for error reporting in next function
		switch (id) {
		case R_Flat: return "Flat";
		case R_FlTR: return "Fl-TR";
		case R_HsTR: return "House-TR";
		case R_UpSt: return "US";
		case R_DnSt: return "DS";
		default: return "";
		}
	}

	bool MixValveController::amControlZone(uint8_t callTemp, uint8_t maxTemp, uint8_t relayID) { // highest callflow temp
		if (callTemp > MIN_FLOW_TEMP && maxTemp < _limitTemp)
			_limitTemp = maxTemp;
		if (_controlZoneRelay == relayID || callTemp > _mixCallTemp) {
#if defined (ZPSIM)
			bool debug;
			if (relayID == 3)
				debug = true;
			switch (_index) {
			case 0: {// upstairs
				debug = true;
				if (readFromValve(Mix_Valve::flow_temp) >= _mixCallTemp)
					bool debug = true;
				break; }
			case 1: // downstairs
				debug = true;
				break;
			}
#endif
			if (_controlZoneRelay != relayID) { // new control zone
				writeToValve(Mix_Valve::control, Mix_Valve::e_stop_and_wait); // trigger stop and wait on valve
				writeToValve(Mix_Valve::max_flow_temp, maxTemp);
				logger().log("MixValveController::amControlZone\t New CZ - Write new MaxTemp: ", maxTemp);
			}
			_controlZoneRelay = relayID;
			if (callTemp <= MIN_FLOW_TEMP) {
				callTemp = MIN_FLOW_TEMP;
				_relayArr[_controlZoneRelay].setRelay(0); // turn call relay OFF
				_limitTemp = 100; // reset since it might have been the limiting zone
			}
			else {
				_relayArr[_controlZoneRelay].setRelay(1); // turn call relay ON
				if (callTemp > _limitTemp)
					callTemp = _limitTemp;
			}
			uint8_t mixValveCallTemp = readFromValve(Mix_Valve::request_temp);
			if (_mixCallTemp != callTemp || _mixCallTemp != mixValveCallTemp) {
				_mixCallTemp = callTemp;
				logger().log("MixValveController::amControlZone MixID: ", _index);
				logger().log(relayName(relayID));
				logger().log("MixValveController::request_temp was: ", mixValveCallTemp);
				logger().log("MixValveController::writeToValve::request_temp", _mixCallTemp);
				logger().log("MixValveController::Actual flow_temp: ", readFromValve(Mix_Valve::flow_temp));
				writeToValve(Mix_Valve::request_temp, _mixCallTemp);
				writeToValve(Mix_Valve::control, Mix_Valve::e_new_temp);
			}
			return true;
		}
		else return false;
	}


	// Private Methods

	bool MixValveController::controlZoneRelayIsOn() const {
		return (_relayArr[_controlZoneRelay].getRelayState() != 0);
	}

	bool MixValveController::needHeat(bool isHeating) const {
		if (!controlZoneRelayIsOn()) return false;
		else if (isHeating) {
			return (getStoreTemp() < _mixCallTemp + THERM_STORE_HYSTERESIS);
		}
		else {
			return (readFromValve(Mix_Valve::status) == Mix_Valve::e_Water_too_cool && getStoreTemp() < _mixCallTemp);
		}
	}

	uint8_t MixValveController::writeToValve(Mix_Valve::Registers reg, uint8_t value) const {
		unsigned long waitTime = 3000UL + *_timeOfReset_mS;
		int NO_OF_ATTEMPTS = 4;
		uint8_t hasFailed;
		int attempts = NO_OF_ATTEMPTS;
		do {
			if (attempts == NO_OF_ATTEMPTS / 2) {
				//speedTestDevice(MIX_VALVE_I2C_ADDR);
			}
			hasFailed = _i2C->write(MIX_VALVE_I2C_ADDR, reg + _index * 16, value);
		} while (hasFailed && (--attempts > 0 || millis() < waitTime));

		if (NO_OF_ATTEMPTS - attempts > 0) {
			if (attempts > 0) {
				logger().log("MixArduino write OK after", NO_OF_ATTEMPTS - attempts + 1);
			}
			else {
				logger().log("MixArduino write Failed after", NO_OF_ATTEMPTS - attempts);
			}
		}
		return hasFailed;
	}

	uint8_t MixValveController::readFromValve(Mix_Valve::Registers reg) const {
		unsigned long waitTime = 3000UL + *_timeOfReset_mS;
		int NO_OF_ATTEMPTS = 4;
		uint8_t hasFailed;
		int attempts = NO_OF_ATTEMPTS;
		uint8_t value;
		do {
			if (attempts == NO_OF_ATTEMPTS / 2) {
				//speedTestDevice(MIX_VALVE_I2C_ADDR);
			}
			hasFailed = _i2C->read(MIX_VALVE_I2C_ADDR, reg + _index * 16, 1, &value);
		} while (hasFailed && (--attempts > 0 || millis() < waitTime));

		if (NO_OF_ATTEMPTS - attempts > 0) {
			if (attempts > 0) {
				logger().log("MixArduino read OK after", NO_OF_ATTEMPTS - attempts + 1);
			}
			else {
				logger().log("MixArduino read Failed after", NO_OF_ATTEMPTS - attempts);
			}
		}
		return value;
	}
}