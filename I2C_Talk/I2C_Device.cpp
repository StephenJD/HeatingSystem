

#include <I2C_Device.h>
#include <I2C_Recover.h>
#include <I2C_Talk.h>
#include <Logging.h>

using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;
using namespace arduino_logger;

auto I_I2Cdevice::read_verify_2bytes(int registerAddress, int16_t & data, int requiredConsecutiveReads, int canTryAgain, uint16_t dataMask)->Error_codes {
	// I2C devices use big-endianness: MSB at the smallest address: So a two-byte read is [MSB, LSB].
	// But Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
	// A two-byte read must reverse the byte-order.
	i2C().setI2CFrequency(runSpeed());
	uint8_t dataBuffer[2];
	Error_codes errorCode; // Non-Recovery
	int16_t newRead;
	data = int16_t(0xAAAA); // 1010 1010 1010 1010
	auto needAnotherGoodReading = requiredConsecutiveReads;
	do {
		errorCode = I_I2Cdevice::read(registerAddress, 2, dataBuffer);
		newRead = i2C().fromBigEndian(dataBuffer); // convert device to native endianness
		newRead &= dataMask;
		if (!errorCode) {
			if (newRead == data) --needAnotherGoodReading;
			else {
				needAnotherGoodReading = requiredConsecutiveReads - 1;
				//logger() << "read_verify_2bytes 0x" << L_hex << newRead << L_endl;
				data = newRead;
			}
		} 
		else { // abort on first error
			logger() << L_time << "read_verify_2bytes device 0x" << L_hex << getAddress() << " Reg 0x" << registerAddress << I2C_Talk::getStatusMsg(errorCode) << L_endl;
			break;
		}
		--canTryAgain;
	} while (canTryAgain >= needAnotherGoodReading && needAnotherGoodReading > 1);
	if (!errorCode) errorCode = needAnotherGoodReading > 1 ? _I2C_ReadDataWrong : _OK;
	return errorCode;
}

auto I_I2Cdevice::read_verify_1byte(int registerAddress, uint8_t & dataBuffer, int requiredConsecutiveReads, int canTryAgain, uint8_t dataMask)->Error_codes {
	i2C().setI2CFrequency(runSpeed());
	Error_codes errorCode; // Non-Recovery
	uint8_t newRead;
	dataBuffer = 0xAA; // 1010 1010
	auto needAnotherGoodReading = requiredConsecutiveReads;
	do {
		errorCode = I_I2Cdevice::read(registerAddress, 1, &newRead);
		newRead &= dataMask;
		if (!errorCode) {
			if (newRead == dataBuffer) --needAnotherGoodReading;
			else {
				needAnotherGoodReading = requiredConsecutiveReads;
				//logger() << "read_verify_1byte 0x" << L_hex << newRead << L_endl;
				dataBuffer = newRead;
			}
		} 
		//else logger() << "read_verify_1byte" << I2C_Talk::getStatusMsg(errorCode) << L_endl;
		--canTryAgain;
	} while (canTryAgain >= needAnotherGoodReading && canTryAgain && needAnotherGoodReading > 1);
	if (!errorCode) errorCode = needAnotherGoodReading > 1 ? _I2C_ReadDataWrong : _OK;
	return errorCode;
}

// ***********************************
//		I_I2Cdevice_Recovery
// ***********************************

I2C_Recovery::I2C_Recover * I_I2Cdevice_Recovery::set_recover;

I2C_Talk & I_I2Cdevice_Recovery::i2C() { return recovery().i2C(); }

Error_codes I_I2Cdevice_Recovery::getStatus() const {
	if (!isEnabled()) return _disabledDevice;
	else {
		//i2C().setI2CFrequency(runSpeed());
		return i2C().status(getAddress());
	}
}

bool I_I2Cdevice_Recovery::isUnrecoverable() const {
	return !isEnabled() && recovery().isUnrecoverable();
}

int32_t I_I2Cdevice_Recovery::set_runSpeed(int32_t i2cFreq) { return _i2c_speed = recovery().i2C().setI2CFrequency(i2cFreq); }

Error_codes I_I2Cdevice_Recovery::read(int registerAddress, int numberBytes, uint8_t *dataBuffer) { // dataBuffer may not be written to if read fails.
	auto status = recovery().newReadWrite(*this, I2C_RETRIES);
	if (status == _OK) {
		do {
			status = i2C().read(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status));
	}
	return status;
} 

Error_codes I_I2Cdevice_Recovery::write(int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	auto status = recovery().newReadWrite(*this, I2C_RETRIES);
	if (status == _OK) {
		do {
			status = i2C().write(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status));
	}
	return status;
}

Error_codes I_I2Cdevice_Recovery::write_verify(int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	auto status = recovery().newReadWrite(*this, I2C_RETRIES);
	if (status == _OK) {
		do {
			status = i2C().write_verify(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status));
	}
	return status;
}

Error_codes I_I2Cdevice_Recovery::reEnable() {
	if (isEnabled()) return _OK;
	else if (millis() - getFailedTime() > DISABLE_PERIOD_ON_FAILURE) {
		reset();
		logger() << L_time << F("Re-enabling disabled device 0x") << L_hex << getAddress() << L_endl;
		return _OK;
	} else 
	return _disabledDevice;
}

