#include "ConsoleController_Thick.h"
#include "TowelRail.h"
#include "ThermalStore.h"
#include "Zone.h"
#include <I2C_Reset.h>

void ui_yield();
namespace arduino_logger {
	Logger& logger();
	Logger& profileLogger();
	Logger& loopLogger();
}
using namespace arduino_logger;

namespace HardwareInterfaces {
	using namespace I2C_Talk_ErrorCodes;
	using OLED = OLED_Thick_Display;

	ConsoleController_Thick::ConsoleController_Thick(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) 
		: I2C_To_MicroController{ recover, prog_registers }
	{}

	void ConsoleController_Thick::initialise(int index, int addr, int roomTS_addr, TowelRail& towelRail, Zone& dhw, Zone& zone, uint8_t console_mode) {
		logger() << F("ConsoleController_Thick::ini ") << index << " i2cMode: " << console_mode  << L_endl;
		auto localRegOffset = PROG_REG_RC_US_OFFSET + (OLED::R_DISPL_REG_SIZE * index);
		I2C_To_MicroController::initialise(addr, localRegOffset, 0);
		auto reg = registers();
		reg.set(OLED::R_ROOM_TS_ADDR, roomTS_addr);
		reg.set(OLED::R_DEVICE_STATE, console_mode);
		reg.set(OLED::R_REMOTE_REG_OFFSET, localRegOffset);
		_towelRail = &towelRail;
		_dhw = &dhw;
		_zone = &zone;
		logger() << F("ConsoleController_Thick::ini done.") << L_endl;
	}

	uint8_t ConsoleController_Thick::get_console_mode() const {
		return registers().get(OLED::R_DEVICE_STATE);
	}

	bool ConsoleController_Thick::console_mode_is(int flag) const {
		return OLED::I2C_Flags_Obj(registers().get(OLED::R_DEVICE_STATE)).is(flag);
	}

	void ConsoleController_Thick::set_console_mode(uint8_t mode) {
		registers().set(OLED::R_DEVICE_STATE, mode);
		auto& ini_state = *rawRegisters().ptr(R_SLAVE_REQUESTING_INITIALISATION);
		ini_state |= RC_US_REQUESTING_INI << index();
		sendSlaveIniData(ini_state);
	}

