#include "MixValveController.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Timer_mS_uS.h>

void ui_yield();
Logger& profileLogger();

namespace HardwareInterfaces {
	using namespace Assembly;
	using namespace I2C_Talk_ErrorCodes;
//#if defined (ZPSIM)
//	using namespace std;
//	ofstream MixValveController::lf("LogFile.csv");
//
//#endif

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, UI_TempSensor * tempSensorArr, int flowTempSens, int storeTempSens) {
		//profileLogger() << F("MixValveController::ini ") << index << L_endl;
		setAddress(addr);
		_index = index;
		_relayArr = relayArr;
		_tempSensorArr = tempSensorArr;
		_flowTempSens = flowTempSens;
		_storeTempSens = storeTempSens;
		//profileLogger() << F("MixValveController::ini done") << L_endl;
	}

	Error_codes MixValveController::testDevice() { // non-recovery test
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		Error_codes status;
		uint8_t val1[1] = { 1 };
		uint8_t val2[1] = { 1 };
		while (!timeOut) {
			//profileLogger() << F("warmup used: ") << timeOut.timeUsed() << L_endl;
			status = I_I2Cdevice::write_verify(Mix_Valve::request_temp, 1, val1); // non-recovery write request temp
			if (status == _OK) { *_timeOfReset_mS = millis() - 3000; break; }
			else if (status >= _BusReleaseTimeout) break;
			ui_yield();
		}
		//status = I_I2Cdevice::read(Mix_Valve::mode + _index * 16, 1, val1); // non-recovery 
		//status = I_I2Cdevice::read(Mix_Valve::state + _index * 16, 1, val2); // non-recovery 
		//if (val1[0] == 0 && val2[0] == 0) return _I2C_ReadDataWrong;

		val1[0] = 25; // any old data
		status = I_I2Cdevice::write_verify(Mix_Valve::request_temp, 1, val1); // non-recovery write request temp
		return status;
	}

	uint8_t MixValveController::sendSetup() {
		uint8_t errCode;
		check();
		errCode = writeToValve(Mix_Valve::request_temp, _mixCallTemp);
		errCode |= writeToValve(Mix_Valve::ratio, 10);
		errCode |= writeToValve(Mix_Valve::temp_i2c_addr, _tempSensorArr[_flowTempSens].getAddress());
		errCode |= writeToValve(Mix_Valve::max_ontime, VALVE_FULL_TRANSIT_TIME);
		errCode |= writeToValve(Mix_Valve::wait_time, VALVE_WAIT_TIME);
		//profileLogger() <<  F("\nMixValveController::sendSetup() ") << errCode << i2C().getStatusMsg(errCode);
		return errCode;
	}

	uint8_t MixValveController::flowTemp() const { return _tempSensorArr[_flowTempSens].get_temp(); }

	bool MixValveController::check() {
		//profileLogger() << L_time << F("Send temps to MV...") << L_endl;
		auto flowTemp = _tempSensorArr[_flowTempSens].get_temp();
		auto tempError = _tempSensorArr[_flowTempSens].hasError();
		if (!tempError) writeToValve(Mix_Valve::flow_temp, flowTemp);
		return true;
	}

	void MixValveController::setRequestFlowTemp(uint8_t callTemp) {
		writeToValve(Mix_Valve::request_temp, callTemp);
		writeToValve(Mix_Valve::control, Mix_Valve::e_new_temp);
		_mixCallTemp = callTemp;
	}

	bool MixValveController::amControlZone(uint8_t newCallTemp, uint8_t maxTemp, uint8_t zoneRelayID) { // highest callflow temp
		// Lambdas
		auto relayName = [](int id) -> const char* { // only for error reporting
			switch (id) {
			case R_Flat: return "Flat-UFH";
			case R_FlTR: return "Fl-TR";
			case R_HsTR: return "House-TR";
			case R_UpSt: return "US-UFH";
			case R_DnSt: return "DS-UFH";
			default: return "";
			}
		};

		auto setNewControlZone = [maxTemp, this, zoneRelayID, relayName]() {
			writeToValve(Mix_Valve::max_flow_temp, maxTemp);
			writeToValve(Mix_Valve::control, Mix_Valve::e_stop_and_wait); // trigger stop and wait on valve
			profileLogger() << L_time << F("MixValveController: ") << relayName(zoneRelayID) << F(" is new ControlZone.\t New MaxTemp: ") << maxTemp << L_endl;
			_limitTemp = maxTemp;
			_controlZoneRelay = zoneRelayID;
		};

		auto hasLimitPriority = [maxTemp, this, newCallTemp]() -> bool {
			auto zoneIsHeating = newCallTemp > MIN_FLOW_TEMP;
			if (zoneIsHeating && maxTemp <= _limitTemp) {
				_limitTemp = maxTemp;
				if (_mixCallTemp > _limitTemp) _mixCallTemp = 0; // trigger take control
				return true;
			} else return false;
		};
		
		auto amControllingZone = [this,zoneRelayID]() -> bool {return _controlZoneRelay == zoneRelayID && _limitTemp < 100; };

		auto checkForNewControllingZone = [newCallTemp, this, amControllingZone, hasLimitPriority, setNewControlZone]() {
			// controlZone is one calling for heat with lowest max-temp and highest call-temp.
			if (!amControllingZone() && hasLimitPriority() && newCallTemp > _mixCallTemp) { // new control zone
				setNewControlZone();
			}
		};
				
		// Algorithm
		checkForNewControllingZone();
		bool amInControl = amControllingZone();
		if (amInControl) {
			auto mv_FlowTemp = flowTemp();
#if defined (ZPSIM)
			bool debug;
			if (zoneRelayID == 3)
				debug = true;
			switch (_index) {
			case M_UpStrs: {// upstairs
				debug = true;
				if (mv_FlowTemp >= _mixCallTemp)
					bool debug = true;
				break; }
			case M_DownStrs: // downstairs
				debug = true;
				break;
			}
#endif
			
			if (newCallTemp <= MIN_FLOW_TEMP) {
				profileLogger() << L_time << F("MixValveController: ") << relayName(zoneRelayID) << F(" Released Control.") << L_endl;
				newCallTemp = MIN_FLOW_TEMP;
				_relayArr[_controlZoneRelay].clear(); // turn call relay OFF
				_limitTemp = 100; // release control of limit temp.
				amInControl = false;
			}
			else {
				_relayArr[_controlZoneRelay].set(); // turn call relay ON
				if (newCallTemp > _limitTemp) newCallTemp = _limitTemp;
			}

			if (_mixCallTemp != newCallTemp) {
				setRequestFlowTemp(newCallTemp);
			} 
		}
		return amInControl;
	}


	// Private Methods

	int8_t MixValveController::relayInControl() const {
		return _relayArr[_controlZoneRelay].logicalState() == 0 ? -1 : _controlZoneRelay;
	}

	bool MixValveController::needHeat(bool isHeating) {
		if (relayInControl() == -1) return false;

		int storeTempAtMixer = _tempSensorArr[_storeTempSens].get_temp(); 
		auto minStoreTemp = isHeating ? _mixCallTemp + THERM_STORE_HYSTERESIS : _mixCallTemp;
		if (storeTempAtMixer < minStoreTemp /*|| readFromValve(Mix_Valve::status) == Mix_Valve::e_Water_too_cool*/) {
			profileLogger() << L_time << int(index()) << " Mix-Store Temp: " << storeTempAtMixer << L_endl;
			return true;
		}
		return false;
	}

	Error_codes  MixValveController::writeToValve(Mix_Valve::Registers reg, uint8_t value) {
		// All writable registers are single-byte
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		auto status = recovery().newReadWrite(*this); // see if is disabled.
		if (timeOut) return status;
		uint8_t mixReg = reg + _index * 16;
		if (status == _OK) {
			status = write_verify(mixReg, 1, &value); // recovery-write
			if (status) {
				logger() << L_time << F("MixValve Write 0x") << L_hex << getAddress() << F(" failed for Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			} 
		} else logger() << L_time << F("MixValve Write disabled");
		return status;
	}

	int16_t MixValveController::readFromValve(Mix_Valve::Registers reg) {
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		uint16_t value = 0;
		auto status = recovery().newReadWrite(*this); // see if is disabled
		if (timeOut) return status;

		constexpr int CONSECUTIVE_COUNT = 2;
		constexpr int MAX_TRIES = 6;
		constexpr int DATA_MASK = 0xFFFF;
		if (status == _OK) {
			// Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
			// But I2C devices use big-endianness: MSB at the smallest address: So a two-byte read is [MSB, LSB].
			// A two-byte read into a uint16_t will put the Device MSB into the uint16_t LSB, and zero MSB - just what we want when reading single-byte registers!
			status = read(reg + _index * 16, 2, (uint8_t*)&value); // recovery
			if (status == _OK) { 
				status = read_verify_2bytes(reg + _index * 16, value, CONSECUTIVE_COUNT, MAX_TRIES, DATA_MASK); // Non-Recovery
				if (status) status = read(reg + _index * 16, 2, (uint8_t*)&value); // recovery
				//if (status == _OK) logger() << L_time << F("MixValve readFromValve OK: ") << value << L_endl;
			}
			if (reg != Mix_Valve::valve_pos) value >>= 8;

			if (status) {
				logger() << L_time << F("MixValve Read 0x") << L_hex << getAddress() << F(" failed for Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			}
		} else logger() << L_time << F("MixValve Read disabled");
		return value;
	}
}