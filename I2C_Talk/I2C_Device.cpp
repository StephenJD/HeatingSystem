#include <I2C_Device.h>
#include <Logging.h>

using namespace I2C_Talk_ErrorCodes;

I2C_Talk * I_I2Cdevice::_i2C = 0;
I2C_Recover * I2Cdevice::_recovery = 0;

void I_I2Cdevice::setAddress(uint8_t addr) {
	_address = addr; 
	//if (_i2C == 0)  logger().log_notime("\n\n*** I2Cdevice not initialized! *** at addr ", addr);
}

uint8_t I2Cdevice::read(uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) {
	TryAgain tryAgain(recovery(), *this);
	auto status = getStatus();
	if (status == _OK) {
		do {
			status = i2c_Talk().read(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (tryAgain(status));
	}
	return status;
} // Return errCode. dataBuffer may not be written to if read fails.

uint8_t I2Cdevice::write(uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) {
	TryAgain tryAgain(recovery(), *this);
	auto status = getStatus();
	if (status == _OK) {
		do {
			status = i2c_Talk().write(getAddress(), registerAddress, numberBytes, dataBuffer);
		} while (tryAgain(status));
	}
	return status;
}  // Return errCode.