	uint8_t ConsoleController_Thick::sendSlaveIniData(volatile uint8_t& requestINI_flags) {
#ifdef ZPSIM
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()]);
#endif
		auto thisIniFlag = RC_US_REQUESTING_INI << index();
		if (!(thisIniFlag & requestINI_flags)) return _OK;
		loopLogger() << index() << "] Console_sendSlaveIniData..." << L_endl;
		logger() << L_time << F("Console_sendSlaveIniData: ") << index() << L_flush;
		uint8_t errCode = reEnable(true);
		if (errCode == _OK) {
			auto reg = registers();
			auto devFlags = OLED::I2C_Flags_Ref(*reg.ptr(OLED::R_DEVICE_STATE));
			devFlags.set(OLED::F_PROGRAMMER_CHANGED_DATA);

			reg.set(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
			reg.set(OLED::R_WARM_UP_ROOM_M10, 10);
			reg.set(OLED::R_ON_TIME_T_RAIL, 0);
			reg.set(OLED::R_WARM_UP_DHW_M10, 2);
			errCode = writeRegSet(OLED::R_REQUESTING_T_RAIL, OLED::R_DISPL_REG_SIZE - OLED::R_REQUESTING_T_RAIL);
			errCode |= writeRegSet(OLED::R_REMOTE_REG_OFFSET,3);
			_state = AWAIT_REM_ACK_TEMP;

			if (errCode == _OK) {
				logger() << L_time << "SendConsIni. IniStatus was:" << requestINI_flags;
				requestINI_flags &= ~thisIniFlag;
				logger() << " IniStatus now:" << requestINI_flags << " sent Offset:" << _localRegOffset << L_endl;
			}
		}
		logger() << F("ConsoleController_Thick::sendSlaveIniData()[") << index() << F("] i2CMode: ") << registers().get(OLED::R_DEVICE_STATE) << i2C().getStatusMsg(errCode) << L_endl;
		loopLogger() << "Console_sendSlaveIniData_OK" << L_endl;
		return errCode;
	}

	void ConsoleController_Thick::logRemoteRegisters() {
		uint8_t regVal;
		logger() << "OfSt" << L_tabs << "MODE" << "TSad" << "RT" << "Fr" << "TRl?" << "HW?" << "Rm?" << "Rmm" << "TRm" << "HWm" << L_endl;
		for (int reg = OLED::R_REMOTE_REG_OFFSET; reg < OLED::R_DISPL_REG_SIZE; ++reg) {
			readRegVerifyValue(reg, regVal);
			logger() << regVal << ", \t";
		}
		logger() << L_endl;
	}

	bool ConsoleController_Thick::refreshRegistersOK() { // Called every second
		// Room temp is read and sent by the console to the Programmer.
		// Requests and Warmup-times are read /sent by the programmer.
		// New temp requests sent to remote are read by remote which then sets its request Temp Reg to 0.
		auto reg = registers();

#ifdef ZPSIM
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()]);
#endif	

		logger() << L_time << index() << "] refreshRegisters-start 0x" << L_hex << getAddress() << L_endl;
		loopLogger() << L_time << index() << "] refreshRegisters-start 0x" << L_hex << getAddress() << L_endl;
		if (reEnable(true) != _OK) return false;
		if (!remoteOffset_OK()) return false;
		//auto [status, towelrail_req_changed, hotwater_req_changed, remReqTemp] = readRemoteRegisters_OK();
		std::tuple<uint8_t, int8_t, int8_t, uint8_t> regStatus{ readRemoteRegisters_OK() };
		auto status = std::get<0>(regStatus);
		auto towelrail_req_changed = std::get<1>(regStatus);
		auto hotwater_req_changed = std::get<2>(regStatus);
		auto remReqTemp = std::get<3>(regStatus); // is 0 normally.
		loopLogger() << "\tGot RemoteRegister" << L_endl;

		if (status != _OK) {
			logger() << L_time << F("ConsoleController_Thick refreshRegistersOK device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_flush;
			loopLogger() << F("\tBad data") << L_endl;
			// timed-out after this line
			return false;
		}
		auto zoneReqTemp = _zone->currTempRequest();
		auto localReqTemp = reg.get(OLED::R_REQUESTING_ROOM_TEMP); // is zoneReqTemp normally.
		// times-out before this line
		loopLogger() << F("\tState: ") << _state << F(" Remote Req Temp: ") << remReqTemp << " Local RegCopy: " << localReqTemp << L_endl;
		logger() << millis() % 10000 << F("\tState: ") << _state << F(" Remote Req Temp: ") << remReqTemp << " Local RegCopy: " << localReqTemp << L_endl;

		switch (_state) {
		case REM_TR:
			_hasChanged = true;
			_towelRail->setMode(towelrail_req_changed);
			_towelRail->check();
			_state = NO_CHANGE;
			break;
		case REM_HW:
			{
				_hasChanged = true;
				bool scheduledProfileIsActive = !_dhw->advancedToNextProfile();
				bool otherProfileIsHotter = _dhw->nextTempRequest() > _dhw->currTempRequest();
				if (hotwater_req_changed == OLED::e_Auto) {
					if (!scheduledProfileIsActive) _dhw->revertToScheduledProfile();
				}
				else if (otherProfileIsHotter) { // OLED::e_ON
					if (scheduledProfileIsActive)  _dhw->startNextProfile();
					else _dhw->revertToScheduledProfile();
				}
				_state = NO_CHANGE;
			}
			break;
		case AWAIT_REM_ACK_TEMP:
			if (remReqTemp == 0) {
				_state = NO_CHANGE;
			}
			else {
				_hasChanged = true;
				logger() << L_time << index() << F("] Resend Prog Request: ") << localReqTemp << L_endl;
				status = writeReg(OLED::R_REQUESTING_ROOM_TEMP);
			}
			break;
		case REM_REQ_TEMP:
			if (abs(remReqTemp - zoneReqTemp) > 10) {
				logger() << L_time << F("Out-of-range Req Temp sent by Remote: ") << remReqTemp << L_endl;
				logRemoteRegisters();
				writeRegValue(OLED::R_REQUESTING_ROOM_TEMP, zoneReqTemp); // Send remote valid request
				_hasChanged = true;
				_state = AWAIT_REM_ACK_TEMP;
			} else {
				logger() << L_time << millis() % 10000 << F("\tNew Req Temp sent by Remote[") << index() << "] : " << remReqTemp << L_endl;
				profileLogger() << L_time << F("_New Req Temp sent by Remote[") << index() << "]:\t" << remReqTemp << L_endl;
				_hasChanged = true;
				localReqTemp = remReqTemp;
				auto limitRequest = _zone->maxUserRequestTemp(true);
				if (limitRequest < localReqTemp) {
					localReqTemp = limitRequest;
					status = writeRegValue(OLED::R_REQUESTING_ROOM_TEMP, localReqTemp); // Send limit temp
					_state = AWAIT_REM_ACK_TEMP;
					logger() << L_time << millis() % 10000 << F("\tlimitRequest[") << index() << "] : " << localReqTemp << L_endl;
				}
				else {
					status = writeRegValue(OLED::R_REQUESTING_ROOM_TEMP, 0); // Notify remote that request has been registered
					uint8_t remReqT;
					readRegVerifyValue(OLED::R_REQUESTING_ROOM_TEMP, remReqT);
					logger() << millis() % 10000 << F("\tAck_Request: ") << status << " Now: " << remReqT << L_endl;
					_state = NO_CHANGE;
				}
				reg.set(OLED::R_REQUESTING_ROOM_TEMP, localReqTemp);				
				_zone->changeCurrTempRequest(localReqTemp);				
				if (status) logger() << L_time << F("write R_REQUESTING_ROOM_TEMP bad:") << L_endl;
			}
			break;
		case NO_CHANGE:
			if (zoneReqTemp != localReqTemp) { // Zonetemp changed locally
				logger() << L_time << millis() % 10000 << F("\tNew Req Temp sent by Programmer: ") << zoneReqTemp << L_endl;
				profileLogger() << L_time << F("_New Req Temp sent by Programmer:\t") << zoneReqTemp << L_endl;
				reg.set(OLED::R_REQUESTING_ROOM_TEMP, zoneReqTemp);
				status = writeReg(OLED::R_REQUESTING_ROOM_TEMP);
				_state = AWAIT_REM_ACK_TEMP;
				_hasChanged = true;
			}
		}
		auto haveNewData = reg.update(OLED::R_WARM_UP_ROOM_M10, _zone->warmUpTime_m10());
		haveNewData |= reg.update(OLED::R_ON_TIME_T_RAIL, uint8_t(_towelRail->timeToGo()/60));
		haveNewData |= reg.update(OLED::R_WARM_UP_DHW_M10, _dhw->warmUpTime_m10()); // If -ve, in 0-60 mins, if +ve in min_10
		if (_hasChanged || haveNewData) {
			logger() << "\t" << millis() % 10000 << "\tSet F_PROGRAMMER_CHANGED_DATA: changed:" << _hasChanged << " : warmChange" << haveNewData << L_endl;
			status |= writeRegSet(OLED::R_WARM_UP_ROOM_M10, 3);
			auto devFlags = OLED::I2C_Flags_Obj(reg.get(OLED::R_DEVICE_STATE));
			devFlags.set(OLED::F_PROGRAMMER_CHANGED_DATA);
			status |= writeRegValue(OLED::R_DEVICE_STATE, devFlags);
			_hasChanged = false;
		}
		logger() << "\trefreshRegisters_Done-status: " << status << L_endl;
		return status == _OK;
	}

	bool ConsoleController_Thick::remoteOffset_OK() {
		//loopLogger() << L_time << "\tremoteOffset_OK..." << L_endl;
		if (rawRegisters().get(R_SLAVE_REQUESTING_INITIALISATION) & 28) return false;

		uint8_t requestINI_flag = RC_US_REQUESTING_INI << index();
		uint8_t remOffset;
		//loopLogger() << L_time << "\twait_DevicesToFinish..." << L_endl;
		wait_DevicesToFinish(rawRegisters());
		if (readRegVerifyValue(OLED::R_REMOTE_REG_OFFSET, remOffset) != _OK) return false;
		if (remOffset == OLED::NO_REG_OFFSET_SET) {
			_state = REM_REQ_OFFSET;
			auto& ini_state = *rawRegisters().ptr(R_SLAVE_REQUESTING_INITIALISATION);
			ini_state |= requestINI_flag;
			logger() << L_time << "\tRem[" << index() << "] iniReq 255. SetFlagVal: " << requestINI_flag << L_endl;
			return false;
		}
		return true;
	}

	std::tuple<uint8_t, int8_t, int8_t, uint8_t> ConsoleController_Thick::readRemoteRegisters_OK() {
		loopLogger() << "\treadRemoteRegisters..." << L_endl;

		// Lambdas
		auto give_RC_Bus = [this](OLED::I2C_Flags_Obj i2c_status) {
			rawRegisters().set(R_PROG_WAITING_FOR_REMOTE_I2C_COMS, 1);
			i2c_status.clear(OLED::F_PROGRAMMER_CHANGED_DATA);
			i2c_status.set(OLED::F_I2C_NOW);
			writeRegValue(OLED::R_DEVICE_STATE, i2c_status);
		};

		auto reg = registers();
		auto i2c_status = OLED::I2C_Flags_Obj{ reg.get(OLED::R_DEVICE_STATE) };
		loopLogger() << "\twait_Devices..." << L_endl;
		give_RC_Bus(i2c_status); // remote reads TS to its local registers.
		wait_DevicesToFinish(rawRegisters());
		loopLogger() << "\treadRegSet..." << L_endl;
		uint8_t status = readRegSet(OLED::R_ROOM_TEMP, 2);
		if (status != _OK || reg.get(OLED::R_ROOM_TEMP) == 0) {
			logger() << L_time << "\tRC Room Temp[" << index() << "] = 0!" << I2C_Talk::getStatusMsg(status) << L_flush;
#ifndef ZPSIM
			status = _I2C_ReadDataWrong;
#endif
		}

		uint8_t towelrail_req_changed = -1;
		uint8_t hotwater_req_changed = -1;
		uint8_t remReqTemp = 0;
		loopLogger() << "\treadRegVerifyValue..." << L_endl;
		bool chain = status || _state // evaluate until true
			|| (status = readRegVerifyValue(OLED::R_REQUESTING_T_RAIL, towelrail_req_changed))
			|| (_state = reg.update(OLED::R_REQUESTING_T_RAIL, towelrail_req_changed) ? REM_TR : _state)
			|| (status = readRegVerifyValue(OLED::R_REQUESTING_DHW, hotwater_req_changed))
			|| (_state = reg.update(OLED::R_REQUESTING_DHW, hotwater_req_changed) ? REM_HW : _state)
			|| (status = readRegVerifyValue(OLED::R_REQUESTING_ROOM_TEMP, remReqTemp))
			|| (_state = remReqTemp != 0 ? REM_REQ_TEMP : _state);

		loopLogger() << "\treadRemoteRegisters_done: " << status << L_endl;

		return std::tuple<uint8_t, int8_t, int8_t, uint8_t>(status, towelrail_req_changed, hotwater_req_changed, remReqTemp);
	}
}
