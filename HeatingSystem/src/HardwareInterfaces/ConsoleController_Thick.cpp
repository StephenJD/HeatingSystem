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
		sendSlaveIniData(RC_US_REQUESTING_INI << index());
	}

	uint8_t ConsoleController_Thick::sendSlaveIniData(uint8_t requestINI_flag) {
#ifdef ZPSIM
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()]);
#endif
		loopLogger() << "Console_sendSlaveIniData..." << L_endl;
		uint8_t errCode = reEnable(true);
		if (errCode == _OK) {
			uint8_t errCode = writeRegSet(OLED::R_REMOTE_REG_OFFSET,3);
			if (errCode) return errCode;

			auto reg = registers();
			reg.set(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
			reg.set(OLED::R_WARM_UP_ROOM_M10, 10);
			reg.set(OLED::R_ON_TIME_T_RAIL, 0);
			reg.set(OLED::R_WARM_UP_DHW_M10, 2);
			errCode |= writeRegSet(OLED::R_REQUESTING_T_RAIL, OLED::R_DISPL_REG_SIZE - OLED::R_REQUESTING_T_RAIL);
			if (errCode == _OK) {
				auto rawReg = rawRegisters();
				auto newIniStatus = rawReg.get(R_SLAVE_REQUESTING_INITIALISATION);
				logger() << L_time << "SendConsIni. IniStatus was:" << newIniStatus;
				newIniStatus &= ~requestINI_flag;
				logger() << " IniStatus now:" << newIniStatus << " sent Offset:" << _localRegOffset << L_endl;
				rawReg.set(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
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
		auto rc_OK = true;
		auto reg = registers();

#ifdef ZPSIM
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()]);
#endif	

		loopLogger() << L_time << "refreshRegisters-start" << L_endl;
		if (reEnable(true) != _OK) return false;
		if (!remoteOffset_OK()) return false;
		//auto [status, towelrail_req_changed, hotwater_req_changed, remReqTemp] = readRemoteRegisters_OK();
		std::tuple<uint8_t, int8_t, int8_t, uint8_t> regStatus{ readRemoteRegisters_OK() };
		auto status = std::get<0>(regStatus);
		auto towelrail_req_changed = std::get<1>(regStatus);
		auto hotwater_req_changed = std::get<2>(regStatus);
		auto remReqTemp = std::get<3>(regStatus);

		if (status != _OK) {
			logger() << L_time << F("ConsoleController_Thick refreshRegistersOK device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			return false;
		}

		if (towelrail_req_changed >= 0) {
			_hasChanged = true;
			_towelRail->setMode(towelrail_req_changed);
			_towelRail->check();
		}
		if (hotwater_req_changed >= 0) {
			_hasChanged = true;
			bool scheduledProfileIsActive = !_dhw->advancedToNextProfile();
			bool otherProfileIsHotter = _dhw->nextTempRequest() > _dhw->currTempRequest();
			if (hotwater_req_changed == OLED::e_Auto) {
				if (!scheduledProfileIsActive) _dhw->revertToScheduledProfile();
			} else if (otherProfileIsHotter) { // OLED::e_ON
				if (scheduledProfileIsActive)  _dhw->startNextProfile();
				else _dhw->revertToScheduledProfile();
			}
		}
		auto zoneReqTemp = _zone->currTempRequest();
		auto localReqTemp = reg.get(OLED::R_REQUESTING_ROOM_TEMP);
		if (remReqTemp && remReqTemp != zoneReqTemp) { // Req_Temp register written to by remote console
			if (abs(remReqTemp - zoneReqTemp) > 10) {
				logger() << L_time << F("Out-of-range Req Temp sent by Remote: ") << remReqTemp << L_endl;
				logRemoteRegisters();
				reg.set(OLED::R_REQUESTING_ROOM_TEMP, 0);
				writeReg(OLED::R_REQUESTING_ROOM_TEMP);
			}
			else {
				logger() << L_time << F("New Req Temp sent by Remote[") << index() << "] : " << remReqTemp << L_endl;
				profileLogger() << L_time << F("_New Req Temp sent by Remote[") << index() << "]:\t" << remReqTemp << L_endl;
				_hasChanged = true;
				localReqTemp = remReqTemp;
				auto limitRequest = _zone->maxUserRequestTemp(true);
				auto isOnNextProfile = _zone->advancedToNextProfile();
				if (isOnNextProfile) limitRequest = _zone->currTempRequest();
				if (limitRequest < localReqTemp || isOnNextProfile) {
					localReqTemp = limitRequest;
					reg.set(OLED::R_REQUESTING_ROOM_TEMP, localReqTemp);
				}
				else {
					reg.set(OLED::R_REQUESTING_ROOM_TEMP, 0);
				}
				_zone->changeCurrTempRequest(localReqTemp);
				status = writeReg(OLED::R_REQUESTING_ROOM_TEMP); // Notify remote that temp has been changed
			}
		}
		else if (zoneReqTemp != localReqTemp) { // Zonetemp changed locally
			logger() << L_time << F("New Req Temp sent by Programmer: ") << zoneReqTemp << L_endl;
			profileLogger() << L_time << F("_New Req Temp sent by Programmer:\t") << zoneReqTemp << L_endl;
			reg.set(OLED::R_REQUESTING_ROOM_TEMP, zoneReqTemp);
			status = writeReg(OLED::R_REQUESTING_ROOM_TEMP);
			_hasChanged = true;
		} 
		auto haveNewData = reg.update(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
		haveNewData |= reg.update(OLED::R_WARM_UP_ROOM_M10, _zone->warmUpTime_m10());
		haveNewData |= reg.update(OLED::R_ON_TIME_T_RAIL, uint8_t(_towelRail->timeToGo()/60));
		haveNewData |= reg.update(OLED::R_WARM_UP_DHW_M10, _dhw->warmUpTime_m10()); // If -ve, in 0-60 mins, if +ve in min_10
		if (_hasChanged || haveNewData) {
			status |= writeRegSet(OLED::R_WARM_UP_ROOM_M10, 3);
			auto devFlags = OLED::I2C_Flags_Obj(reg.get(OLED::R_DEVICE_STATE));
			devFlags.set(OLED::F_DATA_CHANGED);
			status |= writeRegValue(OLED::R_DEVICE_STATE, devFlags);
			_hasChanged = false;
		}
		rc_OK &= (status == _OK);
		loopLogger() << "refreshRegisters - done" << L_endl;
		return rc_OK;
	}

	bool ConsoleController_Thick::remoteOffset_OK() {
		if (rawRegisters().get(R_SLAVE_REQUESTING_INITIALISATION) & 28) return false;

		uint8_t requestINI_flag = RC_US_REQUESTING_INI << index();
		uint8_t remOffset;
		wait_DevicesToFinish(rawRegisters());
		if (readRegVerifyValue(OLED::R_REMOTE_REG_OFFSET, remOffset) != _OK) return false;
		if (remOffset == OLED::NO_REG_OFFSET_SET) {
			logger() << L_time << "Rem[" << index() << "] iniReq 255. SetFlagVal: " << requestINI_flag << L_endl;
			return false;
		}
		return true;
	}

	std::tuple<uint8_t, int8_t, int8_t, uint8_t> ConsoleController_Thick::readRemoteRegisters_OK() {
		// Lambdas
		auto give_RC_Bus = [this](OLED::I2C_Flags_Obj i2c_status) {
			rawRegisters().set(R_PROG_STATE, 1);
			i2c_status.set(OLED::F_I2C_NOW);
			writeRegValue(OLED::R_DEVICE_STATE, i2c_status);
		};

		auto reg = registers();
		uint8_t regVal;
		auto i2c_status = OLED::I2C_Flags_Obj{ reg.get(OLED::R_DEVICE_STATE) };
		give_RC_Bus(i2c_status); // remote reads TS and writes to programmer.
		wait_DevicesToFinish(rawRegisters());
		uint8_t status = readRegSet(OLED::R_ROOM_TEMP, 2); // don't rely on console writing to the registers.
		if (reg.get(OLED::R_ROOM_TEMP) == 0) {
			logger() << L_time << "RC Room Temp[" << index() << "] = 0!" << L_endl;
#ifndef ZPSIM
			status = _I2C_ReadDataWrong;
#endif
		}

		int8_t towelrail_req_changed = -1;
		int8_t hotwater_req_changed = -1;
		uint8_t remReqTemp;

		status |= readRegVerifyValue(OLED::R_REQUESTING_T_RAIL, regVal);
		if (status == _OK && reg.update(OLED::R_REQUESTING_T_RAIL, regVal)) towelrail_req_changed = regVal;
		status |= readRegVerifyValue(OLED::R_REQUESTING_DHW, regVal);
		if (status == _OK && reg.update(OLED::R_REQUESTING_DHW, regVal)) hotwater_req_changed = regVal;
		status |= readRegVerifyValue(OLED::R_REQUESTING_ROOM_TEMP, remReqTemp);

		return std::tuple<uint8_t, int8_t, int8_t, uint8_t>(status, towelrail_req_changed, hotwater_req_changed, remReqTemp);
	}
}
