#include "Temp_Sensor.h"
#include "A__Constants.h"

namespace HardwareInterfaces {
	using namespace I2C_Talk_ErrorCodes;

	int I2C_Temp_Sensor::_error;

	void I2C_Temp_Sensor::initialise(int recID, int address) {
		_recID = recID; 
		setAddress(address);
	}


	int8_t I2C_Temp_Sensor::readTemperature() {
		uint8_t temp[2];
		_error = read(DS75LX_Temp, 2, temp);

#ifdef ZPSIM
		lastGood += change;
		temp[0] = lastGood / 256;
		if (lastGood < 7680) change = 256;
		if (lastGood > 17920) change = -256;
		temp[1] = 0;
#endif
		lastGood = (temp[0] << 8) | temp[1];
		return _error;
	}


	uint8_t I2C_Temp_Sensor::setHighRes() {
		_error = write(DS75LX_Config, 0x60);
		return _error;
	}

	int8_t I2C_Temp_Sensor::get_temp() const {
		return (get_fractional_temp() + 128) / 256;
	}

	int16_t I2C_Temp_Sensor::get_fractional_temp() const {
#ifdef TEST_MIX_VALVE_CONTROLLER
		extern S2_byte tempSensors[2];
		return tempSensors[address];
#else
		return lastGood;
#endif
	}

	error_codes I2C_Temp_Sensor::testDevice() {
		uint8_t temp[2] = {75,0};
		//Serial.print("Temp_Sensor::testDevice : "); Serial.println(addr);
		return write_verify(DS75LX_HYST_REG, 2, temp);
	}

}