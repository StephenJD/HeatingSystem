#pragma once

#include "Arduino.h"
#include "I2C_Helper.h"

namespace HardwareInterfaces {

	class I_IniFunctor {
		public:
			virtual uint8_t operator()() = 0;
	};

	class I_TestDevices {
	public:
		virtual I2C_Helper::I_I2Cdevice & getDevice(uint8_t deviceAddr) = 0;
	};

	class HardReset : public I2C_Helper::I_I2CresetFunctor {
	public:
		uint8_t operator()(I2C_Helper & i2c, int addr) override;

		bool initialisationRequired = true;
		unsigned long timeOfReset_mS = 0;
	private:
		bool i2C_is_released();
	};	
	
	class ResetI2C : public I2C_Helper::I_I2CresetFunctor {
	public:
		ResetI2C(I_IniFunctor & resetFn, I_TestDevices & testDevices);

		uint8_t operator()(I2C_Helper & i2c, int addr) override;
		uint8_t speedTestDevice(I2C_Helper & i2c, int addr, I2C_Helper::I_I2Cdevice & device);
		unsigned long timeOfReset_mS() { return hardReset.timeOfReset_mS; }
		HardReset hardReset;
	private:

		I_IniFunctor * _postI2CResetInitialisation;
		I_TestDevices * _testDevices;
	};	
}