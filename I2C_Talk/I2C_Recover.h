#pragma once
#include <Arduino.h>
#include <I2C_Device.h>
class I2C_Talk;
class TwoWire;
//class I_I2Cdevice;

namespace I2C_Recovery {

	class I2C_Recover { // no-recovery base-class
	public:
		I2C_Recover(I2C_Talk& i2C) : _i2C(&i2C) { i2C.begin(); }
		
		// Queries
		const I2C_Talk & i2C() const { return *_i2C; }
		const I_I2Cdevice_Recovery & device() const { return *_device; }
		
		// Modifiers
		void set_I2C_Talk(I2C_Talk & i2C) { _i2C = &i2C; }
		void registerDevice(I_I2Cdevice_Recovery & device) { _device = &device; }
		I2C_Talk & i2C() { return *_i2C; }
		I_I2Cdevice_Recovery & device() { return *_device; }
		
		// Polymorphic Functions for TryAgain
		virtual auto newReadWrite(I_I2Cdevice_Recovery & device)->I2C_Talk_ErrorCodes::Error_codes { return I2C_Talk_ErrorCodes::_OK; }
		virtual bool tryReadWriteAgain(I2C_Talk_ErrorCodes::Error_codes status) {
			//Serial.println(" Default non-recovery");
			return false;
		}
		virtual I_I2Cdevice_Recovery * lastGoodDevice() const { return _device; }
		virtual bool isUnrecoverable() const { return false; }

		// Polymorphic Functions for I2C_Talk
		/*virtual*/ auto findAworkingSpeed()->I2C_Talk_ErrorCodes::Error_codes;
		virtual auto testDevice(int noOfTests, int allowableFailures)->I2C_Talk_ErrorCodes::Error_codes;
		//friend class I_I2C_Scan;
		constexpr I2C_Recover() = default;
	protected:
		// Non-Polymorphic functions for I2C_Recover
		TwoWire & wirePort();
	private:
		I_I2Cdevice_Recovery * _device = 0;
		I2C_Talk * _i2C = 0;
	};
}