#include "MixValveController.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Timer_mS_uS.h>
#include <Clock.h>
#include "..\HardwareInterfaces\I2C_Comms.h"

void ui_yield();

namespace arduino_logger {
	Logger& profileLogger();
}
using namespace arduino_logger;

namespace HardwareInterfaces {
	using namespace Assembly;
	using namespace I2C_Talk_ErrorCodes;
	using namespace i2c_registers;
//#if defined (ZPSIM)
//	using namespace std;
//	ofstream MixValveController::lf("LogFile.csv");
//
//#endif
	MixValveController::MixValveController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) : I_I2Cdevice_Recovery{recover}, _prog_registers(prog_registers) {}

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, UI_TempSensor & storeTempSens, int flowTS_addr, unsigned long& timeOfReset_mS) {
		//profileLogger() << F("MixValveController::ini ") << index << L_endl;
		setAddress(addr);
		_regOffset = index == 0 ? MV_REG_MASTER_0_OFFSET : MV_REG_MASTER_1_OFFSET;
		_relayArr = relayArr;
		_timeOfReset_mS = &timeOfReset_mS;
		_flowTS_addr = flowTS_addr;
		_storeTempSens = &storeTempSens;
		_controlZoneRelay = index == M_DownStrs ? R_DnSt : R_UpSt;
		i2C().extendTimeouts(15000, 6, 1000);
		auto slaveRegOffset = index == 0 ? MV_REG_SLAVE_0_OFFSET : MV_REG_SLAVE_1_OFFSET;
		_prog_registers.setRegister(_regOffset + Mix_Valve::R_MV_REG_OFFSET, slaveRegOffset);
		//profileLogger() << F("MixValveController::ini done") << L_endl;
	}

	void MixValveController::waitForWarmUp() {
		if (*_timeOfReset_mS != 0) {
			auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
			while (!timeOut) {
				//logger() << L_time << F("MixV Warmup used: ") << timeOut.timeUsed() << L_endl;
				ui_yield();
			}
			*_timeOfReset_mS = 0;
		}
	}

	Error_codes MixValveController::testDevice() { // non-recovery test
		if (runSpeed() > 100000) set_runSpeed(100000);
		Error_codes status = _OK;
		uint8_t val1 = 55;
		waitForWarmUp();
		status = I_I2Cdevice::write(Mix_Valve::R_REQUEST_FLOW_TEMP + _regOffset, 1, &val1); // non-recovery
		if (status == _OK) status = I_I2Cdevice::read(Mix_Valve::R_MODE + _regOffset, 1, &val1); // non-recovery 
		return status;
	}

	uint8_t MixValveController::sendSlaveIniData() {
		uint8_t errCode;
		errCode = writeToValve(Mix_Valve::R_TS_ADDRESS, _flowTS_addr);
		errCode |= writeToValve(Mix_Valve::R_FULL_TRAVERSE_TIME, VALVE_TRANSIT_TIME);
		errCode |= writeToValve(Mix_Valve::R_SETTLE_TIME, VALVE_WAIT_TIME);
		if (errCode == _OK) {
			auto newIniStatus = _prog_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION);
			newIniStatus &= ~MV_REQUESTING_INI;
			_prog_registers.setRegister(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
		}
		logger() <<  F("MixValveController::sendSlaveIniData()") << i2C().getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	uint8_t MixValveController::flowTemp() const { return _prog_registers.getRegister(Mix_Valve::R_FLOW_TEMP); }

	bool MixValveController::check() { // called once per second
		if (reEnable() == _OK) {
			readRegistersFromValve();
			logMixValveOperation(false);
		}
		return true;
	}

	void MixValveController::monitorMode() {
		if (reEnable() == _OK) {
			_relayArr[_controlZoneRelay].set(); // turn call relay ON
			relayController().updateRelays();
			logMixValveOperation(true);
		}
	}

	void MixValveController::logMixValveOperation(bool logThis) {
//#ifndef ZPSIM
		auto algorithmMode = Mix_Valve::Mix_Valve::Mode(getReg(Mix_Valve::R_MODE)); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
		if (algorithmMode >= Mix_Valve::e_Error) {
			logger() << "MixValve Error: " << algorithmMode << L_endl;
			sendSlaveIniData();
			if (getReg(Mix_Valve::R_MODE) >= Mix_Valve::e_Error) {
				recovery().resetI2C();
				if (getReg(Mix_Valve::R_MODE) >= Mix_Valve::e_Error) {
					HardReset::arduinoReset("MixValveController InvalidMode");
				}
			}
		}

		bool ignoreThis = (algorithmMode == Mix_Valve::e_Checking && flowTemp() == _mixCallTemp)
			|| algorithmMode == Mix_Valve::e_AtLimit;

		auto valveIndex = _regOffset ? 1 : 0;
		if (logThis || !ignoreThis || _previous_valveStatus[valveIndex] != algorithmMode) {
			_previous_valveStatus[valveIndex] = algorithmMode;
			profileLogger() << L_time << L_tabs 
				<< (_regOffset == M_DownStrs ? "DS_Mix R/Is" : "US_Mix R/Is") << _mixCallTemp << flowTemp()
				<< showState() << getReg(Mix_Valve::R_COUNT) << getReg(Mix_Valve::R_VALVE_POS) << getReg(Mix_Valve::R_RATIO) 
				<< getReg(Mix_Valve::R_FROM_POS) << getReg(Mix_Valve::R_FROM_TEMP) << L_endl;
		}
//#endif
	}

	const __FlashStringHelper* MixValveController::showState() {
			switch (getReg(Mix_Valve::R_MODE)) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
			case Mix_Valve::e_Wait: return F("Wt");
			case Mix_Valve::e_Checking: return F("Chk");
			case Mix_Valve::e_Mutex: return F("Mx");
			case Mix_Valve::e_NewReq: return F("Req");
			case Mix_Valve::e_AtLimit: return F("Lim");
			case Mix_Valve::e_DontWantHeat: return F("Min");
			case Mix_Valve::e_Moving:
				switch (int8_t(getReg(Mix_Valve::R_STATE))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
				case Mix_Valve::e_Moving_Coolest: return F("Min");
				case Mix_Valve::e_Cooling: return F("Cl");
				case Mix_Valve::e_Heating: return F("Ht");
				default: return F("Stp"); // Now stopped!
				}
			default: return F("SEr");
		}
	}

	void MixValveController::sendRequestFlowTemp(uint8_t callTemp) {
		_mixCallTemp = callTemp;
		setReg(Mix_Valve::R_REQUEST_FLOW_TEMP, callTemp);
		writeToValve(Mix_Valve::R_REQUEST_FLOW_TEMP, callTemp);
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
			setReg(Mix_Valve::R_MAX_FLOW_TEMP, maxTemp);
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
			if (_regOffset == 1) 
				debug = true;
			else { // upstairs
				debug = true;
				if (mv_FlowTemp >= _mixCallTemp)
					bool debug = true;
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
				sendRequestFlowTemp(newCallTemp);
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

		int storeTempAtMixer = _storeTempSens->get_temp(); 
		//auto minStoreTemp = isHeating ? _mixCallTemp + THERM_STORE_HYSTERESIS : _mixCallTemp;
		if (getReg(Mix_Valve::R_VALVE_POS) == VALVE_TRANSIT_TIME) {
			profileLogger() << L_time << (_regOffset == 1 ? "US" : "DS") << " Mix-Store Temp: " << storeTempAtMixer << L_endl;
			return true;
		}
		return false;
	}

	uint8_t MixValveController::getReg(int reg) const {
		return _prog_registers.getRegister(_regOffset + reg);
	}

	void MixValveController::setReg(int reg, uint8_t value) {
		_prog_registers.setRegister(_regOffset + reg, value);
	}

	Error_codes  MixValveController::writeToValve(int reg, uint8_t value) {
		// All writable registers are single-byte
		waitForWarmUp();
		auto status = reEnable(); // see if is disabled.
		if (status == _OK) {
			//status = write_verify(reg + _regOffset, 1, &value); // recovery-write
			status = write(reg + _regOffset, 1, &value); // recovery-write
			if (status) {
				logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			} //else logger() << L_time << F("MixValve Write OK device 0x") << L_hex << getAddress() << L_endl;
		} else logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return status;
	}

	void MixValveController::readRegistersFromValve() {
		uint8_t value = 0;
		auto status = reEnable(); // see if is disabled
		waitForWarmUp();
		constexpr int CONSECUTIVE_COUNT = 1;
		constexpr int MAX_TRIES = 1;
		if (status == _OK) {
			status = read(getReg(Mix_Valve::R_MV_REG_OFFSET), Mix_Valve::R_MV_VOLATILE_REG_SIZE, _prog_registers.reg_ptr(_regOffset));


			if (status) {
				logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			}
		} else logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
	}
}