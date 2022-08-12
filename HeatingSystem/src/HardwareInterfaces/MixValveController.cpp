#include "MixValveController.h"
#include "A__Constants.h"
#include "..\Assembly\HeatingSystemEnums.h"
#include <I2C_Talk_ErrorCodes.h>
#include <I2C_Reset.h>
#include <Logging.h>
//#include "..\Client_DataStructures\Data_TempSensor.h"
#include "..\Client_DataStructures\Data_Relay.h"
#include <Timer_mS_uS.h>
#include <Clock.h>
#include "..\HardwareInterfaces\I2C_Comms.h"

void ui_yield();

namespace arduino_logger {
	Logger& profileLogger();
	Logger& loopLogger();
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
	MixValveController::MixValveController(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& local_registers) 
		: I2C_To_MicroController{recover, local_registers}
	{
		disable(); // prevent use until sendSlaveIniData
	}

	void MixValveController::initialise(int index, int addr, UI_Bitwise_Relay * relayArr, int flowTS_addr, UI_TempSensor & storeTempSens) {
#ifdef ZPSIM
		uint8_t(&debugWire)[SIZE_OF_ALL_REGISTERS] = reinterpret_cast<uint8_t(&)[SIZE_OF_ALL_REGISTERS]>(TwoWire::i2CArr[PROGRAMMER_I2C_ADDR]);
#endif
		logger() << F("MixValveController::ini ") << index << L_endl;
		auto localRegOffset = index == M_UpStrs ? PROG_REG_MV0_OFFSET : PROG_REG_MV1_OFFSET;
		auto remoteRegOffset = index == M_UpStrs ? MV0_REG_OFFSET : MV1_REG_OFFSET;
		I2C_To_MicroController::initialise(addr, localRegOffset, remoteRegOffset);
		auto reg = registers();
		reg.set(Mix_Valve::R_REMOTE_REG_OFFSET, localRegOffset);
		_relayArr = relayArr;
		_flowTS_addr = flowTS_addr;
		_storeTempSens = &storeTempSens;
		_controlZoneRelay = index == M_DownStrs ? R_DnSt : R_UpSt;
	}

	uint8_t MixValveController::sendSlaveIniData(volatile uint8_t& requestINI_flags) {
#ifdef ZPSIM
		uint8_t(&debugWire)[30] = reinterpret_cast<uint8_t(&)[30]>(TwoWire::i2CArr[getAddress()]);
		writeRegValue(Mix_Valve::R_MODE, Mix_Valve::e_Checking);
#endif
		auto thisIniFlag = MV_US_REQUESTING_INI << index();
		if (!(thisIniFlag & requestINI_flags)) return _OK;

		uint8_t errCode = reEnable(true);
		auto reg = registers();
		Mix_Valve::I2C_Flags_Obj{ reg.get(Mix_Valve::R_DEVICE_STATE) };

		if (errCode == _OK) {
			loopLogger() << index() <<  F("] MV_sendSlaveIniData - WriteReg...") << L_endl;
			logger() << index() <<  F("] MV_sendSlaveIniData") << L_endl;
			errCode = writeReg(Mix_Valve::R_DEVICE_STATE);
			errCode |= writeReg(Mix_Valve::R_REMOTE_REG_OFFSET);
			errCode |= writeRegValue(Mix_Valve::R_TS_ADDRESS, _flowTS_addr);
			errCode |= writeRegValue(Mix_Valve::R_SETTLE_TIME, VALVE_WAIT_TIME);
			if (errCode) return errCode;

			loopLogger() <<  F("MixValveController::sendSlaveIniData - WriteRegSet...") << L_endl;
			errCode = writeRegSet(Mix_Valve::R_RATIO, Mix_Valve::MV_VOLATILE_REG_SIZE - Mix_Valve::R_RATIO);
			if (errCode == _OK) {
				requestINI_flags &= ~thisIniFlag;
				loopLogger() << " New IniStatus:" << requestINI_flags << L_endl;
			}
		}
		loopLogger() <<  F("MixValveController::sendSlaveIniData()") << I2C_Talk::getStatusMsg(errCode) << " State:" << reg.get(Mix_Valve::R_DEVICE_STATE) << L_endl;
		return errCode;
	}

	uint8_t MixValveController::flowTemp() const {
		return registers().get(Mix_Valve::R_FLOW_TEMP);
	}

