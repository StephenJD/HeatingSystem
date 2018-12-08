#include "MixValveController.h"

namespace HardwareInterfaces {

	MixValveController::MixValveController(int addr) : I_I2Cdevice(addr) {
		Serial.println("MixValveController Constructed");
	}


	uint8_t MixValveController::testDevice(I2C_Helper & i2c, int addr) {
		unsigned long waitTime = 3000UL + *_timeOfReset_mS;
		//Serial.println("MixValveController.testDevice");
		uint8_t hasFailed;
		do {
			uint8_t dataBuffa[1] = { 1 };
			hasFailed = _i2C->write(_address, 7, 55); // write request temp
			hasFailed |= _i2C->read(_address, 7, 1, dataBuffa); // read request temp
			if (!hasFailed) hasFailed = (dataBuffa[0] == 55 ? I2C_Helper::_OK : I2C_Helper::_I2C_ReadDataWrong);
		} while (hasFailed && millis() < waitTime);
		return hasFailed;
	}
}