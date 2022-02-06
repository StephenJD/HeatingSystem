#include "MixValveController.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
//#include "..\Client_DataStructures\Data_TempSensor.h"
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
	MixValveController::MixValveController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) 
		: I2C_To_MicroController{recover, prog_registers}
		, _slaveMode_flowTempSensor{ recover }
	{}

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, int flowTS_addr, UI_TempSensor & storeTempSens, unsigned long& timeOfReset_mS, bool multi_master_mode) {
		//profileLogger() << F("MixValveController::ini ") << index << L_endl;
		I2C_To_MicroController::initialise(addr, index == 0 ? MV_REG_MASTER_0_OFFSET : MV_REG_MASTER_1_OFFSET, timeOfReset_mS);
		_is_multimaster = multi_master_mode;
		_relayArr = relayArr;
		_flowTS_addr = flowTS_addr;
		_storeTempSens = &storeTempSens;
		_controlZoneRelay = index == M_DownStrs ? R_DnSt : R_UpSt;
		auto slaveRegOffset = index == 0 ? MV_REG_SLAVE_0_OFFSET : MV_REG_SLAVE_1_OFFSET;
		registers().set(Mix_Valve::R_MV_REG_OFFSET, slaveRegOffset);
		_slaveMode_flowTempSensor.setAddress(_flowTS_addr);
		//logger() << F("MixValveController::ini done. Reg:") << _regOffset + Mix_Valve::R_MV_REG_OFFSET << ":" << slaveRegOffset << L_endl;
	}

	uint8_t MixValveController::sendSlaveIniData(uint8_t requestINI_flag) {
#ifdef ZPSIM
		uint8_t(&debugWire)[30] = reinterpret_cast<uint8_t(&)[30]>(TwoWire::i2CArr[getAddress()][0]);
		writeToValve(Mix_Valve::R_MODE, Mix_Valve::e_Checking);
#endif
		uint8_t errCode = reEnable(true);
		if (errCode == _OK) {
			errCode = writeToValve(Mix_Valve::R_MV_REG_OFFSET, _regOffset);
			errCode |= writeToValve(Mix_Valve::R_TS_ADDRESS, _flowTS_addr);
			errCode |= writeToValve(Mix_Valve::R_FULL_TRAVERSE_TIME, VALVE_TRANSIT_TIME);
			errCode |= writeToValve(Mix_Valve::R_SETTLE_TIME, VALVE_WAIT_TIME);
			errCode |= writeToValve(Mix_Valve::R_MULTI_MASTER_MODE, _is_multimaster);
			if (errCode) return errCode;

			auto reg = registers();
			reg.set(Mix_Valve::R_STATUS, Mix_Valve::MV_OK);
			reg.set(Mix_Valve::R_RATIO, 30);
			reg.set(Mix_Valve::R_FROM_TEMP, 55);
			reg.set(Mix_Valve::R_FLOW_TEMP, 55);
			reg.set(Mix_Valve::R_REQUEST_FLOW_TEMP, 25);
			errCode = write_verify(reg.get(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS, Mix_Valve::MV_VOLATILE_REG_SIZE - Mix_Valve::R_STATUS, reg.ptr(Mix_Valve::R_STATUS));
			if (errCode == _OK) {
				auto rawReg = rawRegisters();
				auto newIniStatus = rawReg.get(R_SLAVE_REQUESTING_INITIALISATION);
				//logger() << L_time << "SendMixIni. IniStatus was:" << newIniStatus;
				newIniStatus &= ~requestINI_flag;
				//logger() << " New IniStatus:" << newIniStatus << L_endl;
				rawReg.set(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
			}
			readRegistersFromValve();
		}
		logger() <<  F("MixValveController::sendSlaveIniData()") << I2C_Talk::getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	void MixValveController::enable_multi_master_mode(bool enable) {
		_is_multimaster = enable;
		writeToValve(Mix_Valve::R_MULTI_MASTER_MODE, _is_multimaster);
	}

	uint8_t MixValveController::flowTemp() const {
		return registers().get(Mix_Valve::R_FLOW_TEMP);
	}

	bool MixValveController::check() { // called once per second
		if (reEnable() == _OK) {
			auto mixIniStatus = rawRegisters().get(R_SLAVE_REQUESTING_INITIALISATION);
			if (mixIniStatus > ALL_REQUESTING) {
				logger() << L_time << "Bad Mix IniStatus : " << mixIniStatus << L_endl;
				mixIniStatus = ALL_REQUESTING;
				rawRegisters().set(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
			}
			uint8_t requestINI_flag = MV_US_REQUESTING_INI << index();
			if (mixIniStatus & requestINI_flag) sendSlaveIniData(requestINI_flag);
			readRegistersFromValve();
			logMixValveOperation(false);
		}
		return true;
	}

	void MixValveController::monitorMode() {
		if (reEnable() == _OK) {
			_relayArr[_controlZoneRelay].set(); // turn call relay ON
			relayController().updateRelays();
			readRegistersFromValve();
			logMixValveOperation(true);
		}
	}

	void MixValveController::logMixValveOperation(bool logThis) {
//#ifndef ZPSIM
		auto reg = registers();
		auto algorithmMode = Mix_Valve::Mix_Valve::Mode(reg.get(Mix_Valve::R_MODE)); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
		if (algorithmMode >= Mix_Valve::e_Error) {
			logger() << "MixValve Mode Error: " << algorithmMode << L_endl;
			uint8_t requestINI_flag = MV_US_REQUESTING_INI << index();
			sendSlaveIniData(requestINI_flag);
			if (reg.get(Mix_Valve::R_MODE) >= Mix_Valve::e_Error) {
				recovery().resetI2C();
				if (reg.get(Mix_Valve::R_MODE) >= Mix_Valve::e_Error) {
					HardReset::arduinoReset("MixValveController InvalidMode");
				}
			}
		}

		bool ignoreThis = (algorithmMode == Mix_Valve::e_Checking && flowTemp() == _mixCallTemp)
			|| algorithmMode == Mix_Valve::e_AtLimit;

		auto valveIndex = _regOffset ? 1 : 0;
		static bool wasLogging = false;
		if (!logThis) wasLogging = false;
		else if (!wasLogging  ) {
			wasLogging = true;
			profileLogger() << L_time << "Logging\t\tReq\tIs\tState\tTime\tPos\tRatio\tFromP\tFromT\n";
		}
		if (logThis || !ignoreThis || _previous_valveStatus[valveIndex] != algorithmMode) {
			_previous_valveStatus[valveIndex] = algorithmMode;
			profileLogger() << L_time << L_tabs 
				<< (valveIndex == M_UpStrs ? "US_Mix R/Is" : "DS_Mix R/Is") << _mixCallTemp << flowTemp()
				<< showState() << reg.get(Mix_Valve::R_COUNT) << reg.get(Mix_Valve::R_VALVE_POS) << reg.get(Mix_Valve::R_RATIO)
				<< reg.get(Mix_Valve::R_FROM_POS) << reg.get(Mix_Valve::R_FROM_TEMP) << L_endl;
		}
//#endif
	}

	const __FlashStringHelper* MixValveController::showState() const {
		auto reg = registers();
		switch (reg.get(Mix_Valve::R_MODE)) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
		case Mix_Valve::e_Wait: return F("Wt");
		case Mix_Valve::e_Checking: return F("Chk");
		case Mix_Valve::e_Mutex: return F("Mx");
		case Mix_Valve::e_NewReq: return F("Req");
		case Mix_Valve::e_AtLimit: return F("Lim");
		case Mix_Valve::e_DontWantHeat: return F("Min");
		case Mix_Valve::e_Moving:
			switch (int8_t(reg.get(Mix_Valve::R_STATE))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
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
		registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
		writeToValve(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
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
		if (registers().get(Mix_Valve::R_VALVE_POS) == VALVE_TRANSIT_TIME) {
			profileLogger() << L_time << (_regOffset == 1 ? "US" : "DS") << " Mix-Store Temp: " << storeTempAtMixer << L_endl;
			return true;
		}
		return false;
	}

	Error_codes  MixValveController::writeToValve(int reg, uint8_t value) {
		// All writable registers are single-byte
		waitForWarmUp();
		auto status = reEnable(); // see if is disabled.
		if (status == _OK) {
			status = write_verify(registers().get(Mix_Valve::R_MV_REG_OFFSET) + reg, 1, &value); // recovery-write_verify
			if (status) {
				logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			} //else logger() << L_time << F("MixValve Write OK device 0x") << L_hex << getAddress() << L_endl;
		} else logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return status;
	}

	void MixValveController::readRegistersFromValve() {
#ifdef ZPSIM
		//uint8_t (&debugWire)[30] = reinterpret_cast<uint8_t (&)[30]>(TwoWire::i2CArr[getAddress()][0]);
#endif
		int8_t flowTemp = 0;
		auto reg = registers();
		if (!_is_multimaster) {
			_slaveMode_flowTempSensor.readTemperature();
			flowTemp = _slaveMode_flowTempSensor.get_temp();
			writeToValve(Mix_Valve::R_FLOW_TEMP, flowTemp);
			reg.set(Mix_Valve::R_FLOW_TEMP, flowTemp);
			//logger() << "ReadMixVTempS: " << flowTemp << L_endl;
		}
		uint8_t value = 0;
		auto status = reEnable(); // see if is disabled
		waitForWarmUp();
		if (status == _OK) {
			//profileLogger() << "Read :" << Mix_Valve::MV_NO_TO_READ << " MVreg from: " << getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MODE << " to : " << _regOffset + Mix_Valve::R_MODE << L_endl;
			//status = read_verify(getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MODE, Mix_Valve::MV_NO_TO_READ, regPtr(Mix_Valve::R_MODE), 2, 4); // non recovery
			status = read(reg.get(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MODE, Mix_Valve::MV_NO_TO_READ, reg.ptr(Mix_Valve::R_MODE)); // recovery
			if (reg.get(Mix_Valve::R_FLOW_TEMP) == 255) {
				logger() << L_time << F("read device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status);
				logger() << L_dec << F(". FlowTemp reads 255 but is: ") << flowTemp << " at freq: " << runSpeed();
				delay(100);
				status = read(reg.get(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MODE, Mix_Valve::MV_NO_TO_READ, reg.ptr(Mix_Valve::R_MODE)); // recovery
				logger() << L_dec << F(" Re-read: FlowTemp: ") << reg.get(Mix_Valve::R_FLOW_TEMP) << L_endl;
				//auto thisFreq = runSpeed(); 
				//set_runSpeed(max(thisFreq - thisFreq / 10, 10000));
				reg.set(Mix_Valve::R_FLOW_TEMP, flowTemp);
			}
			if (status) {
				logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
				read(reg.get(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MODE, Mix_Valve::MV_NO_TO_READ, reg.ptr(Mix_Valve::R_MODE)); // recovery
			}
		} else logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		
		if (reg.get(Mix_Valve::R_REQUEST_FLOW_TEMP) == 0) { // Resend to confirm new temp request
			reg.set(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
			writeToValve(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
		} else if (reg.get(Mix_Valve::R_REQUEST_FLOW_TEMP) != _mixCallTemp) { // This gets called when R_REQUEST_FLOW_TEMP == 255. -- not sure why!
			logger() << "Mix_Valve::R_REQUEST_FLOW_TEMP :" << reg.get(Mix_Valve::R_REQUEST_FLOW_TEMP) << " Req: " << _mixCallTemp << L_endl;
			logger() << "Mix_Valve::R_FLOW_TEMP :" << reg.get(Mix_Valve::R_FLOW_TEMP) << L_endl;
			reg.set(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
			writeToValve(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
		}
	}
}