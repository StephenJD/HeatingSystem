#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_Registers.h>
#include <I2C_To_MicroController.h>
#include "I2C_Comms.h"
#include "OLED_Thick_Display.h"
#include <tuple>

namespace HardwareInterfaces {
	class TowelRail;
	class ThermalStore;
	class Zone;

	class ConsoleController_Thick : public I2C_To_MicroController {
	public:
		ConsoleController_Thick(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers);
		void initialise(int index, int addr, int roomTS_addr, TowelRail& towelRail, Zone& dhw, Zone& zone, uint8_t console_mode);
		
		uint8_t sendSlaveIniData(uint8_t requestINI_flag);
		uint8_t index() const { return (_localRegOffset - PROG_REG_RC_US_OFFSET) / OLED_Thick_Display::R_DISPL_REG_SIZE; }
		bool refreshRegistersOK();
		bool console_mode_is(int) const;
		void set_console_mode(uint8_t mode);
		uint8_t get_console_mode() const;
		bool hasChanged() {
			bool changed = _hasChanged; _hasChanged = false; return changed;
		}
	private:
		enum State {NO_CHANGE, AWAIT_REM_ACK_TEMP, REM_REQ_TEMP, REM_TR, REM_HW};
		bool remoteOffset_OK();
		std::tuple<uint8_t, int8_t, int8_t, uint8_t> readRemoteRegisters_OK();
		void logRemoteRegisters();
		bool _hasChanged = false;
		TowelRail* _towelRail = 0;
		Zone* _dhw = 0;
		Zone* _zone = 0;
		State _state = NO_CHANGE;
	};
}

