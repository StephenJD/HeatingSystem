#include "RemoteConsole.h"
#include "OLED_Master_Display.h"
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
	using namespace OLED_Master_Display;

	RemoteConsole::RemoteConsole(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers) : I_I2Cdevice_Recovery{ recover }, _prog_registers(prog_registers) {}

	void RemoteConsole::initialise(int index, int addr, int roomTS_addr, TowelRail& towelRail, Zone& dhw, Zone& zone, unsigned long& timeOfReset_mS) {
		//logger() << F("RemoteConsole::ini ") << index << L_endl;
		setAddress(addr);
		auto remoteConsole_register = RC_REG_MASTER_US_OFFSET + (R_DISPL_REG_SIZE * index);
		_regOffset = remoteConsole_register;
		setReg(R_DISPL_REG_OFFSET, 0);
		setReg(R_ROOM_TS_ADDR, roomTS_addr);
		_towelRail = &towelRail;
		_dhw = &dhw;
		_zone = &zone;
		_timeOfReset_mS = &timeOfReset_mS;
		//logger() << F("RemoteConsole::ini done. Reg:") << _regOffset + R_DISPL_REG_OFFSET << ":" << _regOffset << L_endl;
	}

	void RemoteConsole::waitForWarmUp() {
		if (*_timeOfReset_mS != 0) {
			auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
			while (!timeOut) {
				//logger() << L_time << F("RemoteConsole Warmup used: ") << timeOut.timeUsed() << L_endl;
				ui_yield();
			}
			*_timeOfReset_mS = 0;
		}
	}

	Error_codes RemoteConsole::testDevice() { // non-recovery test
		if (runSpeed() > 100000) set_runSpeed(100000);
		Error_codes status = _OK;
		uint8_t val1 = 20;
		waitForWarmUp();
		status = I_I2Cdevice::write(R_REQUESTED_ROOM_TEMP + _regOffset, 1, &val1); // non-recovery
		if (status == _OK) status = I_I2Cdevice::read(R_REQUESTED_ROOM_TEMP + _regOffset, 1, &val1); // non-recovery 
		return status;
	}

	uint8_t RemoteConsole::getReg(int reg) const {
		return _prog_registers.getRegister(_regOffset + reg);
	}

	void RemoteConsole::setReg(int reg, uint8_t value) {
		//logger() << F("RemoteConsole::setReg:") << _regOffset + reg << ":" << value << L_endl;
		_prog_registers.setRegister(_regOffset + reg, value);
	}

	uint8_t RemoteConsole::sendSlaveIniData(uint8_t requestINI_flag) {
#ifdef ZPSIM
		uint8_t(&debug)[58] = reinterpret_cast<uint8_t(&)[58]>(*_prog_registers.reg_ptr(0));
		uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(Wire.i2CArr[getAddress()][0]);
#endif
		uint8_t errCode = write(getReg(R_DISPL_REG_OFFSET), 1, &_regOffset);
		errCode |= writeRegisterToConsole(R_ROOM_TS_ADDR);
		//errCode |= writeRegisterToConsole(R_REQUESTED_ROOM_TEMP);
		setReg(R_REQUESTING_T_RAIL, e_ModeIsSet);
		setReg(R_REQUESTING_DHW, e_ModeIsSet);
		setReg(R_REQUESTED_ROOM_TEMP, _zone->currTempRequest());
		setReg(R_WARM_UP_ROOM_M10, 10);
		setReg(R_ON_TIME_T_RAIL, 0);
		setReg(R_WARM_UP_DHW_M10, 2);
		read(R_ROOM_TEMP, 2, _prog_registers.reg_ptr(_regOffset + R_ROOM_TEMP));

		if (errCode == _OK) {
			auto newIniStatus = _prog_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION);
			newIniStatus &= ~requestINI_flag;
			_prog_registers.setRegister(R_SLAVE_REQUESTING_INITIALISATION, newIniStatus);
		}
		//logger() << F("RemoteConsole::sendSlaveIniData()") << i2C().getStatusMsg(errCode) << L_endl;
		return errCode;
	}

	Error_codes  RemoteConsole::writeRegisterToConsole(int reg) {
		auto status = reEnable(); // see if is disabled.
		waitForWarmUp();
		if (status == _OK) {
			status = write(reg, 1, _prog_registers.reg_ptr(_regOffset + reg)); // recovery-write
			if (status) {
				logger() << L_time << F("RemoteConsole Write device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(status) << " at freq: " << L_dec << runSpeed() << L_endl;
			} //else logger() << L_time << F("MixValve Write OK device 0x") << L_hex << getAddress() << L_endl;
		} //else logger() << L_time << F("RemoteConsole Write device 0x") << L_hex << getAddress() << F(" disabled") << L_endl;
		return status;
	}

	void RemoteConsole::refreshRegisters() {
	// Room temp and requests are sent by the console to the Programmer.
	// The programmer does not read them from the console.
	// New request temps initiated by the programmer are sent by the programmer and not read by the console.
	// Warmup-times are read by the console from the programmer.

#ifdef ZPSIM
		uint8_t(&debug)[58] = reinterpret_cast<uint8_t(&)[58]>(*_prog_registers.reg_ptr(0));
		uint8_t(&debugWire)[R_DISPL_REG_SIZE] = reinterpret_cast<uint8_t(&)[R_DISPL_REG_SIZE]>(Wire.i2CArr[getAddress()][0]);
		//logger() << "\nRegisters\n";
		//for (int i = 0; i < SIZE_OF_ALL_REGISTERS; ++i) {
		//	logger() << i << "," << *_prog_registers.reg_ptr(i) << L_endl;
		//}		
#endif		
		auto iniStatus = _prog_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION);
		uint8_t requestINI_flag = RC_US_REQUESTING_INI << index();
		if (iniStatus & requestINI_flag) 
			sendSlaveIniData(requestINI_flag);
		else {
			auto towelRail_Request = getReg(R_REQUESTING_T_RAIL);
			if (towelRail_Request != e_ModeIsSet) {
				_towelRail->setMode(towelRail_Request);
				_towelRail->check();
				setReg(R_REQUESTING_T_RAIL, e_ModeIsSet);
			}
			auto dhwRequest = getReg(R_REQUESTING_DHW);
			if (dhwRequest != e_ModeIsSet) {
				bool currentProfileIsActive = _dhw->currTempRequest() != _dhw->nextTempRequest();
				bool currentProfileIsOn = _dhw->currTempRequest() > _dhw->nextTempRequest();
				if (!currentProfileIsActive && dhwRequest == e_Auto) _dhw->revertToCurrentProfile();
				else if (currentProfileIsActive && currentProfileIsOn && dhwRequest == e_Off) _dhw->startNextProfile();
				else if (currentProfileIsActive && !currentProfileIsOn && dhwRequest == e_On)  _dhw->startNextProfile(); 
				else if (!currentProfileIsActive && _dhw->currTempRequest() > 30 && dhwRequest == e_Off) _dhw->revertToCurrentProfile();
				else if (!currentProfileIsActive && _dhw->currTempRequest() <= 30 && dhwRequest == e_On) _dhw->revertToCurrentProfile();
				setReg(R_REQUESTING_DHW, e_ModeIsSet);
			}
			auto requestedTempRegister = getReg(R_REQUESTED_ROOM_TEMP);
			if (requestedTempRegister != 0) { // Req_Temp register written to by remote console
				auto limitRequest = _zone->maxUserRequestTemp(true);
				if (limitRequest < requestedTempRegister) {
					requestedTempRegister = limitRequest;
					setReg(R_REQUESTED_ROOM_TEMP, requestedTempRegister);
				}
				_zone->changeCurrTempRequest(requestedTempRegister);
			} 
			setReg(R_REQUESTED_ROOM_TEMP, _zone->currTempRequest());
			writeRegisterToConsole(OLED_Master_Display::R_REQUESTED_ROOM_TEMP);
			setReg(R_REQUESTED_ROOM_TEMP, 0);
			setReg(R_WARM_UP_ROOM_M10, _zone->warmUpTime_m10());
			setReg(R_ON_TIME_T_RAIL, uint8_t(_towelRail->timeToGo()/60));
			setReg(R_WARM_UP_DHW_M10, _dhw->warmUpTime_m10()); // If -ve, in 0-60 mins, if +ve in min_10
		}
	}

}
