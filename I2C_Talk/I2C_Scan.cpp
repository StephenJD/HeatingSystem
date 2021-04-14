#include "I2C_Scan.h"
#include <I2C_Recover.h>
//#include <Logging.h>
#include <..\Logging\Logging.h>

using namespace I2C_Talk_ErrorCodes;
using namespace arduino_logger;

I2C_Recovery::I2C_Recover I_I2C_Scan::nullRecovery;

I_I2C_Scan::I_I2C_Scan(I2C_Talk & i2c, I2C_Recovery::I2C_Recover & recovery) 
	:_device(recovery)
{
	recovery.set_I2C_Talk(i2c);
}

void I_I2C_Scan::scanFromZero(){
	foundDeviceAddr = 0; // address 0 is a call-all, not a specific device
	totalDevicesFound = 0;
    _error = _OK;
}

int8_t I_I2C_Scan::nextDevice() { // returns address of next device found or zero
	if (!i2C().isMaster()) return 0;
	inScan(true);
	if (foundDeviceAddr <= 0) scanFromZero();
	while((_error = i2C().addressOutOfRange(++foundDeviceAddr)) == _OK) {
		//Serial.print(foundDeviceAddr, HEX); Serial.print(" at freq: "); Serial.println(i2C().getI2CFrequency(), DEC); Serial.flush();
		device().setAddress(foundDeviceAddr);
		//Serial.println(" recovery().registerDevice..."); Serial.flush();
		recovery().registerDevice(device());
		//Serial.println(" recovery().findAworkingSpeed..."); Serial.flush();
		_error = recovery().findAworkingSpeed();
		if (error() == _OK) {
		  ++totalDevicesFound;
		  break;
		} else if (error() == _NACK_during_complete) break; // unknown bus error
	}
	if (error() == _NACK_during_address_send) _error = _OK; // don't report expected errors whilst searching for a device
	else if (error() == _I2C_AddressOutOfRange) {
		foundDeviceAddr = 0;
		_error = _OK;
	}
	inScan(false);
	return foundDeviceAddr;
}