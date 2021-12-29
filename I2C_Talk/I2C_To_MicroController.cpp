#include "I2C_To_MicroController.h"

namespace HardwareInterfaces {
	using namespace I2C_Recovery;
	using namespace i2c_registers;
	using namespace I2C_Talk_ErrorCodes;


	I2C_To_MicroController::I2C_To_MicroController(I2C_Recover& recover, I_Registers& prog_registers, int address, int regOffset, unsigned long* timeOfReset_mS)
		: I_I2Cdevice_Recovery{ recover, address }
		, _localRegisters(&prog_registers)
		, _regOffset(regOffset)
		, _timeOfReset_mS(timeOfReset_mS)
		{}

	void I2C_To_MicroController::initialise(int address, int regOffset, unsigned long& timeOfReset_mS) {
		I_I2Cdevice_Recovery::setAddress(address);
		_regOffset = regOffset;
		_timeOfReset_mS = &timeOfReset_mS;
		i2C().extendTimeouts(15000, 6, 1000);
	}

	void I2C_To_MicroController::waitForWarmUp() const {
		if (*_timeOfReset_mS != 0) {
			auto timeOut = Timer_mS(*_timeOfReset_mS + 3000UL - millis());
			while (!timeOut) {
				ui_yield();
			}
			*_timeOfReset_mS = 0;
		}
	}

	Error_codes I2C_To_MicroController::testDevice() { // non-recovery test
		if (runSpeed() > 100000) set_runSpeed(100000);
		Error_codes status = _OK;
		uint8_t val1 = 55;
		waitForWarmUp();
		status = I_I2Cdevice::write(0, 1, &val1); // non-recovery
		if (status == _OK) status = I_I2Cdevice::read(0, 1, &val1); // non-recovery 
		return status;
	}

}