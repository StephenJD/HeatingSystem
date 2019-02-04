#include "MixValveController.h"

namespace HardwareInterfaces {

	MixValveController::MixValveController(int addr) : I_I2Cdevice(addr) {
		Serial.println("MixValveController Constructed");
	}


	uint8_t MixValveController::testDevice(I2C_Helper & i2c, int addr) {
		unsigned long waitTime = 3000UL + *_timeOfReset_mS;
		//Serial.println("MixValveController.testDevice");
		uint8_t hasFailed;
		uint8_t dataBuffa[1] = { 55 };
		do hasFailed = _i2C->write_verify(_address, 7, 1, dataBuffa); // write request temp
		while (hasFailed && millis() < waitTime);
		return hasFailed;
	}
}