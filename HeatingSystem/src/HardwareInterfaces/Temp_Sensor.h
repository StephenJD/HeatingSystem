#pragma once
#include "Arduino.h"
#include <I2C_Helper.h>

namespace HardwareInterfaces {

	class I2C_Temp_Sensor : public I2C_Helper::I_I2Cdevice {
	public:
		//explicit I2C_Temp_Sensor(int address) : I2C_Helper::I_I2Cdevice(address) {}
		//I2C_Temp_Sensor() = default;
		// Queries
		int8_t get_temp() const;
		int16_t get_fractional_temp() const;
		int lastError() const { return _error; }
		uint8_t setHighRes();
		// Virtual Functions
		uint8_t testDevice(I2C_Helper & i2c, int addr) override;

	protected:
		mutable int16_t lastGood = 100 * 256;
		static int _error;
		#ifdef ZPSIM
			mutable int16_t change = 256;
		#endif
	};

	extern I2C_Temp_Sensor * tempSensors; // Array of TempSensor provided by client
}