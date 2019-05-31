#pragma once
#include "Arduino.h"
#include <I2C_Talk\I2C_Talk.h>
#include <I2C_Talk/I2C_Device.h>
#include <RDB\src\RDB.h>

namespace HardwareInterfaces {

	class I2C_Temp_Sensor : public I_I2Cdevice {
	public:
		I2C_Temp_Sensor(I2C_Talk & i2C) : I_I2Cdevice(i2C) {} // initialiser for first array element 
		I2C_Temp_Sensor() : I_I2Cdevice(i2c_Talk()) {} // allow array to be constructed
#ifdef ZPSIM
		I2C_Temp_Sensor(I2C_Talk & i2C, int16_t temp) : I_I2Cdevice(i2C) { lastGood = temp << 8; }
#endif
		// Queries
		int8_t get_temp() const;
		int16_t get_fractional_temp() const;
		static bool hasError() { return _error != I2C_Talk::_OK; }
		bool operator== (const I2C_Temp_Sensor & rhs) const { return _recID == rhs._recID; }

		// Modifiers
		int8_t readTemperature();
		uint8_t setHighRes();
		// Virtual Functions
		uint8_t testDevice() override;

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