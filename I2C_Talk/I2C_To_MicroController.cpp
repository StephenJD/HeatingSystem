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
	Error_codes I2C_To_MicroController::writeOnly_RegValue(int reg, uint8_t value) {
		return write(_remoteRegOffset + reg, 1, &value); // recovery-write;
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
			status = readVerifyReg(regNo); // recovery
			regVal = reg.get(regNo);
			if (regVal >= minVal && regVal <= maxVal) break;
			i2C().begin();
			//testDevice();
		} while (!timeout);
		auto timeused = timeout.timeUsed();
		if (timeused > 200 && !timeout) logger() << L_time << "Read 0x" << L_hex << getAddress() << " Reg 0x" << regNo << " in mS " << L_dec << timeused << L_endl;

		if (timeused > timeout.period()) {
			logger() << L_time << F("Read 0x") << L_hex << getAddress() << " Bad Reg 0x" << regNo << " was:" << L_dec << regVal << I2C_Talk::getStatusMsg(status) << L_endl;
			status = status == _OK ? _I2C_ReadDataWrong : status;
		}
		return status;
	}


	bool I2C_To_MicroController::handShake_send(uint8_t remoteRegNo, const uint8_t data) {
		/* Hand-shake: DATA_READY = 0x40, DATA_READ = 0x80, EXCHANGE_COMPLETE = 0xC0
		* We need to know we have sucessfully written to a remote,
		* So we write then read-back. If the read-back gives the wrong result we write again.
		* But the read-back might have failed, causing us to write again and making the remote think we have sent two messages.
		* So the remote changes its data to show it has seen it. When we read-back we look for the changed data.
		* Once we see the changed data we know it has been seen. But it might take a while before we are successful at reading the changed data.
		* So we might keep sending data after it has been seen. The remote needs to know how long to wait for the same data before moving on.
		* It is not safe for the remote to become master until we have stopped resending data.
		* So we need to tell the remote we have seen the changed data! So we send EXCHANGE_COMPLETE.
		* The remote will timeout if it doesn't get EXCHANGE_COMPLETE, so how long do we send it for? - up to 100mS. After that we must let the remote become master.
		* When remote gets EXCHANGE_COMPLETE it changes its data to DATA_READY. But it knows we will stop sending after 100mS.
		do {
			send data + DATA_READY // when receiver sees DATA_READY it sets DATA_READ and clears DATA_READY
			read data + flags // receiver will not act on new valid-flag if ready-flag is set.
		} while (read != send OR flags != DATA_READ);
		do {
			send data + EXCHANGE_COMPLETE // when receiver sees EXCHANGE_COMPLETE it clears DATA_READ
			read data + flags 
		} while (read != send OR flags != DATA_READY);
		new data can be sent.
		*/			
		auto timeout = Timer_mS(300);
		const uint8_t SEND_DATA = data | DATA_READY & ~DATA_READ;
		const uint8_t COMPLETE_DATA = data | EXCHANGE_COMPLETE;
		const uint8_t RECEIVE_OK = data | DATA_READ & ~DATA_READY;
		uint8_t read_data;
		do {
			writeOnly_RegValue(remoteRegNo, SEND_DATA);
			readRegVerifyValue(remoteRegNo, read_data);
			if (read_data == RECEIVE_OK) break;
			i2C().begin();
		} while (!timeout);
		auto timeused = timeout.timeUsed();
		if (timeused > 200 && !timeout) logger() << L_time << F("send_data 0x") << L_hex << getAddress() << F(" Reg 0x") << remoteRegNo << F(" in mS ") << L_dec << timeused << L_endl;

		if (timeused > timeout.period()) {
			logger() << L_time << F("send_data 0x") << L_hex << getAddress() << " Bad Reg 0x" << remoteRegNo << " Read: 0x" << read_data << L_endl;
			return false;
		}
		timeout.restart();
		do {
			writeOnly_RegValue(remoteRegNo, COMPLETE_DATA);
			readRegVerifyValue(remoteRegNo, read_data);
			if (read_data == SEND_DATA) break;
			i2C().begin();
		} while (!timeout);
		timeused = timeout.timeUsed();
		writeOnly_RegValue(remoteRegNo, RECEIVE_OK);

		if (timeused > 200 && !timeout) logger() << L_time << "confirm_data 0x" << L_hex << getAddress() << " Reg 0x" << remoteRegNo << " in mS " << L_dec << timeused << L_endl;

		if (timeused > timeout.period()) {
			logger() << L_time << F("confirm_data 0x") << L_hex << getAddress() << " Bad Reg 0x" << remoteRegNo << " Read: 0x" << read_data << L_endl;
			return false;
		}
		return true;
	}

	bool I2C_To_MicroController::give_I2C_Bus(i2c_registers::RegAccess localReg, uint8_t localRegNo, uint8_t remoteRegNo, const uint8_t i2c_status) {
		localReg.set(localRegNo, DEVICE_CAN_WRITE);
		return handShake_send(remoteRegNo, i2c_status); // top-two bits (x,x,...) used in hand-shaking
	}

	bool I2C_To_MicroController::wait_DevicesToFinish(i2c_registers::RegAccess localReg, int regNo) {
		bool hasFinished = true;
		// DATA_READY = 0x40, DATA_READ = 0x80, EXCHANGE_COMPLETE = 0xC0
		if (localReg.get(regNo) == DEVICE_CAN_WRITE) {
			auto timeout = Timer_mS(300);
			while (!timeout && localReg.get(regNo) != DEVICE_IS_FINISHED) {
				//i2C().begin();
			}
			//auto delayedBy = timeout.timeUsed();
			//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
			localReg.set(regNo, DEVICE_IS_FINISHED);
			if (timeout) {
				hasFinished = false;
				logger() << L_time << F("wait_DevicesToFinish 0x") << L_hex << getAddress() << " Timed-out" << L_flush;
			}
		}
		return hasFinished;
	}
}