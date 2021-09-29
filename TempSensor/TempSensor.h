#pragma once
#include <Arduino.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

//class I2C_Talk;
namespace HardwareInterfaces {

	class TempSensor : public I_I2Cdevice_Recovery {
	public:
		using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
		TempSensor(I2C_Recovery::I2C_Recover & recover, uint8_t addr, int16_t temp) : I_I2Cdevice_Recovery(recover, addr) { _lastGood = temp << 8; }
		TempSensor(I2C_Recovery::I2C_Recover & recover, int addr);
		TempSensor(I2C_Recovery::I2C_Recover & recover);
		TempSensor() = default;

		// Queries
		int8_t get_temp() const;
		int16_t get_fractional_temp() const;
		bool hasError() { return _error != I2C_Talk_ErrorCodes::_OK; }
		I2C_Talk_ErrorCodes::Error_codes lastError() { return _error; }

		// Modifiers
		I2C_Talk_ErrorCodes::Error_codes readTemperature();
		uint8_t setHighRes();
		// Virtual Functions
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;

		void initialise(int address);

	protected:
		auto checkTemp(int16_t newTemp)->I2C_Talk_ErrorCodes::Error_codes;
		mutable int16_t _lastGood = 16 * 256;
		I2C_Talk_ErrorCodes::Error_codes _error = I2C_Talk_ErrorCodes::_OK;
#ifdef ZPSIM
		mutable int16_t change = 256;
	//public:
		//void setTemp(int temp) {_lastGood = temp * 256;}
#endif
	};

}
