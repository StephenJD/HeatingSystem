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

	error_codes MixValveController::testDevice() { // non-recovery test
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		error_codes status;
		uint8_t val[1] = { 1 };
		while (!timeOut) {
			//profileLogger() << F("warmup used: ") << timeOut.timeUsed() << L_endl;
			status = I_I2Cdevice::write_verify(7, 1, val); // non-recovery write request temp
			if (status == _OK) { *_timeOfReset_mS = millis() - 3000; break; }
			else if (status >= _BusReleaseTimeout) break;
			ui_yield();
		}
		val[0] = uint8_t(i2C().getI2CFrequency() / 2000); // generate any old data
		status = I_I2Cdevice::write_verify(7, 1, val); // non-recovery write request temp
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

	bool MixValveController::amControlZone(uint8_t newCallTemp, uint8_t maxTemp, uint8_t zoneRelayID) { // highest callflow temp
		// Lambdas
		auto relayName = [](int id) -> const char* { // only for error reporting
			switch (id) {
			case R_Flat: return "Flat";
			case R_FlTR: return "Fl-TR";
			case R_HsTR: return "House-TR";
			case R_UpSt: return "US";
			case R_DnSt: return "DS";
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

		auto hasLimitPriority = [maxTemp, this, zoneRelayID, setNewControlZone]() -> bool {
			auto zoneIsHeating = _relayArr[zoneRelayID].logicalState();
			if (zoneIsHeating && maxTemp < _limitTemp) setNewControlZone();
			return zoneIsHeating && maxTemp == _limitTemp;
		};
		
		auto amControllingZone = [this,zoneRelayID]() -> bool {return _controlZoneRelay == zoneRelayID; };

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
				_mixCallTemp = newCallTemp;
				writeToValve(Mix_Valve::request_temp, _mixCallTemp);				
				writeToValve(Mix_Valve::control, Mix_Valve::e_new_temp);
				//auto mv_ReqTemp = readFromValve(Mix_Valve::request_temp);
				//if (mv_ReqTemp != _mixCallTemp) { 
				//	profileLogger() << L_time << F("MixValve Confirm 0x") << L_hex << getAddress() << F(" failed for request_temp. Wrote: ") << L_dec << _mixCallTemp << F(" Read: ") << mv_ReqTemp << L_endl;
				//}
			} else if (mv_FlowTemp != _mixCallTemp) {
				//profileLogger() << L_time << L_tabs << relayName(zoneRelayID);
				//profileLogger() << F("MV_Request: ") << _mixCallTemp;
				//profileLogger() << F("Is: ") << mv_FlowTemp << L_endl;
			}
		}
		return amInControl;
	}


	// Private Methods

	bool MixValveController::controlZoneRelayIsOn() const {
		return (_relayArr[_controlZoneRelay].logicalState() != 0);
	}

	bool MixValveController::needHeat(bool isHeating) {
		if (!controlZoneRelayIsOn()) return false;

		auto storeTempAtMixer = _tempSensorArr[_storeTempSens].get_temp(); 

		if (isHeating) {
			return (storeTempAtMixer < _mixCallTemp + THERM_STORE_HYSTERESIS);
		}
		else {
			return (storeTempAtMixer < _mixCallTemp || readFromValve(Mix_Valve::status) == Mix_Valve::e_Water_too_cool);
		}
	}

	error_codes  MixValveController::writeToValve(Mix_Valve::Registers reg, uint8_t value) {
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		auto status = recovery().newReadWrite(*this); // see if is disabled.
		uint8_t mixReg = reg + _index * 16;
		if (status == _OK) {
			do {
				i2C().setI2CFrequency(runSpeed());
				status = I_I2Cdevice::write_verify(mixReg, 1, &value); // non-recovery
				ui_yield();
			} while (status && !timeOut);

			if (status) {
				status = write_verify(mixReg, 1, &value); // attempt recovery-write
				if (status) {
					profileLogger() << L_time << F("MixValve Write 0x") << L_hex << getAddress() << F(" failed for Reg: ") << reg << I2C_Talk::getStatusMsg(status) << L_endl;
				} 
				//else {
				//	uint8_t readValue = readFromValve(reg);
				//	status = (readValue == value? _OK : _I2C_ReadDataWrong);
				//	if (status) {
				//		profileLogger() << L_time << F("MixValve Write 0x") << L_hex << getAddress() << F(" failed for Reg: ") << reg << F(" Verify OK but then Wrote: ") << L_dec << value << F(" Read: ") << readValue << L_endl;
				//	} //else {
				//		//profileLogger() << F("\tMixValveController::writeToValve OK after recovery. Write: ") << value << F(" Read: ") << readValue << L_endl;
				//	//}
				//}
			} 
		} else profileLogger() << L_time << F("MixValve Write disabled");
		return status;
	}

	uint8_t MixValveController::readFromValve(Mix_Valve::Registers reg) {
		auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
		uint8_t value = 0;
		auto status = recovery().newReadWrite(*this); // see if is disabled
		if (status == _OK) {
			do {
				i2C().setI2CFrequency(runSpeed());			
				status = I_I2Cdevice::read(reg + _index * 16, 1, &value); // non-recovery
				ui_yield();
			}
			while (status && !timeOut);

			if (status) {
				status = read(reg + _index * 16, 1, &value); // attempt recovery
				if (status) {
					profileLogger() << L_time << F("MixValve Read 0x") << L_hex << getAddress() << F(" failed for Reg: ") << reg << I2C_Talk::getStatusMsg(status) << L_endl;
				} else {
					profileLogger() << L_time << F("MixValve Read OK after recovery.") << L_endl;
				}
			}
		} else profileLogger() << L_time << F("MixValve Read disabled");
		return value;
	}
}