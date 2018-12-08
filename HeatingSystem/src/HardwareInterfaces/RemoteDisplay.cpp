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
		uint8_t dataBuffa[2];
		uint16_t & thisKey = reinterpret_cast<uint16_t &>(dataBuffa[0]);
		thisKey = 0x5C36;
		uint8_t hasFailed = _i2C->write(_address, 0x04, 2, dataBuffa); // Write to GPINTEN
		thisKey = 0;
		hasFailed = _i2C->read(_address, 0x04, 2, dataBuffa); // Check GPINTEN is correct
		if (!hasFailed) hasFailed = (thisKey == 0x5C36 ? I2C_Helper::_OK : I2C_Helper::_I2C_ReadDataWrong);
		return hasFailed;
	}

	uint8_t RemoteDisplay::initialiseDevice() {
		return _lcd.begin(16, 2);	
	}

}