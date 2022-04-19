#pragma once

#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_RecoverRetest.h>
#include <LocalKeypad.h>
#include <OLED_Thick_Display.h>
#include <Mix_Valve.h>

namespace HardwareInterfaces {

	void wait_DevicesToFinish(i2c_registers::RegAccess reg);

	class I_IniFunctor {
		public:
			virtual uint8_t operator()() = 0; // performs post-reset initialisation. Return 0 for OK 
	};

	class I_TestDevices {
	public:
		virtual I_I2Cdevice_Recovery & getDevice(uint8_t deviceAddr) = 0;
	};

	class HardReset : public I2C_Recovery::I2C_Recover_Retest::I_I2CresetFunctor {
	public:
		HardReset(Pin_Wag resetPinWag) : _resetPinWag(resetPinWag) {}
		I2C_Talk_ErrorCodes::Error_codes operator()(I2C_Talk & i2c, int addr) override;
		static void arduinoReset(const char* msg);
		bool initialisationRequired = false;
		unsigned long timeOfReset_mS = 1; // must be non-zero at startup
	private:
		Pin_Wag _resetPinWag;
	};	
	
	class ResetI2C : public I2C_Recovery::I2C_Recover_Retest::I_I2CresetFunctor {
	public:
		ResetI2C(I2C_Recovery::I2C_Recover_Retest & recover, I_IniFunctor & resetFn, I_TestDevices & testDevices, Pin_Wag resetPinWag);

		I2C_Talk_ErrorCodes::Error_codes operator()(I2C_Talk & i2c, int addr) override;
		unsigned long timeOfReset_mS() { return hardReset.timeOfReset_mS; }
		void postResetInitialisation() override;
		HardReset hardReset;
	private:

		I_IniFunctor * _postI2CResetInitialisation = 0;
		I_TestDevices * _testDevices = 0;
		I2C_Recovery::I2C_Recover_Retest * _recover = 0;
	};

	enum Register_Constants {
		R_SLAVE_REQUESTING_INITIALISATION
		, R_PROG_STATE
		, PROG_REG_MV0_OFFSET
		, PROG_REG_MV1_OFFSET = PROG_REG_MV0_OFFSET + Mix_Valve::MV_VOLATILE_REG_SIZE
		, PROG_REG_RC_US_OFFSET = PROG_REG_MV0_OFFSET + Mix_Valve::MV_VOLATILE_REG_SIZE * NO_OF_MIXERS
		, PROG_REG_RC_DS_OFFSET = PROG_REG_RC_US_OFFSET + OLED_Thick_Display::R_DISPL_REG_SIZE
		, PROG_REG_RC_FL_OFFSET = PROG_REG_RC_DS_OFFSET + OLED_Thick_Display::R_DISPL_REG_SIZE
		, SIZE_OF_ALL_REGISTERS = PROG_REG_RC_FL_OFFSET + OLED_Thick_Display::R_DISPL_REG_SIZE
		, MV0_REG_OFFSET = 0
		, MV1_REG_OFFSET = MV0_REG_OFFSET + Mix_Valve::MV_ALL_REG_SIZE
	};

	enum SlaveRequestIni {
		NO_INI_REQUESTS
		, MV_US_REQUESTING_INI = 1
		, MV_DS_REQUESTING_INI = 2
		, RC_US_REQUESTING_INI = 4
		, RC_DS_REQUESTING_INI = 8
		, RC_FL_REQUESTING_INI = 16
		, ALL_REQUESTING = (RC_FL_REQUESTING_INI * 2) - 1
	};
}