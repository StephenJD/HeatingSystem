#include "MixValveController.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Timer_mS_uS.h>

void ui_yield();

namespace HardwareInterfaces {
	using namespace Assembly;
	using namespace I2C_Talk_ErrorCodes;
//#if defined (ZPSIM)
//	using namespace std;
//	ofstream MixValveController::lf("LogFile.csv");
//
//#endif

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, UI_TempSensor * tempSensorArr, int flowTempSens, int storeTempSens) {
		//logger() << F("MixValveController::ini ") << index << L_endl;
		setAddress(addr);
		_index = index;
		_relayArr = relayArr;
		_tempSensorArr = tempSensorArr;
		_flowTempSens = flowTempSens;
		_storeTempSens = storeTempSens;
		//logger() << F("MixValveController::ini done") << L_endl;
	}

	error_codes MixValveController::testDevice() { // non-recovery test
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		error_codes status;
		uint8_t val[1] = { 1 };
		while (!timeOut) {
			//logger() << F("warmup used: ") << timeOut.timeUsed() << L_endl;
			status = I_I2Cdevice::write_verify(7, 1, val); // non-recovery write request temp
			if (status == _OK) { *_timeOfReset_mS = millis() - 3000; break; }
			else if (status >= _BusReleaseTimeout) break;
			ui_yield();
		}
		val[0] = uint8_t(i2C().getI2CFrequency() / 2000);
		status = I_I2Cdevice::write_verify(7, 1, val); // non-recovery write request temp
		return status;
	}

	uint8_t MixValveController::getStoreTemp() const {
		return _tempSensorArr[_storeTempSens].get_temp();
	}

	uint8_t MixValveController::sendSetup() {
		uint8_t errCode;
		errCode = writeToValve(Mix_Valve::flow_temp, _tempSensorArr[_flowTempSens].get_temp());
		errCode |= writeToValve(Mix_Valve::request_temp, _mixCallTemp);
		errCode |= writeToValve(Mix_Valve::ratio, 10);
		errCode |= writeToValve(Mix_Valve::temp_i2c_addr, _tempSensorArr[_flowTempSens].getAddress());
		errCode |= writeToValve(Mix_Valve::max_ontime, VALVE_FULL_TRANSIT_TIME);
		errCode |= writeToValve(Mix_Valve::wait_time, VALVE_WAIT_TIME);
		logger() <<  F("\nMixValveController::sendSetup() ") << errCode << i2C().getStatusMsg(errCode);
		return errCode;
	}

	bool MixValveController::check() {
		//logger() << L_time << F("Send temps to MV...") << L_endl;
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

	bool MixValveController::amControlZone(uint8_t callTemp, uint8_t maxTemp, uint8_t zoneRelayID) { // highest callflow temp
		// Lambdas
		auto checkMaxMixTempIsPermissible = [maxTemp,this]() { if (maxTemp < _limitTemp) _limitTemp = maxTemp;};

		auto requestIsFromControllingZone = [zoneRelayID, this]()-> bool {return _controlZoneRelay == zoneRelayID; };

		auto checkForNewControllingZone = [callTemp, zoneRelayID, maxTemp, this, requestIsFromControllingZone]() {
			if (!requestIsFromControllingZone() && callTemp > _mixCallTemp) { // new control zone
				writeToValve(Mix_Valve::control, Mix_Valve::e_stop_and_wait); // trigger stop and wait on valve
				writeToValve(Mix_Valve::max_flow_temp, maxTemp);
				logger() << L_time << F("MixValveController::amControlZone\t New CZ - Write new MaxTemp: ") << maxTemp << L_endl;
				_controlZoneRelay = zoneRelayID; 
			}
		};
				
		// Algorithm
		checkMaxMixTempIsPermissible();
		checkForNewControllingZone();

		if (requestIsFromControllingZone()) {
#if defined (ZPSIM)
			bool debug;
			if (zoneRelayID == 3)
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
			
			if (callTemp <= MIN_FLOW_TEMP) {
				callTemp = MIN_FLOW_TEMP;
				_relayArr[_controlZoneRelay].set(0); // turn call relay OFF
				_limitTemp = 100; // reset since it might have been the limiting zone
			}
			else {
				_relayArr[_controlZoneRelay].set(1); // turn call relay ON
				if (callTemp > _limitTemp) callTemp = _limitTemp;
			}
			uint8_t mixValveCallTemp = readFromValve(Mix_Valve::request_temp);
			auto loggingTemp = readFromValve(Mix_Valve::flow_temp);
			if (_mixCallTemp != callTemp || _mixCallTemp != mixValveCallTemp) {
				_mixCallTemp = callTemp;
				logger() << L_time << F("MixValveController::amControlZone MixID: ") << _index;
				logger() << F("\n\t") << relayName(zoneRelayID);
				logger() << F("\n\tNew_request_temp: ") << _mixCallTemp;
				logger() << F("\n\trequest_temp was: ") << mixValveCallTemp;
				logger() << F("\n\tActual flow_temp: ") << loggingTemp << L_endl;
				writeToValve(Mix_Valve::request_temp, _mixCallTemp);				
				writeToValve(Mix_Valve::control, Mix_Valve::e_new_temp);
				logger() << F(" MixValveController:: Done writing to valve") << L_endl;
			} else if (loggingTemp != _mixCallTemp) {
				logger() << L_time << F("MixValveController::amControlZone MixID: ") << _index;
				logger() << F("\n\t") << relayName(zoneRelayID);
				logger() << F("\n\tRequest_temp is: ") << _mixCallTemp;
				logger() << F("\n\tActual flow_temp: ") << loggingTemp << L_endl;
			}
			return true;
		}
		else return false;
	}


	// Private Methods

	bool MixValveController::controlZoneRelayIsOn() const {
		return (_relayArr[_controlZoneRelay].logicalState() != 0);
	}

	bool MixValveController::needHeat(bool isHeating) {
		if (!controlZoneRelayIsOn()) return false;
		else if (isHeating) {
			return (getStoreTemp() < _mixCallTemp + THERM_STORE_HYSTERESIS);
		}
		else {
			return (readFromValve(Mix_Valve::status) == Mix_Valve::e_Water_too_cool && getStoreTemp() < _mixCallTemp);
		}
	}

	error_codes  MixValveController::writeToValve(Mix_Valve::Registers reg, uint8_t value) {
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		auto status = recovery().newReadWrite(*this); // see if is disabled;
		if (status == _OK) {
			do {
				status = I_I2Cdevice::write_verify(uint8_t(reg + _index * 16), 1, &value); // non-recovery
				ui_yield();
			} while (status && !timeOut);

			if (status) {
				logger() << F("\tMixValveController::writeToValve failed first try to Reg: ") << reg << F(" Value: ") << value << L_endl;
				status = write_verify(uint8_t(reg + _index * 16), 1, &value); // attempt recovery-write
			}

			if (status) logger() << F("\tMixValveController::writeToValve failed. 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << L_endl;
		}
		return status;
	}

	uint8_t MixValveController::readFromValve(Mix_Valve::Registers reg) {
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		uint8_t value = 0;
		auto status = recovery().newReadWrite(*this); // see if is disabled
		if (status == _OK) {
			do {
				status = I_I2Cdevice::read(reg + _index * 16, 1, &value); // non-recovery
				ui_yield();
			}
			while (status && !timeOut);

			if (status) {
				logger() << F("\tMixValveController::readFromValve failed first try from Reg: ") << reg << L_endl;
				status = read(reg + _index * 16, 1, &value); // attempt recovery
			}

			if (status) logger() << F("\tMixValveController::readFromValve failed. 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << L_endl;
		}
		return value;
	}
}