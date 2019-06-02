#include <I2C_Recover.h>
#include<I2C_Talk.h>
#include <I2C_Device.h>

unsigned long I2C_Recover::getFailedTime() const { return i2C().getFailedTime(_deviceAddr); }

void I2C_Recover::setFailedTime() const { i2C().setFailedTime(_deviceAddr); }

TwoWire & I2C_Recover::wirePort() const { return i2C().wire_port; }

void I2C_Recover::wireBegin() const { i2C().wireBegin(); }

uint8_t I2C_Recover::testDevice(I_I2Cdevice * deviceFailTest) {
	if (deviceFailTest == 0) {
		//Serial.println("testDevice - none set, using notExists");
		return i2C().notExists(addr());
	}
	else {
		return deviceFailTest->testDevice();
	}
}