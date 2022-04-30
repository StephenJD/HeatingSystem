#pragma once

#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_RecoverRetest.h>
#include <PinObject.h>

namespace I2C_Recovery {

	class I_IniFunctor {
		public:
			virtual uint8_t operator()() = 0; // notifies reset has happend.
	};

	class I_TestDevices {
	public:
		virtual I_I2Cdevice_Recovery & getDevice(uint8_t deviceAddr) = 0;
	};

	class HardReset : public I2C_Recover_Retest::I_I2CresetFunctor {
	public:
		HardReset(HardwareInterfaces::Pin_Wag i2c_resetPinWag, HardwareInterfaces::Pin_Wag arduino_resetPin, HardwareInterfaces::Pin_Wag led_indicatorPin)
		{
			_i2c_resetPin = i2c_resetPinWag;
			_arduino_resetPin = arduino_resetPin;
			_led_indicatorPin = led_indicatorPin;
		}
		I2C_Talk_ErrorCodes::Error_codes operator()(I2C_Talk & i2c, int addr) override;
		static void arduinoReset(const char* msg);
		static bool hasWarmedUp(bool wait = false);
	private:
		enum {WARMUP_uS = 3000000};
		static unsigned long _timeOfReset_uS;
		static HardwareInterfaces::Pin_Wag _i2c_resetPin;
		static HardwareInterfaces::Pin_Wag _arduino_resetPin;
		static HardwareInterfaces::Pin_Wag _led_indicatorPin;
	};	
	
	class ResetI2C : public I2C_Recover_Retest::I_I2CresetFunctor {
	public:
		ResetI2C(I2C_Recovery::I2C_Recover_Retest & recover, I_IniFunctor & notify_reset_fn, I_TestDevices & testDevices
			, HardwareInterfaces::Pin_Wag i2c_resetPinWag, HardwareInterfaces::Pin_Wag arduino_resetPin, HardwareInterfaces::Pin_Wag led_indicatorPin);

		I2C_Talk_ErrorCodes::Error_codes operator()(I2C_Talk & i2c, int addr) override;
		void notify_reset() override;
		HardReset hardReset;
	private:
		I_IniFunctor * _notify_reset_fn = 0;
		I_TestDevices * _testDevices = 0;
		I2C_Recover_Retest * _recover = 0;
	};
}