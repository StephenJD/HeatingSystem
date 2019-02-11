#pragma once
#include "Arduino.h"
#include <I2C_Helper.h>
#include <RDB\src\RDB.h>

namespace HardwareInterfaces {

	class I2C_Temp_Sensor : public I2C_Helper::I_I2Cdevice {
	public:
		I2C_Temp_Sensor() = default;
#ifdef ZPSIM
		I2C_Temp_Sensor(int16_t temp) { lastGood = temp << 8; }
#endif
		// Queries
		int8_t get_temp() const;
		int16_t get_fractional_temp() const;
		static bool hasError() { return _error != I2C_Helper::_OK; }
		bool operator== (const I2C_Temp_Sensor & rhs) const { return _recID == rhs._recID; }

		// Modifiers
		int8_t readTemperature();
		uint8_t setHighRes();
		// Virtual Functions
		uint8_t testDevice(I2C_Helper & i2c, int addr) override;

		void initialise(int recID, int address);

	protected:
		int16_t lastGood = 20 * 256;
		static int _error;
		#ifdef ZPSIM
			mutable int16_t change = 256;
		#endif
	private:
		RelationalDatabase::RecordID _recID = 0;
	};

	//extern I2C_Temp_Sensor * tempSensors; // Array of TempSensor provided by client
}