#pragma once
#include <Arduino.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

//class I2C_Talk;
namespace HardwareInterfaces {

	class TempSensor : public I_I2Cdevice_Recovery {
	public:
		using I_I2Cdevice_Recovery::I_I2Cdevice_Recovery;
		TempSensor(I2C_Recovery::I2C_Recover & recover, int addr);
		TempSensor(I2C_Recovery::I2C_Recover & recover);
		TempSensor() = default;
#ifdef ZPSIM
		TempSensor(I2C_Recovery::I2C_Recover & recover, uint8_t addr, int16_t temp) : I_I2Cdevice_Recovery(recover, addr) { _lastGood = temp << 8; }
		void setTestTemp(uint8_t temp) {_lastGood = temp << 8;}
		void setTestTemp(double temp) {_lastGood = temp*256;}
#endif
		// Queries
		int8_t get_temp() const;
		int16_t get_fractional_temp() const;
		/*static*/ bool hasError() { return _error != I2C_Talk_ErrorCodes::_OK; }
		/*static*/ I2C_Talk_ErrorCodes::error_codes lastError() { return _error; }

		// Modifiers
		I2C_Talk_ErrorCodes::error_codes readTemperature();
		uint8_t setHighRes();
		// Virtual Functions
		I2C_Talk_ErrorCodes::error_codes testDevice() override;

		void initialise(int address);

	protected:
		auto checkTemp(int16_t newTemp)->I2C_Talk_ErrorCodes::error_codes;
		mutable int16_t _lastGood = 16 * 256;
		/*static*/ I2C_Talk_ErrorCodes::error_codes _error = I2C_Talk_ErrorCodes::_OK;
#ifdef ZPSIM
		mutable int16_t change = 256;
#endif
	};

}
