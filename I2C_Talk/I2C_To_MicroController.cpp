#include "I2C_To_MicroController.h"
#include <I2C_Reset.h>

namespace HardwareInterfaces {
	using namespace I2C_Recovery;
	using namespace i2c_registers;
	using namespace I2C_Talk_ErrorCodes;


	I2C_To_MicroController::I2C_To_MicroController(I2C_Recover& recover, I_Registers& local_registers, int other_microcontroller_address, int localRegOffset, int remoteRegOffset)
		: I_I2Cdevice_Recovery{ recover, other_microcontroller_address }
		, _localRegisters(&local_registers)
		, _localRegOffset(localRegOffset)
		, _remoteRegOffset(remoteRegOffset)
		{}

	void I2C_To_MicroController::initialise(int other_microcontroller_address, int localRegOffset, int remoteRegOffset) {
		I_I2Cdevice_Recovery::setAddress(other_microcontroller_address);
		_localRegOffset = localRegOffset;
		_remoteRegOffset = remoteRegOffset;
		i2C().extendTimeouts(WORKING_SLAVE_BYTE_PROCESS_TIMOUT_uS, STOP_MARGIN_TIMEOUT, 1000);
	}

	Error_codes I2C_To_MicroController::testDevice() { // non-recovery test
		if (runSpeed() > 100000) {
			set_runSpeed(100000);
		}
		Error_codes status = _OK;
		uint8_t wasReg0, testReg0 = 3;
		I2C_Recovery::HardReset::hasWarmedUp(true);
		status = I_I2Cdevice::read(0, 1, &wasReg0); // non-recovery 		
		if (status == _OK) status = I_I2Cdevice::write_verify(0, 1, &testReg0); // non-recovery
		I_I2Cdevice::write_verify(0, 1, &wasReg0); // non-recovery
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
		auto lastReg = _remoteRegOffset + reg + noToWrite - 1;
		if (lastReg != regset.validReg(lastReg)) {
			logger() << L_time << "Out of range reg access: " << lastReg << L_flush;
			return _I2C_RegOutOfRange;
		};
		return write_verify(_remoteRegOffset + reg, noToWrite, regset.ptr(reg));
	}

	Error_codes I2C_To_MicroController::readVerifyReg(int reg) {
		auto regset = registers();
		return read_verify_1byte(_remoteRegOffset + reg, *regset.ptr(reg),2,4); // recovery
	}	
	
	Error_codes I2C_To_MicroController::readRegSet(int reg, int noToRead) {
		auto regset = registers();
		auto lastReg = _remoteRegOffset + reg + noToRead - 1;
		if (lastReg != regset.validReg(lastReg)) {
			logger() << L_time << "Out of range reg access: " << lastReg << L_flush;
			return _I2C_RegOutOfRange;
		};
		return read(_remoteRegOffset + reg, noToRead, regset.ptr(reg)); // recovery
	}	
	
	Error_codes I2C_To_MicroController::readRegVerifyValue(int reg, uint8_t& value) {
		return read_verify_1byte(_remoteRegOffset + reg, value, 2, 4); // recovery
	}

	Error_codes I2C_To_MicroController::getInrangeVal(int regNo, int minVal, int maxVal) {
		auto reg = registers();
		Error_codes status;
		uint8_t regVal;
		I2C_Recovery::HardReset::hasWarmedUp(true);
		auto timeout = Timer_mS(300);
		do {
			//testDevice();
			status = readVerifyReg(regNo); // recovery
			regVal = reg.get(regNo);
			if (regVal >= minVal && regVal <= maxVal) break;
			i2C().begin();
		} while (!timeout);
		auto timeused = timeout.timeUsed();
		if (timeused > 200 && !timeout) logger() << L_time << "Read 0x" << L_hex << getAddress() << " Reg 0x" << regNo << " in mS " << L_dec << timeused << L_endl;

		if (timeused > timeout.period()) {
			logger() << L_time << F("Read 0x") << L_hex << getAddress() << " Bad Reg 0x" << regNo << " was:" << L_dec << regVal << I2C_Talk::getStatusMsg(status) << L_endl;
			status = status == _OK ? _I2C_ReadDataWrong : status;
		}
		return status;
	}

	void wait_DevicesToFinish(i2c_registers::RegAccess reg, int regNo) {
		if (reg.get(regNo) == DEVICE_CAN_WRITE) {
			auto timeout = Timer_mS(300);
			while (!timeout && reg.get(regNo) != DEVICE_IS_FINISHED) {
			}
			//auto delayedBy = timeout.timeUsed();
			//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
			reg.set(regNo, DEVICE_IS_FINISHED);
			if (timeout) logger() << L_time << "wait_DevicesToFinish Timed-out" << L_flush;
		}
	};

}