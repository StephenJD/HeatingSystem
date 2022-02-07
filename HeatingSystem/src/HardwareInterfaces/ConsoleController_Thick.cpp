#include "ConsoleController_Thick.h"
#include "TowelRail.h"
#include "ThermalStore.h"
#include "Zone.h"

void ui_yield();
namespace arduino_logger {
	Logger& logger();
}
using namespace arduino_logger;

namespace HardwareInterfaces {
	using namespace I2C_Talk_ErrorCodes;
	using OLED = OLED_Thick_Display;

	ConsoleController_Thick::ConsoleController_Thick(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) 
		: I2C_To_MicroController{ recover, prog_registers }
	{}

	void ConsoleController_Thick::initialise(int index, int addr, int roomTS_addr, TowelRail& towelRail, Zone& dhw, Zone& zone, unsigned long& timeOfReset_mS, uint8_t console_mode) {
		logger() << F("ConsoleController_Thick::ini ") << index << " i2cMode: " << console_mode  << L_endl;
		auto consoleController_Thick_register = RC_REG_MASTER_US_OFFSET + (OLED::R_DISPL_REG_SIZE * index);
		I2C_To_MicroController::initialise(addr, consoleController_Thick_register, timeOfReset_mS);
		auto reg = registers();
		reg.set(OLED::R_ROOM_TS_ADDR, roomTS_addr);
		reg.set(OLED::R_MODE, console_mode);
		reg.set(OLED::R_DISPL_REG_OFFSET, 0);
		uint8_t remoteReqIni = OLED::NO_REG_OFFSET_SET;
		write_verify(reg.get(OLED::R_DISPL_REG_OFFSET), 1, &remoteReqIni);
		_towelRail = &towelRail;
		_dhw = &dhw;
		_zone = &zone;
		//logRemoteRegisters();
		logger() << F("ConsoleController_Thick::ini done.") << L_endl;
	}
	uint8_t ConsoleController_Thick::get_console_mode() const {
		return registers().get(OLED::R_MODE);
	}

	bool ConsoleController_Thick::console_mode_is(int flag) const {
		return OLED::ModeFlagsObj(registers().get(OLED::R_MODE)).is(flag);
	}

	void ConsoleController_Thick::set_console_mode(uint8_t mode) {
		registers().set(OLED::R_MODE, mode);
		sendSlaveIniData(RC_US_REQUESTING_INI << index());
	}

