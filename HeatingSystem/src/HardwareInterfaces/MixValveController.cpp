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
	MixValveController::MixValveController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) : I_I2Cdevice_Recovery{recover}, _prog_registers(prog_registers) {}

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, int flowTS_addr, UI_TempSensor & storeTempSens, unsigned long& timeOfReset_mS) {
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
		setReg(Mix_Valve::R_MV_REG_OFFSET, slaveRegOffset);
		_disabled_multimaster_flowTempSensor.setAddress(_flowTS_addr);
		//logger() << F("MixValveController::ini done. Reg:") << _regOffset + Mix_Valve::R_MV_REG_OFFSET << ":" << slaveRegOffset << L_endl;
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
		status = I_I2Cdevice::write(getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MAX_FLOW_TEMP, 1, &val1); // non-recovery
		if (status == _OK) status = I_I2Cdevice::read(getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_MAX_FLOW_TEMP, 1, &val1); // non-recovery 
		return status;
	}

	uint8_t MixValveController::sendSlaveIniData(uint8_t requestINI_flag) {
#ifdef ZPSIM
		uint8_t(&debug)[58] = reinterpret_cast<uint8_t(&)[58]>(*_prog_registers.reg_ptr(0));
		uint8_t(&debugWire)[30] = reinterpret_cast<uint8_t(&)[30]>(Wire.i2CArr[getAddress()][0]);
#endif
		uint8_t errCode;
		errCode = writeToValve(Mix_Valve::R_MV_REG_OFFSET, _regOffset);
		errCode |= writeToValve(Mix_Valve::R_TS_ADDRESS, _flowTS_addr);
		errCode |= writeToValve(Mix_Valve::R_FULL_TRAVERSE_TIME, VALVE_TRANSIT_TIME);
		errCode |= writeToValve(Mix_Valve::R_SETTLE_TIME, VALVE_WAIT_TIME);
		errCode |= writeToValve(Mix_Valve::R_DISABLE_MULTI_MASTER_MODE, _disabled_multimaster);
		setReg(Mix_Valve::R_STATUS, Mix_Valve::MV_OK);
		setReg(Mix_Valve::R_MODE, Mix_Valve::e_Checking);
		setReg(Mix_Valve::R_STATE, Mix_Valve::e_Stop);
		setReg(Mix_Valve::R_RATIO, 30);
		setReg(Mix_Valve::R_FROM_TEMP, 55);
		setReg(Mix_Valve::R_COUNT, 0);
		setReg(Mix_Valve::R_VALVE_POS, 70);
		setReg(Mix_Valve::R_FROM_POS, 75);
		setReg(Mix_Valve::R_FLOW_TEMP, 55);
		setReg(Mix_Valve::R_REQUEST_FLOW_TEMP, 25);
		setReg(Mix_Valve::R_MAX_FLOW_TEMP, 55);
		write(getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS, Mix_Valve::R_MAX_FLOW_TEMP, _prog_registers.reg_ptr(_regOffset + Mix_Valve::R_STATUS));
		if (errCode == _OK) {
			auto newIniStatus = _prog_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION);
			newIniStatus &= ~requestINI_flag;
			_prog_registers.setRegister(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
		}
		logger() <<  F("MixValveController::sendSlaveIniData()") << i2C().getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	void MixValveController::enable_multi_master_mode(bool enable) {
		_disabled_multimaster = !enable;
		writeToValve(Mix_Valve::R_DISABLE_MULTI_MASTER_MODE, _disabled_multimaster);
	}

	uint8_t MixValveController::flowTemp() const {
		return getReg(Mix_Valve::R_FLOW_TEMP);
	}

	bool MixValveController::check() { // called once per second
		if (reEnable() == _OK) {
			auto mixIniStatus = _prog_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION);
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
		auto algorithmMode = Mix_Valve::Mix_Valve::Mode(getReg(Mix_Valve::R_MODE)); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
		if (algorithmMode >= Mix_Valve::e_Error) {
			logger() << "MixValve Mode Error: " << algorithmMode << L_endl;
			uint8_t requestINI_flag = MV_US_REQUESTING_INI << index();
			sendSlaveIniData(requestINI_flag);
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
		static bool wasLogging = false;
		if (!logThis) wasLogging = false;
		else if (!wasLogging  ) {
			wasLogging = true;
			profileLogger() << L_time << "Logging\t\tReq\tIs\tState\tTime\tPos\tRatio\tFromP\tFromT\n";
		}
		if (logThis || !ignoreThis || _previous_valveStatus[valveIndex] != algorithmMode) {
			_previous_valveStatus[valveIndex] = algorithmMode;
			profileLogger() << L_time << L_tabs 
				<< (_regOffset == M_UpStrs ? "US_Mix R/Is" : "DS_Mix R/Is") << _mixCallTemp << flowTemp()
				<< showState() << getReg(Mix_Valve::R_COUNT) << getReg(Mix_Valve::R_VALVE_POS) << getReg(Mix_Valve::R_RATIO) 
				<< getReg(Mix_Valve::R_FROM_POS) << getReg(Mix_Valve::R_FROM_TEMP) << L_endl;
		}
//#endif
	}

	const __FlashStringHelper* MixValveController::showState() const {
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
		setReg(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
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
		//logger() << F("MixValveController::setReg:") << _regOffset + reg << ":" << value << L_endl;
		_prog_registers.setRegister(_regOffset + reg, value);
	}

	Error_codes  MixValveController::writeToValve(int reg, uint8_t value) {
		// All writable registers are single-byte
		waitForWarmUp();
		auto status = reEnable(); // see if is disabled.
		if (status == _OK) {
			//status = write_verify(reg + _regOffset, 1, &value); // recovery-write
			status = write(getReg(Mix_Valve::R_MV_REG_OFFSET) + reg, 1, &value); // recovery-write
			if (status) {
				logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" Reg: ") << reg << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			} //else logger() << L_time << F("MixValve Write OK device 0x") << L_hex << getAddress() << L_endl;
		} else logger() << L_time << F("MixValve Write device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return status;
	}

	void MixValveController::readRegistersFromValve() {
#ifdef ZPSIM
		uint8_t (&debug)[58] = reinterpret_cast<uint8_t (&)[58]>(*_prog_registers.reg_ptr(0));
		uint8_t (&debugWire)[30] = reinterpret_cast<uint8_t (&)[30]>(Wire.i2CArr[getAddress()][0]);
#endif
		if (_disabled_multimaster) {
			_disabled_multimaster_flowTempSensor.readTemperature();
			setReg(Mix_Valve::R_FLOW_TEMP, _disabled_multimaster_flowTempSensor.get_temp());
		}
		uint8_t value = 0;
		auto status = reEnable(); // see if is disabled
		waitForWarmUp();
		constexpr int CONSECUTIVE_COUNT = 1;
		constexpr int MAX_TRIES = 1;
		if (status == _OK) {
			//profileLogger() << "Read :" << Mix_Valve::R_MAX_FLOW_TEMP - Mix_Valve::R_STATUS << " MVreg from: " << getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS << " to : " << _regOffset + Mix_Valve::R_STATUS << L_endl;
			status = read(getReg(Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS , Mix_Valve::R_MAX_FLOW_TEMP - Mix_Valve::R_STATUS, _prog_registers.reg_ptr(_regOffset + Mix_Valve::R_STATUS));
			if (status) {
				logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			}
		} else logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		
		if (getReg(Mix_Valve::R_REQUEST_FLOW_TEMP) != _mixCallTemp) {
			profileLogger() << "Mix_Valve::R_REQUEST_FLOW_TEMP :" << getReg(Mix_Valve::R_REQUEST_FLOW_TEMP) << " Req: " << _mixCallTemp << L_endl;
			profileLogger() << "Mix_Valve::R_FLOW_TEMP :" << getReg(Mix_Valve::R_FLOW_TEMP) << L_endl;
			setReg(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
			writeToValve(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
		}
	}
}