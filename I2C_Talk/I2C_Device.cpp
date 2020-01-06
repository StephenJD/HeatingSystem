#include <I2C_Device.h>
#include <I2C_Recover.h>

using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;

I2C_Recovery::I2C_Recover * I_I2Cdevice_Recovery::set_recover;

I2C_Talk & I_I2Cdevice_Recovery::i2C() { return recovery().i2C(); }

error_codes I_I2Cdevice_Recovery::getStatus() {
	if (!isEnabled()) return _disabledDevice;
	else {
		i2C().setI2CFrequency(runSpeed());
		return i2C().status(getAddress());
	}
}

error_codes I_I2Cdevice_Recovery::read(int registerAddress, int numberBytes, uint8_t *dataBuffer) { // dataBuffer may not be written to if read fails.
	auto status = recovery().newReadWrite(*this);
	//logger() << " I2Cdevice::read status:", status);
	if (status == _OK) {
		do {
			status = i2C().read(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status));
	}
	return status;
} 

error_codes I_I2Cdevice_Recovery::write(int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	auto status = recovery().newReadWrite(*this);
	if (status == _OK) {
		do {
			status = i2C().write(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status));
	}
	return status;
}

error_codes I_I2Cdevice_Recovery::write_verify(int registerAddress, int numberBytes, const uint8_t *dataBuffer) {
	auto status = recovery().newReadWrite(*this);
	if (status == _OK) {
		do {
			status = i2C().write_verify(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (recovery().tryReadWriteAgain(status));
	}
	return status;
}