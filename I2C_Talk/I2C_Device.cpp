#include <I2C_Device.h>
#include <I2C_Recover.h>
#include <I2C_Talk.h>
#include <Logging.h>

using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;

auto I_I2Cdevice::read_verify_2bytes(int registerAddress, uint8_t(& dataBuffer)[2], int requiredConsecutiveReads, int canTryAgain, uint16_t dataMask)->Error_codes {
	Error_codes errorCode;
	int16_t newRead = 0;
	int16_t prevRead = 0;
	auto needAnotherGoodReading = requiredConsecutiveReads;
	do {
		errorCode = I_I2Cdevice::read(registerAddress, 2, dataBuffer);
		newRead = (dataBuffer[0] << 8) + dataBuffer[1];
		newRead &= dataMask;
		if (!errorCode) {
			if (newRead == prevRead) --needAnotherGoodReading;
			else {
				needAnotherGoodReading = requiredConsecutiveReads;
				//logger() << "read_verify_2bytes 0x" << L_hex << newRead << L_endl;
				prevRead = newRead;
			}
		} 
		//else logger() << "read_verify_2bytes" << I2C_Talk::getStatusMsg(errorCode) << L_endl;
		--canTryAgain;
	} while (canTryAgain >= needAnotherGoodReading && needAnotherGoodReading > 1);
	if (!errorCode) errorCode = needAnotherGoodReading > 1 ? _I2C_ReadDataWrong : _OK;
	return errorCode;
}

auto I_I2Cdevice::read_verify_1byte(int registerAddress, uint8_t & dataBuffer, int requiredConsecutiveReads, int canTryAgain, uint8_t dataMask)->Error_codes {
	Error_codes errorCode;
	int8_t newRead = 0;
	int8_t prevRead = 0;
	auto needAnotherGoodReading = requiredConsecutiveReads;
	do {
		errorCode = I_I2Cdevice::read(registerAddress, 1, &dataBuffer);
		newRead = dataBuffer;
		newRead &= dataMask;
		if (!errorCode) {
			if (newRead == prevRead) --needAnotherGoodReading;
			else {
				needAnotherGoodReading = requiredConsecutiveReads;
				//logger() << "read_verify_1byte 0x" << L_hex << newRead << L_endl;
				prevRead = newRead;
			}
		} 
		//else logger() << "read_verify_1byte" << I2C_Talk::getStatusMsg(errorCode) << L_endl;
		--canTryAgain;
	} while (canTryAgain >= needAnotherGoodReading && canTryAgain && needAnotherGoodReading > 1);
	if (!errorCode) errorCode = needAnotherGoodReading > 1 ? _I2C_ReadDataWrong : _OK;
	return errorCode;
}

I2C_Recovery::I2C_Recover * I_I2Cdevice_Recovery::set_recover;

I2C_Talk & I_I2Cdevice_Recovery::i2C() { return recovery().i2C(); }

Error_codes I_I2Cdevice_Recovery::getStatus() {
	if (!isEnabled()) return _disabledDevice;
	else {
		i2C().setI2CFrequency(runSpeed());
		return i2C().status(getAddress());
	}
}

int32_t I_I2Cdevice_Recovery::set_runSpeed(int32_t i2cFreq) { return _i2c_speed = recovery().i2C().setI2CFrequency(i2cFreq); }

Error_codes I_I2Cdevice_Recovery::read(int registerAddress, int numberBytes, uint8_t *dataBuffer) { // dataBuffer may not be written to if read fails.
	auto status = recovery().newReadWrite(*this);
	//logger() << F(" I_I2Cdevice_Recovery::read newReadWrite:") << I2C_Talk::getStatusMsg(status) << L_endl;
	if (status == _OK) {
		auto tries = 15;
		do {
			status = i2C().read(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status) && --tries);
	}
	return status;
} 

Error_codes I_I2Cdevice_Recovery::write(int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	auto status = recovery().newReadWrite(*this);
	if (status == _OK) {
		auto tries = 15;
		do {
			status = i2C().write(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status) && --tries);
	}
	return status;
}

Error_codes I_I2Cdevice_Recovery::write_verify(int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	//logger() << F("I_I2Cdevice_Recovery::write_verify Try newReadWrite") << L_endl;
	auto status = recovery().newReadWrite(*this);
	if (status == _OK) {
		auto tries = 15;
		do {
			status = i2C().write_verify(getAddress(), registerAddress, numberBytes, dataBuffer);
			//logger() << F("I_I2Cdevice_Recovery::write_verify done") << I2C_Talk::getStatusMsg(status) << L_endl;
		} while (recovery().tryReadWriteAgain(status) && --tries);
	}
	return status;
}

