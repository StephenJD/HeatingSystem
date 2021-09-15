#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Recover.h>
#include <I2C_Registers.h>
#include "I2C_Comms.h"



namespace HardwareInterfaces {
	class TowelRail;
	class ThermalStore;
	class Zone;

	class RemoteConsole : public I_I2Cdevice_Recovery {
	public:
		RemoteConsole(I2C_Recovery::I2C_Recover& recover, i2c_registers::I_Registers& prog_registers);
		void initialise(int index, int addr, int roomTS_addr, TowelRail& towelRail, ThermalStore& thermalStore, Zone& zone, unsigned long& timeOfReset_mS);
		
		// Virtual Functions
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;

		uint8_t sendSlaveIniData(uint8_t requestINI_flag);
		uint8_t getReg(int reg) const;
		I2C_Talk_ErrorCodes::Error_codes readRegistersFromConsole(); // returns value
		uint8_t index() const { return (_regOffset - RC_REG_MASTER_US_OFFSET) / OLED_Master_Display::R_DISPL_REG_SIZE; }
		void refreshRegisters();

	private:
		I2C_Talk_ErrorCodes::Error_codes writeRegistersToConsole(int start = OLED_Master_Display::R_REQUESTED_ROOM_TEMP, int end = OLED_Master_Display::R_DISPL_REG_SIZE);
		void waitForWarmUp();
		void setReg(int reg, uint8_t value);

		unsigned long * _timeOfReset_mS = 0;
		i2c_registers::I_Registers& _prog_registers;
		TowelRail* _towelRail = 0;
		ThermalStore* _thermalStore = 0;
		Zone* _zone = 0;

		uint8_t _regOffset = 0;		
	};
}

