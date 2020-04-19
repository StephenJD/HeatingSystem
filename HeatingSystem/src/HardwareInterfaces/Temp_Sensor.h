#pragma once
#include <Arduino.h>
#include <TempSensor.h>
#include "..\LCD_UI\I_Record_Interface.h" // relative path required by Arduino
#include <RDB.h>

namespace HardwareInterfaces {

	class I2C_Temp_Sensor : public TempSensor, public LCD_UI::VolatileData {
	public:
		using TempSensor::TempSensor;
		I2C_Temp_Sensor() = default;
#ifdef ZPSIM
		I2C_Temp_Sensor(I2C_Recovery::I2C_Recover & recover, uint8_t addr, int16_t temp) : TempSensor(recover,addr ) { _lastGood = temp << 8; }
#endif
		// Queries
		bool operator== (const I2C_Temp_Sensor & rhs) const { return _recordID == rhs._recordID; }

		// Modifiers
		void initialise(int recordID, int address) {_recordID = recordID; TempSensor::initialise(address);}

	private:
		RelationalDatabase::RecordID _recordID = 0;
	};

	//extern I2C_Temp_Sensor * tempSensors; // Array of TempSensor provided by client
}