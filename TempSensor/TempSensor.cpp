#include "TempSensor.h"
#include <I2C_Recover.h>
#include "Logging.h"

using namespace I2C_Talk_ErrorCodes;

namespace HardwareInterfaces {
	const uint8_t DS75LX_Temp = 0x00;  // two bytes must be read. Temp is MS 9 bits, in 0.5 deg increments, with MSB indicating -ve temp.
	const uint8_t DS75LX_Config = 0x01;
	const uint8_t DS75LX_HYST_REG = 0x02;
	//error_codes TempSensor::_error;

	TempSensor::TempSensor(I2C_Recovery::I2C_Recover & recover, int addr) : I_I2Cdevice_Recovery(recover, addr) {
		recover.i2C().setMax_i2cFreq(400000);
	} // initialiser for first array element 
	
	TempSensor::TempSensor(I2C_Recovery::I2C_Recover & recover) : I_I2Cdevice_Recovery(recover) {
		recover.i2C().setMax_i2cFreq(400000);
	}


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
		uint8_t temp[2]; // 1stByte = units, 2nd byte = fraction
		_error = read(DS75LX_Temp, 2, temp);

#ifdef ZPSIM
		//  if (getAddress() == 0x70) _error = _NACK_during_data_send;
			//bool debug = true;
		//_lastGood += change;
		temp[0] = _lastGood / 256;
		if (_lastGood < 2560) change = 256;
		if (_lastGood > 17920) change = -256;
		temp[1] = 0;
#endif
		return checkTemp((temp[0] << 8) + temp[1]);
	}

	auto TempSensor::checkTemp(int16_t newTemp)->I2C_Talk_ErrorCodes::error_codes {
		if (_error == _OK) {
			if (_lastGood != 0 && abs(_lastGood - newTemp) > 1280) {
				_error = _I2C_ReadDataWrong;
				logger() << F("TempError 0x") << L_hex << getAddress() << " . Was " << L_dec << _lastGood << F(" Now: ") << newTemp << L_endl;
			} 
		}
		_lastGood = newTemp;
		return _error;
	}

	error_codes TempSensor::testDevice() { // non-recovery test
		uint8_t temp[2] = {75,0};
		_error = I_I2Cdevice::write_verify(DS75LX_HYST_REG, 2, temp);
		if (_error == _OK) {
			_error = I_I2Cdevice::read(DS75LX_Temp, 2, temp);
			if (_error == _OK) {
				_error = checkTemp((temp[0] << 8) + temp[1]);
			}
		}
		return _error;
	}
}