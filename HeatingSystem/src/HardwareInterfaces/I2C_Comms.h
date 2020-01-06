#pragma once

#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_RecoverRetest.h>

namespace HardwareInterfaces {

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
		I2C_Talk_ErrorCodes::error_codes operator()(I2C_Talk & i2c, int addr) override;

		bool initialisationRequired = false;
		unsigned long timeOfReset_mS = 0;
	private:
	};	
	
	class ResetI2C : public I2C_Recovery::I2C_Recover_Retest::I_I2CresetFunctor {
	public:
		ResetI2C(I2C_Recovery::I2C_Recover_Retest & recover, I_IniFunctor & resetFn, I_TestDevices & testDevices);

		I2C_Talk_ErrorCodes::error_codes operator()(I2C_Talk & i2c, int addr) override;
		unsigned long timeOfReset_mS() { return hardReset.timeOfReset_mS; }
		void postResetInitialisation() override;
		HardReset hardReset;
	private:

		I_IniFunctor * _postI2CResetInitialisation;
		I_TestDevices * _testDevices;
		I2C_Recovery::I2C_Recover_Retest * _recover;
	};	
}