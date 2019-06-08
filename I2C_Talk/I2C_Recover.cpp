#include <I2C_Recover.h>
#include<I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

using namespace I2C_Talk_ErrorCodes;

TwoWire & I2C_Recover::wirePort() const { return i2C()._wire_port; }

void I2C_Recover::wireBegin() const { i2C().wireBegin(); }

uint8_t I2C_Recover::testDevice(int , int ) {
	return device().testDevice();
}