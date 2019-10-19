#include <I2C_Recover.h>
#include<I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

namespace I2C_Recovery {

	using namespace I2C_Talk_ErrorCodes;

	TwoWire & I2C_Recover::wirePort() const { return i2C()._wire_port; }

	void I2C_Recover::wireBegin() { 
		//Serial.print("I2C_Recover::wire: "); Serial.println(long(&i2C())); Serial.flush();
		i2C().wireBegin(); 
	}

	error_codes I2C_Recover::testDevice(int, int) {
		return device().testDevice();
	}
}