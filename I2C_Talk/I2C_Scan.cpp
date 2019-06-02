#include "I2C_Scan.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <Logging.h>

void I2C_Scan::reset(){
	foundDeviceAddr = -1;
	totalDevicesFound = 0;
    error = I2C_Talk::_OK;
}

bool I2C_Scan::nextDevice() { // returns false when no more found
	if (!_i2C->_canWrite) return false;
	_inScanOrSpeedTest = true;
	_i2C->useAutoSpeed(false);
	if (foundDeviceAddr == -1) {
		totalDevicesFound = 0;
		foundDeviceAddr = 0;
	}
	while(++foundDeviceAddr >= 0) {
		_i2C->recovery().callTime_OutFn();
		auto startFreq = _i2C->getThisI2CFrequency(foundDeviceAddr);
		if (startFreq == 0) startFreq = 100000;
		startFreq = _i2C->setI2Cfreq_retainAutoSpeed(startFreq);
		//Serial.print(foundDeviceAddr, HEX); Serial.print(" at freq: "); Serial.println(startFreq, DEC);

		error = _i2C->recovery().findAworkingSpeed();
		if (error == I2C_Talk::_OK) {
		  ++totalDevicesFound;
		  break;
		} else if (error == I2C_Talk::_NACK_during_complete) break; // unknown bus error
	}
	if (error == I2C_Talk::_NACK_during_address_send) error = I2C_Talk::_OK; // don't report expected errors whilst searching for a device
	_i2C->useAutoSpeed(true);
	_inScanOrSpeedTest = false;
	return (foundDeviceAddr >= 0);
}