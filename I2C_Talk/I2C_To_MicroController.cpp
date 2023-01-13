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
		return read_verify_1byte(_remoteRegOffset + reg, *regset.ptr(reg), 2, 4); // recovery
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
		/*
		* Two soldiers, Fire & Scout, need to exchange messages at night to arrange covering fire on the scout's position.
	* Scout asks for Fire cover.
		* 1. Scout keeps signalling to Fire saying "Cover me" or times-out after 30mS.
		* 2. Fire sees some of Scout's "Cover me" signals and keeps replying "Ready" until he sees "Evacuating" or times-out after 30mS.
		* 3. If Scout sees "Ready" he sends one "Evacuating" signal.
	* Scout waits for blast.
		* 4. Scout then evacuates and waits for "Fired" up to 300mS. After that he can assume no fire is coming.
		* 5. If Fire sees "Evacuating" before the time-out he fires.
	* Fire sends the "All Clear"
		* 6. After firing or time-out, Fire keeps signalling for up to 300mS "All Clear" until he sees "Hit" or "Missed".
		* 7. Whilst Scout sees "All Clear" he keeps sending "Hit" or times-out after 30mS.
		* 8. Fire keeps sending "All Clear" until he sees "Hit" or times-out after 30mS. Then sends one "No More Firing".
	* Fire assumes Scout is happy with the result
		* 9. After seeing "No More Firing" Scout returns to his position.
		* 10. If Scout doesn't see "No More Firing" he returns to his position after 300mS timeout.
		*/
		/*
	* A tells B it can become master
		* 1. When A wants to send data it keeps sending DATA_SENT until it sees DATA_READ.
		* 2. Whilst B sees DATA_SENT it keeps sending DATA_READ.
		* 3. A keeps sending until it sees DATA_READ (or it times-out). Then it sends one "EXCHANGE_COMPLETE".
	* A assumes B is now Master
		* 4. A then waits to get DATA_SENT from B for up to 300mS.
		* 5. If B sees "EXCHANGE_COMPLETE" it processes its data (takes Master control).
	* B tells A it has finished as Master.
		* 6. When finished as Master, B keeps sending DATA_SENT to A to say it is completed, until it sees DATA_READ (or it times-out 300mS after ReadTime).
		* 7. Whilst A sees DATA_SENT it keeps sending DATA_READ.
		* 8. B keeps sending DATA_SENT until it sees DATA_READ (or it times-out 300mS after start as Master). Then it sends one "EXCHANGE_COMPLETE".
	* B assumes A is now Master
		* 9. After seeing "EXCHANGE_COMPLETE" A becomes Master.
		* 10. If A doesn't see "EXCHANGE_COMPLETE" it resumes Master role after 300mS timeout.
		*/
		/* Hand-shake: DATA_SENT = 0x40, DATA_READ = 0x80, EXCHANGE_COMPLETE = 0xC0
		do { // A tells B it can be Master
			send data + DATA_SENT // when receiver sees DATA_SENT it sets DATA_READ and clears DATA_SENT
			read data + flags // receiver will not act on new valid-flag if ready-flag is set.
			if (read == send AND flags == DATA_READ) break;
		} while (!timeout_300mS);
		send data + EXCHANGE_COMPLETE;
		// A assumes B is Master
		timeout = 300mS
		do { // A reads local register
			if (read == EXCHANGE_COMPLETE) break;
			if (read = DATA_SENT) read = DATA_READ;
		} while (!timeout_300mS);
		// A is Master
		*/

		//DEVICE_CAN_WRITE = 0x38, DEVICE_IS_FINISHED = 0x07 /* 00,111,000 : 00,000,111 */
		//	, DATA_SENT = 0xC0, DATA_READ = 0x40, EXCHANGE_COMPLETE = 0x80 /* 11,000,000 : 01,000,000 : 10,000,000 */
		//	, HANDSHAKE_MASK = 0xC0, DATA_MASK = uint8_t(~HANDSHAKE_MASK) /* 11,000,000 : 00,111,111 */

		const uint8_t SEND_DATA = (data & DATA_MASK) | DATA_SENT;
		uint8_t read_data;
		auto timeout = Timer_mS(300);
		do {
			readRegVerifyValue(remoteRegNo, read_data);
			logger() << L_time << F("send_data 0x") << L_hex << getAddress() << F(" Reg ") << L_dec << _remoteRegOffset + remoteRegNo << F(" read: 0x") << L_hex << read_data << L_endl;
			if ((read_data & HANDSHAKE_MASK) == DATA_READ) break;
			writeOnly_RegValue(remoteRegNo, SEND_DATA);
		} while (!timeout);
		auto timeused = timeout.timeUsed();
		//if (timeused > 200 && !timeout) 
		if (!timeout) {
			logger() << L_time << F("send_data 0x") << L_hex << getAddress() << F(" Reg ") << L_dec << _remoteRegOffset + remoteRegNo << F(" OK mS ") << timeused << L_endl;
		}

		if (timeused > timeout.period()) {
			logger() << L_time << F("send_data 0x") << L_hex << getAddress() << " Timeout Reg " << L_dec << _remoteRegOffset + remoteRegNo << " Read: 0x" << L_hex << read_data << L_endl;
			return false;
		}
		return writeOnly_RegValue(remoteRegNo, (data & DATA_MASK) | EXCHANGE_COMPLETE) == _OK;
	}

	bool I2C_To_MicroController::give_I2C_Bus(i2c_registers::RegAccess localReg, uint8_t localRegNo, uint8_t remoteRegNo, const uint8_t i2c_status) {
		// top-two bits (x,x,...) used in hand-shaking
		if (handShake_send(remoteRegNo, i2c_status)) {
			localReg.set(localRegNo, DEVICE_CAN_WRITE); // must NOT be EXCHANGE_COMPLETE otherwise wait will exit immediatly!!!
			return true;
		}
		return false;
	}

	bool I2C_To_MicroController::wait_DevicesToFinish(i2c_registers::RegAccess localReg, int regNo) {
		/* A assumes B is Master
		timeout = 300mS
		do { // A reads local register
			if (read == EXCHANGE_COMPLETE) break;
			if (read = DATA_SENT) read = DATA_READ;
		} while (!timeout_300mS);
		// A is Master
		*/

		// DEVICE_CAN_WRITE = 0x38, DEVICE_IS_FINISHED = 0x07 /* 00,111,000 : 00,000,111 */
		//, DATA_SENT = 0xC0, DATA_READ = 0x40, EXCHANGE_COMPLETE = 0x80 /* 11,000,000 : 01,000,000 : 10,000,000 */
		//, HANDSHAKE_MASK = 0xC0, DATA_MASK = ~HANDSHAKE_MASK /* 11,000,000 : 00,111,111 */

		i2C().begin();
		if ((millis() / 10000) % 2 == 0) {
			if ((localReg.get(regNo) & DATA_MASK) == DEVICE_CAN_WRITE) { // must get captured immediatly, cos gets called immediatly after giving bus away
				//logger() << "receive_handshakeData" << L_endl;
				/*auto isGood = */receive_handshakeData(*localReg.ptr(regNo));
				//localReg.set(regNo, DEVICE_IS_FINISHED | EXCHANGE_COMPLETE);
				//if (!isGood) return false;
				//else return true;
			}
		}
		else {
			if ((localReg.get(regNo) & DATA_MASK) == DEVICE_CAN_WRITE) { // must get captured immediatly, cos gets called immediatly after giving bus away
				logger() << "old v" << L_endl;
				auto timeout = Timer_mS(300);
				do {
					auto regVal = localReg.get(regNo);
					auto handshake = regVal & HANDSHAKE_MASK;
					if (handshake == EXCHANGE_COMPLETE) break;
					if (handshake == DATA_SENT) {
						logger() << L_time << F("wait_DevicesToFinish 0x") << L_hex << getAddress() << " read: 0x" << regVal << L_endl;
						localReg.set(regNo, (regVal & DATA_MASK) | DATA_READ);
					}
				} while (!timeout);
				//auto delayedBy = timeout.timeUsed();
				//logger() << L_time << "WaitedforI2C: " << delayedBy << L_endl;
				if (timeout) {
					logger() << L_time << F("wait_DevicesToFinish 0x") << L_hex << getAddress() << " read: 0x" << localReg.get(regNo) << " Timed-out" << L_endl;
				}
				else {
					auto timeused = timeout.timeUsed();
					logger() << L_time << F("wait_DevicesToFinish OK 0x") << L_hex << getAddress() << " in mS: " << L_dec << timeused << L_endl;
				}
			}
		}
		return (localReg.get(regNo) & DATA_MASK) == DEVICE_IS_FINISHED;
	}

	bool I2C_To_MicroController::receive_handshakeData(volatile uint8_t & data) {
		/*
		do { // Prog tells Remote it can be Master
			Prog.send data + DATA_SENT // when receiver sees DATA_SENT it sets DATA_READ and clears DATA_SENT
			Prog.read data + flags // receiver will not act on new valid-flag if ready-flag is set.
			if (flags == DATA_READ) break;
		} while (!timeout_300mS);
		Prog.send data + EXCHANGE_COMPLETE;
		// Prog assumes Remote is Master
		timeout = 300mS
		*/

		// DEVICE_CAN_WRITE = 0x38, DEVICE_IS_FINISHED = 0x07 /* 00,111,000 : 00,000,111 */
		//, DATA_SENT = 0xC0, DATA_READ = 0x40, EXCHANGE_COMPLETE = 0x80 /* 11,000,000 : 01,000,000 : 10,000,000 */
		//, HANDSHAKE_MASK = 0xC0, DATA_MASK = ~HANDSHAKE_MASK /* 11,000,000 : 00,111,111 */

		if ((data & HANDSHAKE_MASK) != EXCHANGE_COMPLETE) {
			auto timeout = Timer_mS(300);
			do { // prog will keep sending DATA_SENT until it reads DATA_READ, when it will send EXCHANGE_COMPLETE
				auto handshake = data & HANDSHAKE_MASK;
				if (handshake == EXCHANGE_COMPLETE) break;
				if (handshake == DATA_SENT) {
					logger() << L_time << F("receive_data Reg ") << int(_remoteRegOffset) << F(" is: 0x") << L_hex << int(handshake) << L_endl;
					data = (data & DATA_MASK) | DATA_READ;
				}
			} while (!timeout);
			if (timeout) {
				logger() << L_time << F("receive_data 0x") << L_hex << getAddress() << " read: 0x" << data << " Timed-out" << L_endl;
			}
			else {
				auto timeused = timeout.timeUsed();
				logger() << L_time << F("receive_data OK 0x") << L_hex << getAddress() << " in mS: " << L_dec << timeused << L_endl;
				return true;
			}
		}
		return false;
	}
}