	bool MixValveController::readReg_and_log(bool alwaysLog) { // called once per second
		bool mixV_OK = true;
		if (reEnable() == _OK) {
			auto mixIniStatus = rawRegisters().get(R_SLAVE_REQUESTING_INITIALISATION);
			if (mixIniStatus > ALL_REQUESTING) {
				logger() << L_time << "Bad Mix IniStatus : " << mixIniStatus << L_endl;
				mixIniStatus = ALL_REQUESTING;
				rawRegisters().set(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
				mixV_OK = false;
			}
			uint8_t requestINI_flag = MV_US_REQUESTING_INI << index();
			if (mixIniStatus & requestINI_flag) mixV_OK = false;
			else {
				mixV_OK &= readRegistersFromValve_OK();
				if (!mixV_OK) {
					auto& iniStatus = *rawRegisters().ptr(R_SLAVE_REQUESTING_INITIALISATION);
					iniStatus |= requestINI_flag;
				}
				//logMixValveOperation(alwaysLog);
				logMixValveOperation(true);
			}
		}
		return mixV_OK;
	}

	bool MixValveController::tuningMixV() {
		if (registers().get(Mix_Valve::R_ADJUST_MODE) == Mix_Valve::PID_CHECK) {
			monitorMode();
			return true;
		} else return false;
	}	
	
	void MixValveController::monitorMode() {
		if (reEnable() == _OK) {
			_relayArr[_controlZoneRelay].set(); // turn call relay ON
			relayController().updateRelays();
			readReg_and_log(true);
		}
	}

	bool MixValveController::logMixValveOperation(bool logThis) {
//#ifndef ZPSIM
		auto reg = registers();
		// {e_newReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_Error }
		// 	Tune: {init, findOff, riseToSetpoint, findMax, fallToSetPoint, findMin, lastRise, calcPID, restart}
		auto adjustMode = [](uint8_t adjust_mode, float & psuV) {
			switch (adjust_mode) {
			case Mix_Valve::A_GOOD_RATIO: return "G";
			case Mix_Valve::A_UNDERSHOT: return "U";
			case Mix_Valve::A_OVERSHOT: return "O";
			case Mix_Valve::PID_CHECK:
				psuV = (psuV/5.f/256.f);
				return "T";
			default: return "";
			}
		};

		auto algorithmMode = Mix_Valve::Mix_Valve::Mode(reg.get(Mix_Valve::R_MODE));
		if (algorithmMode >= Mix_Valve::e_Error) {
			logger() << "MixValve Mode Error: " << algorithmMode << L_endl;
			return false;
		}

		auto valveIndex = index();
		if (logThis || _previous_valveStatus[valveIndex] != algorithmMode /*|| algorithmMode < Mix_Valve::e_Checking*/ ) {
			_previous_valveStatus[valveIndex] = algorithmMode;
			float psuV = reg.get(Mix_Valve::R_PSU_V) * 5.f;
			auto adjust_mode = reg.get(Mix_Valve::R_ADJUST_MODE);
			profileLogger() << L_time << L_tabs 
				<< (valveIndex == M_UpStrs ? "_US_Mix" : "_DS_Mix") << _mixCallTemp << flowTemp()
				<< showState(adjust_mode) << reg.get(Mix_Valve::R_COUNT) << reg.get(Mix_Valve::R_VALVE_POS)
				<< adjustMode(adjust_mode, psuV) << reg.get(Mix_Valve::R_RATIO) << psuV  << L_endl;
		}
//#endif
		return true;
	}

	const __FlashStringHelper* MixValveController::showState(uint8_t adjust_mode) const {
		auto reg = registers();
		// e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff
		// /*These are temporary triggers */, e_newReq, e_swapMutex, e_completedMove, e_overshot, e_reachedLimit, e_Error
		auto mv_mode = reg.get(Mix_Valve::R_MODE);
		if (adjust_mode == Mix_Valve::PID_CHECK) {
			switch (mv_mode) {
			case Mix_Valve::init:
				return F("Ini");
			case Mix_Valve::findOff:
				return F("z");
			case Mix_Valve::riseToSetpoint:
				return F("^");
			case Mix_Valve::findMax:
				return F("^^");
			case Mix_Valve::fallToSetPoint:
				return F("v");
			case Mix_Valve::findMin:
				return F("vv");
			case Mix_Valve::lastRise:
				return F("^");
			case Mix_Valve::calcPID:
				return F("?");
			default:
				return F("Ini");
			}
		}
		switch (mv_mode) {
		case Mix_Valve::e_Wait: return F("Wt");
		//case Mix_Valve::e_swapMutex: return F("SwM");
		case Mix_Valve::e_Checking: return F("Ok");
		case Mix_Valve::e_Mutex: return F("Mx");
		case Mix_Valve::e_NewReq: return F("Req");
		case Mix_Valve::e_WaitToCool: return F("WtC");
		case Mix_Valve::e_ValveOff: return F("Off");
		//case Mix_Valve::e_reachedLimit: return F("RLm");
		case Mix_Valve::e_HotLimit: return F("Lim");
		//case Mix_Valve::e_completedMove: return F("Mok");
		//case Mix_Valve::e_overshot: return F("OvS");
		//case Mix_Valve::e_findOff:
		case Mix_Valve::e_StopHeating:
		case Mix_Valve::e_Moving:
			switch (int8_t(reg.get(Mix_Valve::R_MOTOR_ACTIVITY))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
			case Mix_Valve::e_Moving_Coolest: return F("Min");
			case Mix_Valve::e_Cooling: return F("Cl");
			case Mix_Valve::e_Heating: return F("Ht");
			default: return F("Stp"); // Now stopped!
			}
		default: return F("SEr");
		}
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
			profileLogger() << L_time << F("MixValveController: ") << relayName(zoneRelayID) << F(" is new ControlZone.\tNew MaxTemp:\t") << maxTemp << L_endl;
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
			if (index() == M_DownStrs) // DS
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
		auto minStoreTemp = isHeating ? _mixCallTemp + THERM_STORE_HYSTERESIS : _mixCallTemp;
		//if (registers().get(Mix_Valve::R_VALVE_POS) >= VALVE_TRANSIT_TIME && storeTempAtMixer < _mixCallTemp + THERM_STORE_HYSTERESIS) {
		if (storeTempAtMixer <= minStoreTemp) {
			profileLogger() << L_time << (index() == M_UpStrs ? "_US" : "_DS") << "_Mix\tStore-Req Is:\t" << storeTempAtMixer << L_endl;
			return true;
		}
		return false;
	}

	void MixValveController::sendRequestFlowTemp(uint8_t callTemp) {
		_mixCallTemp = callTemp;
		registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
		//writeReg(Mix_Valve::R_REQUEST_FLOW_TEMP);
		I_I2Cdevice::write(Mix_Valve::R_REQUEST_FLOW_TEMP + _remoteRegOffset, 1, &_mixCallTemp);
		profileLogger() << L_time << (index() == M_UpStrs ? "_US" : "_DS") << "_Mix\tSend new Request:\t" << _mixCallTemp << L_endl;
	}

	bool MixValveController::readRegistersFromValve_OK() { // called once per second
#ifdef ZPSIM
		//uint8_t (&debugWire)[30] = reinterpret_cast<uint8_t (&)[30]>(TwoWire::i2CArr[getAddress()]);
#endif
		// Lambdas
		auto give_MixV_Bus = [this](Mix_Valve::I2C_Flags_Obj i2c_status) {
			rawRegisters().set(R_PROG_WAITING_FOR_REMOTE_I2C_COMS, 1);
			i2c_status.set(Mix_Valve::F_I2C_NOW);
			writeRegValue(Mix_Valve::R_DEVICE_STATE, i2c_status);
		};

		// Algorithm
		auto status = reEnable(); // see if is disabled
		if (status == _OK) {
			wait_DevicesToFinish(rawRegisters());
			status = readReg(Mix_Valve::R_DEVICE_STATE); // recovery
			if (status) {
				logger() << L_time << F("read R_DEVICE_STATE 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status);
				return false;
			}
			auto reg = registers();
			auto i2c_status = Mix_Valve::I2C_Flags_Obj{ reg.get(Mix_Valve::R_DEVICE_STATE) };
			//logger() << L_time << "MV-Master[" << index() << "] Req Temp" << L_endl;
			give_MixV_Bus(i2c_status);
			wait_DevicesToFinish(rawRegisters());
			auto prev_flowTemp = reg.get(Mix_Valve::R_FLOW_TEMP);
			status = readRegSet(Mix_Valve::R_DEVICE_STATE, Mix_Valve::MV_NO_TO_READ); // recovery
			auto flowTemp = reg.get(Mix_Valve::R_FLOW_TEMP);
			
			if (status) {
				logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
				return false;
				//status = readRegSet(Mix_Valve::R_DEVICE_STATE, Mix_Valve::MV_NO_TO_READ);
			} else if (flowTemp == 255) {
				logger() << L_time << F("read device 0x") << L_hex << getAddress() << F(" OK. FlowTemp 255! Try again...") << L_endl;
				status = readRegSet(Mix_Valve::R_DEVICE_STATE, Mix_Valve::MV_NO_TO_READ); // recovery
				flowTemp = reg.get(Mix_Valve::R_FLOW_TEMP);
				if (flowTemp == 255) {
					reg.set(Mix_Valve::R_FLOW_TEMP, prev_flowTemp);
					return false;
				}
			}
			if (reg.get(Mix_Valve::R_REQUEST_FLOW_TEMP) == 0) { // Resend to confirm new temp request
				profileLogger() << L_time << (index() == M_UpStrs ? "_US" : "_DS") << "_Mix\tConfirm new Request:\t" << _mixCallTemp << L_endl;
				reg.set(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
				status |= writeReg(Mix_Valve::R_REQUEST_FLOW_TEMP);
				_relayArr[_controlZoneRelay].clear(); // turn call relay OFF to terminate MV-Tune
				relayController().updateRelays();
				//I_I2Cdevice::write(Mix_Valve::R_REQUEST_FLOW_TEMP + _remoteRegOffset, 1, &_mixCallTemp);
			} else if (reg.get(Mix_Valve::R_REQUEST_FLOW_TEMP) != _mixCallTemp) { // This gets called when R_REQUEST_FLOW_TEMP == 255. -- not sure why!
				profileLogger() << L_time << (index() == M_UpStrs ? "_US" : "_DS") << "_Mix\tCorrect Request:\t" << _mixCallTemp << L_endl;
				reg.set(Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
				status |= writeReg(Mix_Valve::R_REQUEST_FLOW_TEMP);
				//I_I2Cdevice::write(Mix_Valve::R_REQUEST_FLOW_TEMP + _remoteRegOffset, 1, &_mixCallTemp);
			}
		} else logger() << L_time << F("MixValve Read device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return status == _OK;
	}
}