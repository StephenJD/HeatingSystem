#include "TempSensor.h"

using namespace I2C_Talk_ErrorCodes;

namespace HardwareInterfaces {
	const uint8_t DS75LX_Temp = 0x00;  // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
	const uint8_t DS75LX_Config = 0x01;
	const uint8_t DS75LX_HYST_REG = 0x02;
	error_codes TempSensor::_error;

	void TempSensor::initialise(int address) {
		setAddress(address);
	}

	uint8_t TempSensor::setHighRes() {
		_error = write(DS75LX_Config, 0x60);
		return _error;
	}

	int8_t TempSensor::get_temp() const {
		return (get_fractional_temp() + 128) / 256;
	}

	int16_t TempSensor::get_fractional_temp() const {
#ifdef TEST_MIX_VALVE_CONTROLLER
		extern S2_byte tempSensors[2];
		return tempSensors[address];
#else
		return _lastGood;
#endif
	}

	error_codes TempSensor::readTemperature() {
		uint8_t temp[2];
		_error = read(DS75LX_Temp, 2, temp);

#ifdef ZPSIM
		//if (getAddress() == 0x70)
			//bool debug = true;
		//_lastGood += change;
		temp[0] = _lastGood / 256;
		if (_lastGood < 2560) change = 256;
		if (_lastGood > 17920) change = -256;
		temp[1] = 0;
#endif
		_lastGood = (temp[0] << 8) + temp[1];
		return _error;
	}

	error_codes TempSensor::testDevice() {
		uint8_t temp[2] = {75,0};
		return write_verify(DS75LX_HYST_REG, 2, temp);
	}
}