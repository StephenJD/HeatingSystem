#include <I2C_Recover.h>
#include<I2C_Talk.h>

unsigned long I2C_Recover::getFailedTime(int16_t devAddr) const { return i2C().getFailedTime(devAddr); }

void I2C_Recover::setFailedTime(int16_t devAddr) const { i2C().setFailedTime(devAddr); }

TwoWire & I2C_Recover::wirePort() const { return i2C().wire_port; }

void I2C_Recover::wireBegin() const { i2C().wireBegin(); }
