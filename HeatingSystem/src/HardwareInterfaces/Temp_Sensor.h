#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include <RDB.h>

namespace HardwareInterfaces {

	class I2C_Temp_Sensor : public I_I2Cdevice_Recovery, public LCD_UI::VolatileData {
	public:
		using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
		I2C_Temp_Sensor() = default;
#ifdef ZPSIM
		I2C_Temp_Sensor(I2C_Recovery::I2C_Recover & recover, uint8_t addr, int16_t temp) : I_I2Cdevice_Recovery(recover,addr ) { _lastGood = temp << 8; }
#endif
		// Queries
		int8_t get_temp() const;
		int16_t get_fractional_temp() const;
		static bool hasError() { return _error != I2C_Talk_ErrorCodes::_OK; }
		bool operator== (const I2C_Temp_Sensor & rhs) const { return _recordID == rhs._recordID; }

		// Modifiers
		int8_t readTemperature();
		uint8_t setHighRes();
		// Virtual Functions
		I2C_Talk_ErrorCodes::error_codes testDevice() override;

		void initialise(int recordID, int address);

	protected:
		int16_t _lastGood = 16 * 256;
		static int _error;
		#ifdef ZPSIM
			mutable int16_t change = 256;
		#endif
	private:
		RelationalDatabase::RecordID _recordID = 0;
	};

	//extern I2C_Temp_Sensor * tempSensors; // Array of TempSensor provided by client
}