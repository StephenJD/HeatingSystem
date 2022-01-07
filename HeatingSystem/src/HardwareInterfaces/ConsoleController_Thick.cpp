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
		logger() << F("ConsoleController_Thick::ini ") << index << " i2cMode: " << console_mode << L_endl;
		auto consoleController_Thick_register = RC_REG_MASTER_US_OFFSET + (OLED::R_DISPL_REG_SIZE * index);
		I2C_To_MicroController::initialise(addr, consoleController_Thick_register, timeOfReset_mS);
		uint8_t remoteReqIni = OLED::NO_REG_OFFSET_SET;
		write(getReg(OLED::R_DISPL_REG_OFFSET), 1, &remoteReqIni);
		setReg(OLED::R_DISPL_REG_OFFSET, 0);
		setReg(OLED::R_ROOM_TS_ADDR, roomTS_addr);
		setReg(OLED::R_MODE, console_mode);
		_towelRail = &towelRail;
		_dhw = &dhw;
		_zone = &zone;
		//logger() << F("ConsoleController_Thick::ini done. Reg:") << _regOffset + R_DISPL_REG_OFFSET << ":" << _regOffset << L_endl;
	}

	bool ConsoleController_Thick::console_mode_is(int flag) const {
		return OLED::ModeFlags(getReg(OLED::R_MODE)).is(flag);
	}

	void ConsoleController_Thick::set_console_mode(uint8_t mode) {
		setReg(OLED::R_MODE, mode);
		sendSlaveIniData(RC_US_REQUESTING_INI << index());
	}

	uint8_t ConsoleController_Thick::sendSlaveIniData(uint8_t requestINI_flag) {
		if (console_mode_is(OLED::e_LCD)) return _OK;
#ifdef ZPSIM
		uint8_t(&debug)[58] = reinterpret_cast<uint8_t(&)[58]>(*regPtr(0));
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()][0]);
#endif
		uint8_t errCode = reEnable(true);
		if (errCode == _OK) {
			uint8_t errCode = write(getReg(OLED::R_DISPL_REG_OFFSET), 1, &_regOffset);
			errCode |= writeRegisterToConsole(OLED::R_ROOM_TS_ADDR);
			errCode |= writeRegisterToConsole(OLED::R_MODE);

			//errCode |= writeRegisterToConsole(R_REQUESTING_ROOM_TEMP);
			setReg(OLED::R_REQUESTING_T_RAIL, OLED::e_ModeIsSet);
			setReg(OLED::R_REQUESTING_DHW, OLED::e_ModeIsSet);
			setReg(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
			setReg(OLED::R_WARM_UP_ROOM_M10, 10);
			setReg(OLED::R_ON_TIME_T_RAIL, 0);
			setReg(OLED::R_WARM_UP_DHW_M10, 2);
			if (console_mode_is(OLED::e_MASTER)) read(OLED::R_ROOM_TEMP, 2, regPtr(OLED::R_ROOM_TEMP));
			else write(OLED::R_ROOM_TEMP, 2, regPtr(OLED::R_ROOM_TEMP));
			writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP);
			if (errCode == _OK) {
				auto newIniStatus = getRawReg(R_SLAVE_REQUESTING_INITIALISATION);
				newIniStatus &= ~requestINI_flag;
				setRawReg(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
			}
		}
		logger() << F("ConsoleController_Thick::sendSlaveIniData()[") << index() << F("] i2CMode: ") << console_mode_is(OLED::e_MASTER) << i2C().getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	Error_codes  ConsoleController_Thick::writeRegisterToConsole(int reg) {
		if (console_mode_is(OLED::e_LCD)) return _OK;
		logger() << F("ConsoleController_Thick::writeRegisterToConsole()[") << index() << F("] i2CMode: ") << console_mode_is(OLED::e_MASTER) << L_endl; // { e_INACTIVE, e_MASTER, e_SLAVE };
		auto status = reEnable(); // see if is disabled.
		waitForWarmUp();
		if (status == _OK) {
			status = write(reg, 1, regPtr(reg)); // recovery-write
			if (status) {
				logger() << L_time << F("ConsoleController_Thick Write device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			}
		} 
		return status;
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
		uint8_t(&debug)[58] = reinterpret_cast<uint8_t(&)[58]>(*regPtr(0));
		uint8_t(&debugWire)[OLED::R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[OLED::R_DISPL_REG_SIZE]>(TwoWire::i2CArr[getAddress()][0]);
#endif	
		auto status = reEnable(); // see if is disabled.
		if (status) return;
		waitForWarmUp();
		auto iniStatus = getRawReg(R_SLAVE_REQUESTING_INITIALISATION);
		uint8_t requestINI_flag = RC_US_REQUESTING_INI << index();
		if (!console_mode_is(OLED::e_MASTER)) {
			uint8_t remOffset;
			read(OLED::R_DISPL_REG_OFFSET, 1, &remOffset);
			if (remOffset == OLED::NO_REG_OFFSET_SET) {
				iniStatus |= requestINI_flag;
			}
		}
		//static auto debugStopIni = true;
		if (iniStatus & requestINI_flag) {
			//if (debugStopIni) return;
			sendSlaveIniData(requestINI_flag);
		} else {
			if (!console_mode_is(OLED::e_MASTER)) {
				status = read(OLED::R_REQUESTING_T_RAIL, 2, regPtr(OLED::R_REQUESTING_T_RAIL)); // recovery-write
				status |= write(OLED::R_ROOM_TEMP, 2, regPtr(OLED::R_ROOM_TEMP));
				if (status) {
					logger() << L_time << F("ConsoleController_Thick refreshRegisters device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
				}
			}
			auto towelRail_Request = getReg(OLED::R_REQUESTING_T_RAIL);
			if (towelRail_Request != OLED::e_ModeIsSet) {
				_hasChanged = true;
				_towelRail->setMode(towelRail_Request);
				_towelRail->check();
				setReg(OLED::R_REQUESTING_T_RAIL, OLED::e_ModeIsSet);
			}
			auto dhwRequest = getReg(OLED::R_REQUESTING_DHW);
			if (dhwRequest != OLED::e_ModeIsSet) {
				_hasChanged = true;
				bool currentProfileIsActive = _dhw->currTempRequest() != _dhw->nextTempRequest();
				bool currentProfileIsOn = _dhw->currTempRequest() > _dhw->nextTempRequest();
				if (!currentProfileIsActive && dhwRequest == OLED::e_Auto) _dhw->revertToCurrentProfile();
				else if (currentProfileIsActive && currentProfileIsOn && dhwRequest == OLED::e_Off) _dhw->startNextProfile();
				else if (currentProfileIsActive && !currentProfileIsOn && dhwRequest == OLED::e_On)  _dhw->startNextProfile();
				else if (!currentProfileIsActive && _dhw->currTempRequest() > 30 && dhwRequest == OLED::e_Off) _dhw->revertToCurrentProfile();
				else if (!currentProfileIsActive && _dhw->currTempRequest() <= 30 && dhwRequest == OLED::e_On) _dhw->revertToCurrentProfile();
				setReg(OLED::R_REQUESTING_DHW, OLED::e_ModeIsSet);
			}
			auto zoneReqTemp = _zone->currTempRequest();
			auto localReqTemp = getReg(OLED::R_REQUESTING_ROOM_TEMP);
			uint8_t remReqTemp;
			read(OLED::R_REQUESTING_ROOM_TEMP, 1, &remReqTemp);
			if (remReqTemp && remReqTemp != zoneReqTemp) { // Req_Temp register written to by remote console
				logger() << L_time << F("New Req Temp sent by Remote: ") << remReqTemp << L_endl;
				_hasChanged = true;
				localReqTemp = remReqTemp;
				auto limitRequest = _zone->maxUserRequestTemp(true);
				if (limitRequest < localReqTemp) {
					localReqTemp = limitRequest;
					setReg(OLED::R_REQUESTING_ROOM_TEMP, localReqTemp);
				}
				else {
					setReg(OLED::R_REQUESTING_ROOM_TEMP, 0);
				}
				_zone->changeCurrTempRequest(localReqTemp);
				writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP); // Notify remote that temp has been changed
			}
			else if (zoneReqTemp != localReqTemp) { // Zonetemp changed locally
				logger() << L_time << F("New Req Temp sent by Programmer: ") << zoneReqTemp << L_endl;
				setReg(OLED::R_REQUESTING_ROOM_TEMP, zoneReqTemp);
				writeRegisterToConsole(OLED::R_REQUESTING_ROOM_TEMP);
			} 
			auto haveNewData = updateReg(OLED::R_REQUESTING_ROOM_TEMP, _zone->currTempRequest());
			haveNewData |= updateReg(OLED::R_WARM_UP_ROOM_M10, _zone->warmUpTime_m10());
			haveNewData |= updateReg(OLED::R_ON_TIME_T_RAIL, uint8_t(_towelRail->timeToGo()/60));
			haveNewData |= updateReg(OLED::R_WARM_UP_DHW_M10, _dhw->warmUpTime_m10()); // If -ve, in 0-60 mins, if +ve in min_10
			if (!console_mode_is(OLED::e_MASTER)) write(OLED::R_WARM_UP_ROOM_M10, 3, regPtr(OLED::R_WARM_UP_ROOM_M10));
			if (haveNewData) write(OLED::R_MODE, static_cast<uint8_t>(OLED::ModeFlags(getReg(OLED::R_MODE)).set(OLED::e_DATA_CHANGED)));
		}
	}

}
