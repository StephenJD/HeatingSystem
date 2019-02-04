#include "RemoteDisplay.h"
#include "MultiCrystal.h"

namespace HardwareInterfaces {

	RemoteDisplay::RemoteDisplay(I2C_Helper & i2c, int addr) 
		: I_I2Cdevice(addr),
		_lcd(7, 6, 5,
		4, 3, 2, 1,
		0, 4, 3, 2,
		&i2c, addr,
		0xFF, 0x1F) {
		Serial.print("RemoteDisplay : "); Serial.print(addr,HEX); Serial.println(" Constructed");
	}
	
	uint8_t RemoteDisplay::testDevice(I2C_Helper & i2c, int addr) {
		//Serial.println("RemoteDisplay.testDevice");
		uint8_t dataBuffa[2] = {0x5C, 0x36};
		return _i2C->write_verify(_address, 0x04, 2, dataBuffa); // Write to GPINTEN
	}

	uint8_t RemoteDisplay::initialiseDevice() {
		return _lcd.begin(16, 2);	
	}

}