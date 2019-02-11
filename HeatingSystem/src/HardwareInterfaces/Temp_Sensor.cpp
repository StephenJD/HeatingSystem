#include "Temp_Sensor.h"
//#include "I2C_Helper.h"
#include "A__Constants.h"

namespace HardwareInterfaces {
	//I2C_Temp_Sensor * tempSensors;

	int I2C_Temp_Sensor::_error;

	void I2C_Temp_Sensor::initialise(int recID, int address) {
		_recID = recID; 
		setAddress(address);
		readTemperature();
	}


	int8_t I2C_Temp_Sensor::readTemperature() {
		uint8_t temp[2];
		if (_i2C != 0) {
			_error = _i2C->read(_address, DS75LX_Temp, 2, temp);
		}
		else _error = I2C_Helper::_I2C_not_created;
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
		if (_i2C != 0) {
			_i2C->write(_address, DS75LX_Config, 0x60);
		}
		else _error = I2C_Helper::_I2C_not_created;
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

	uint8_t I2C_Temp_Sensor::testDevice(I2C_Helper & i2c, int addr) {
		uint8_t temp[2] = {75,0};
		//Serial.print("Temp_Sensor::testDevice : "); Serial.println(addr);
		return _i2C->write_verify(_address, DS75LX_HYST_REG, 2, temp);
	}

}