#include "MixValveController.h"
#include "PID_Controller.h"
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
			if (errCode) return errCode;

			loopLogger() <<  F("MixValveController::sendSlaveIniData - WriteRegSet...") << L_endl;
			errCode = writeRegSet(Mix_Valve::R_FLOW_TEMP, Mix_Valve::MV_VOLATILE_REG_SIZE - Mix_Valve::R_FLOW_TEMP);
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
		//e_Moving, e_WaitingToMove, e_ValveOff, e_AtTargetPosition, e_FindingOff, e_HotLimit

		auto algorithmMode = Mix_Valve::Mix_Valve::Mode(reg.get(Mix_Valve::R_MODE));
		if (algorithmMode >= Mix_Valve::e_Error) {
			logger() << "MixValve Mode Error: " << algorithmMode << L_flush;
			return false;
		}

		auto valveIndex = index();
		if (logThis || _previous_valveStatus[valveIndex] != algorithmMode /*|| algorithmMode < Mix_Valve::e_Checking*/ ) {
			reEnable(true);
			_previous_valveStatus[valveIndex] = algorithmMode;
			int endPos = reg.get(Mix_Valve::R_END_POS);
			int ts_err = reg.get(Mix_Valve::R_TS_ERR);
			int psuMaxV = reg.get(Mix_Valve::R_PSU_MAX_V) + 800;
			int psuMaxOffV = reg.get(Mix_Valve::R_PSU_MAX_OFF_V) + 800;
			profileLogger() << L_time << L_tabs 
				<< (valveIndex == M_UpStrs ? "_US_Mix" : "_DS_Mix") << _mixCallTemp << flowTemp()
				<< showPID_State()
				<< showState() << reg.get(Mix_Valve::R_VALVE_POS)
				<< psuMaxV << psuMaxOffV << endPos << ts_err << L_endl;
		}
//#endif
		return true;
	}

	const __FlashStringHelper* MixValveController::showState() const {
		auto reg = registers();
		//e_Moving, e_WaitingToMove, e_ValveOff, e_AtTargetPosition, e_FindingOff, e_HotLimit

		auto mv_mode = reg.get(Mix_Valve::R_MODE);

		switch (mv_mode) {
		case Mix_Valve::e_Moving:
			switch (int8_t(reg.get(Mix_Valve::R_MOTOR_ACTIVITY))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
			case Motor::e_Cooling: return F("Cl");
			case Motor::e_Heating: return F("Ht");
			default: return F("Stp"); // Now stopped!
			}
		case Mix_Valve::e_WaitingToMove: return F("Mx");
		case Mix_Valve::e_ValveOff: return F("Off");
		case Mix_Valve::e_AtTargetPosition: return F("Tg");
		case Mix_Valve::e_FindingOff: return F("FO");
		case Mix_Valve::e_HotLimit: return F("Lim");
		//case Mix_Valve::e_NewReq: return F("Req");
		//case Mix_Valve::e_WaitToCool: return F("WtC");
		default: return F("SEr");
		}
	}

	const __FlashStringHelper* MixValveController::showPID_State() const {
		auto pid_mode = registers().get(Mix_Valve::R_PID_MODE);
		switch (pid_mode) {
		case PID_Controller::NEW_TEMP: return F("NewT");
		case PID_Controller::MOVE_TO_P: return F("ToP");
		case PID_Controller::MOVE_TO_D: return F("ToD");
		case PID_Controller::RETURN_TO_P: return F("FromD");
		case PID_Controller::WAIT_TO_SETTLE: return F("W_Settle");
		case PID_Controller::WAIT_FOR_DRIFT: return F("W_Drift");
		case PID_Controller::GET_BACK_ON_TARGET: return F("GetOnTarget");
		case PID_Controller::MOVE_TO_OFF: return F("ToOff");
		default: return F("Err_state");
		};
	};

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
		auto status_flags = Mix_Valve::I2C_Flags_Obj{ registers().get(Mix_Valve::R_DEVICE_STATE)};

		if (status_flags.is(Mix_Valve::F_STORE_TOO_COOL)) {
		//if (storeTempAtMixer <= minStoreTemp) {
			profileLogger() << L_time << (index() == M_UpStrs ? "_US" : "_DS") << "_Mix\tTooCool. Store-T Is:\t" << storeTempAtMixer << L_endl;
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
				logger() << L_time << F("read device 0x") << L_hex << getAddress() << F(" OK. FlowTemp 255! Try again... Was:") << prev_flowTemp << L_endl;
				status = readRegSet(Mix_Valve::R_DEVICE_STATE, Mix_Valve::MV_NO_TO_READ); // recovery
				flowTemp = reg.get(Mix_Valve::R_FLOW_TEMP);
				if (flowTemp == 255) {
					logger() << L_time << F("read device 0x") << L_hex << getAddress() << F(" OK. FlowTemp 255! Give up.") << L_endl;
					reg.set(Mix_Valve::R_FLOW_TEMP, prev_flowTemp);
					//return false;
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