	uint8_t ConsoleController_Thick::sendSlaveIniData(uint8_t requestINI_flag) {
#ifdef ZPSIM
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()][0]);
#endif
		uint8_t errCode = reEnable(true);
		if (errCode == _OK) {
			auto reg = registers();
			uint8_t errCode = write_verify(reg.get(OLED::R_DISPL_REG_OFFSET), 1, &_regOffset);
			errCode |= writeRegisterToConsole(OLED::R_ROOM_TS_ADDR);
			errCode |= writeRegisterToConsole(OLED::R_MODE);
			if (errCode) return errCode;

			reg.set(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
			reg.set(OLED::R_WARM_UP_ROOM_M10, 10);
			reg.set(OLED::R_ON_TIME_T_RAIL, 0);
			reg.set(OLED::R_WARM_UP_DHW_M10, 2);
			if (console_mode_is(OLED::e_MASTER)) errCode |= read_verify_1byte(OLED::R_ROOM_TEMP, *reg.ptr(OLED::R_ROOM_TEMP), 2, 4);
			else errCode |= write_verify(OLED::R_ROOM_TEMP, 2, reg.ptr(OLED::R_ROOM_TEMP));
			writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP);
			errCode |= write_verify(OLED::R_REQUESTING_T_RAIL, 2, reg.ptr(OLED::R_REQUESTING_T_RAIL));
			if (errCode == _OK) {
				auto rawReg = rawRegisters();
				auto newIniStatus = rawReg.get(R_SLAVE_REQUESTING_INITIALISATION);
				logger() << L_time << "SendConsIni. IniStatus was:" << newIniStatus;
				newIniStatus &= ~requestINI_flag;
				logger() << " IniStatus now:" << newIniStatus << " sent Offset:" << _regOffset << L_endl;
				rawReg.set(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
			}
		}
		logger() << F("ConsoleController_Thick::sendSlaveIniData()[") << index() << F("] i2CMode: ") << registers().get(OLED::R_MODE) << i2C().getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	Error_codes  ConsoleController_Thick::writeRegisterToConsole(int reg) {
		//logger() << F("ConsoleController_Thick::writeRegisterToConsole()[") << index() << F("] i2CMode: ") << console_mode_is(OLED::e_MASTER) << L_endl; // { e_INACTIVE, e_MASTER, e_SLAVE };
		auto status = reEnable(); // see if is disabled.
		waitForWarmUp();
		if (status == _OK) {
			status = write_verify(reg, 1, registers().ptr(reg)); // recovery-write_verify
			if (status) {
				logger() << L_time << F("ConsoleController_Thick Write device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			}
		} 
		return status;
	}

	void ConsoleController_Thick::logRemoteRegisters() {
		uint8_t regVal;
		logger() << "OfSt" << L_tabs << "MODE" << "TSad" << "RT" << "Fr" << "TRl?" << "HW?" << "Rm?" << "Rmm" << "TRm" << "HWm" << L_endl;
		for (int reg = OLED::R_DISPL_REG_OFFSET; reg < OLED::R_DISPL_REG_SIZE; ++reg) {
			read_verify_1byte(reg, regVal, 2, 4);
			logger() << regVal << ", \t";
		}
		logger() << L_endl;
	}

	void ConsoleController_Thick::refreshRegisters() {
		// New request temps initiated by the programmer are sent by the programmer.
		// In Multi-Master Mode: 
		//		Room temp and requests are sent by the console to the Programmer.
		//		Warmup-times are read by the console from the programmer.
		// In Slave-Mode: 
		//		Requests are read from the console by the Programmer.
		//		Room Temp and Warmup-times are sent to the console by the programmer.

#ifdef ZPSIM
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()][0]);
#endif	
		auto status = reEnable(true); // see if is disabled.
		//if (status) return;
		waitForWarmUp();
		auto iniStatus = rawRegisters().get(R_SLAVE_REQUESTING_INITIALISATION);
		//if (iniStatus > ALL_REQUESTING) {
		//	logger() << L_time << "Bad Console IniStatus : " << iniStatus << L_endl;
		//	iniStatus = ALL_REQUESTING;
		//	rawRegisters().set(R_SLAVE_REQUESTING_INITIALISATION, ALL_REQUESTING);
		//}
		uint8_t requestINI_flag = RC_US_REQUESTING_INI << index();
		//if (!console_mode_is(OLED::e_MASTER)) {
			uint8_t remOffset;
			//if (read_verify_1byte(OLED::R_DISPL_REG_OFFSET, remOffset, 2 , 4) != _OK) return;
			if (read(OLED::R_DISPL_REG_OFFSET, 1, &remOffset) != _OK) return;
			if (remOffset == OLED::NO_REG_OFFSET_SET) {
				logger() << L_time << "Rem[" << index() << "] iniReq 255. SetFlagVal: " << requestINI_flag << L_endl;
				iniStatus |= requestINI_flag;
			}
		//}
		//logger() << L_time << "IniStatus : " << iniStatus << L_endl;
		int8_t towelrail_req_changed = -1;
		int8_t hotwater_req_changed = -1;
		uint8_t regVal;

		//static auto debugStopIni = true;
		if (iniStatus & requestINI_flag) {
			//if (debugStopIni) return;
			sendSlaveIniData(requestINI_flag);
		} else {
			auto reg = registers();
			//if (!console_mode_is(OLED::e_MASTER)) {
				status = read_verify_1byte(OLED::R_REQUESTING_T_RAIL,regVal, 2, 4);
				if (status == _OK && reg.update(OLED::R_REQUESTING_T_RAIL, regVal)) towelrail_req_changed = regVal;
				status |= read_verify_1byte(OLED::R_REQUESTING_DHW, regVal, 2, 4);
				if (status == _OK && reg.update(OLED::R_REQUESTING_DHW, regVal)) hotwater_req_changed = regVal;
			if (!console_mode_is(OLED::e_MASTER)) {
				status |= write_verify(OLED::R_ROOM_TEMP, 2, reg.ptr(OLED::R_ROOM_TEMP));
			}
			if (status) {
				logger() << L_time << F("ConsoleController_Thick refreshRegisters device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
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
			uint8_t remReqTemp;
			status = read_verify_1byte(OLED::R_REQUESTING_ROOM_TEMP, remReqTemp, 2, 4);
			if (status != _OK) {
				logger() << L_time << F("Bad Read Req Temp sent by Remote[") << index() << "] : " << remReqTemp << I2C_Talk::getStatusMsg(status) << L_endl;
			}
			else if (remReqTemp && remReqTemp != zoneReqTemp) { // Req_Temp register written to by remote console
				if (abs(remReqTemp - zoneReqTemp) > 10) {
					logger() << L_time << F("Out-of-range Req Temp sent by Remote: ") << remReqTemp << L_endl;
					logRemoteRegisters();
					reg.set(OLED::R_REQUESTING_ROOM_TEMP, 0);
					writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP);
				}
				else {
					logger() << L_time << F("New Req Temp sent by Remote[") << index() << "] : " << remReqTemp << L_endl;
					_hasChanged = true;
					localReqTemp = remReqTemp;
					auto limitRequest = _zone->maxUserRequestTemp(true);
					if (limitRequest < localReqTemp) {
						localReqTemp = limitRequest;
						reg.set(OLED::R_REQUESTING_ROOM_TEMP, localReqTemp);
					}
					else {
						reg.set(OLED::R_REQUESTING_ROOM_TEMP, 0);
					}
					_zone->changeCurrTempRequest(localReqTemp);
					writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP); // Notify remote that temp has been changed
				}
			}
			else if (zoneReqTemp != localReqTemp) { // Zonetemp changed locally
				logger() << L_time << F("New Req Temp sent by Programmer: ") << zoneReqTemp << L_endl;
				reg.set(OLED::R_REQUESTING_ROOM_TEMP, zoneReqTemp);
				writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP);
			} 
			auto haveNewData = reg.update(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
			haveNewData |= reg.update(OLED::R_WARM_UP_ROOM_M10, _zone->warmUpTime_m10());
			haveNewData |= reg.update(OLED::R_ON_TIME_T_RAIL, uint8_t(_towelRail->timeToGo()/60));
			haveNewData |= reg.update(OLED::R_WARM_UP_DHW_M10, _dhw->warmUpTime_m10()); // If -ve, in 0-60 mins, if +ve in min_10
			if (!console_mode_is(OLED::e_MASTER)) write_verify(OLED::R_WARM_UP_ROOM_M10, 3, reg.ptr(OLED::R_WARM_UP_ROOM_M10));
			if (_hasChanged || haveNewData) {
				OLED::ModeFlagsRef(*reg.ptr(OLED::R_MODE)).set(OLED::e_DATA_CHANGED);
				writeRegisterToConsole(OLED::R_MODE);
			}
			if (status != _OK) read(OLED::R_REQUESTING_ROOM_TEMP, 1, &regVal); // recovery
		}
	}

}
