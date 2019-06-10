#include "I2C_Scan.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <Logging.h>

using namespace I2C_Talk_ErrorCodes;

void I2C_Scan::scanFromZero(){
	foundDeviceAddr = 0; // address 0 is a call-all, not a specific device
	totalDevicesFound = 0;
    error = _OK;
}

int8_t I2C_Scan::nextDevice() { // returns address of next device found or zero
	if (!_i2C->_canWrite) return 0;
	inScan(true);
	if (foundDeviceAddr <= 0) scanFromZero();
	while((error = _i2C->addressOutOfRange(++foundDeviceAddr)) == _OK) {
		//Serial.print(foundDeviceAddr, HEX); Serial.print(" at freq: "); Serial.println(startFreq, DEC);
		auto device = I2Cdevice(recovery(), foundDeviceAddr);
		recovery().registerDevice(device);
		error = recovery().findAworkingSpeed();
		if (error == _OK) {
		  ++totalDevicesFound;
		  break;
		} else if (error == _NACK_during_complete) break; // unknown bus error
	}
	if (error == _NACK_during_address_send) error = _OK; // don't report expected errors whilst searching for a device
	else if (error == _I2C_AddressOutOfRange) {
		foundDeviceAddr = 0;
		error = _OK;
	}
	inScan(false);
	return foundDeviceAddr;
}