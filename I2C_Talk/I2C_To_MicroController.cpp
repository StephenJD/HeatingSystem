#include "I2C_To_MicroController.h"

namespace HardwareInterfaces {
	using namespace I2C_Recovery;
	using namespace i2c_registers;
	using namespace I2C_Talk_ErrorCodes;


	I2C_To_MicroController::I2C_To_MicroController(I2C_Recover& recover, I_Registers& local_registers, int other_microcontroller_address, int localRegOffset, int remoteRegOffset, unsigned long* timeOfReset_mS)
		: I_I2Cdevice_Recovery{ recover, other_microcontroller_address }
		, _localRegisters(&local_registers)
		, _localRegOffset(localRegOffset)
		, _remoteRegOffset(remoteRegOffset)
		, _timeOfReset_mS(timeOfReset_mS)
		{}

	void I2C_To_MicroController::initialise(int other_microcontroller_address, int localRegOffset, int remoteRegOffset, unsigned long& timeOfReset_mS) {
		I_I2Cdevice_Recovery::setAddress(other_microcontroller_address);
		_localRegOffset = localRegOffset;
		_remoteRegOffset = remoteRegOffset;
		_timeOfReset_mS = &timeOfReset_mS;
		i2C().extendTimeouts(15000, STOP_MARGIN_TIMEOUT, 1000);
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
		uint8_t reg0;
		waitForWarmUp();
		status = I_I2Cdevice::read(0, 1, &reg0); // non-recovery 		
		if (status == _OK) status = I_I2Cdevice::write(0, 1, &reg0); // non-recovery
		return status;
	}

	Error_codes  I2C_To_MicroController::writeRegValue(int reg, uint8_t value) {
		return write_verify(_remoteRegOffset + reg, 1, &value); // recovery-write_verify;
	}

	Error_codes I2C_To_MicroController::writeReg(int reg) {
		auto regset = registers();
		return write_verify(_remoteRegOffset + reg, 1, regset.ptr(reg));
	}

	Error_codes I2C_To_MicroController::writeRegSet(int reg, int noToWrite) {
		auto regset = registers();
		return write_verify(_remoteRegOffset + reg, noToWrite, regset.ptr(reg));
	}

	Error_codes I2C_To_MicroController::readReg(int reg) {
		auto regset = registers();
		return read_verify_1byte(_remoteRegOffset + reg, *regset.ptr(reg),2,4); // recovery
	}	
	
	Error_codes I2C_To_MicroController::readRegSet(int reg, int noToRead) {
		auto regset = registers();
		return read(_remoteRegOffset + reg, noToRead, regset.ptr(reg)); // recovery
	}	
	
	Error_codes I2C_To_MicroController::readRegVerifyValue(int reg, uint8_t& value) {
		return read_verify_1byte(_remoteRegOffset + reg, value, 2, 4); // recovery
	}
}