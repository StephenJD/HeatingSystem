#include "RemoteDisplay.h"
#include "MultiCrystal.h"
#include <Logging.h>

namespace HardwareInterfaces {

	RemoteDisplay::RemoteDisplay(I2C_Talk & i2C, int addr)
		: I_I2Cdevice(i2C, addr),
		_lcd(7, 6, 5,
		4, 3, 2, 1,
		0, 4, 3, 2,
		&i2c_Talk(), I_I2Cdevice::getAddress(),
		0xFF, 0x1F) {
		Serial.print("RemoteDisplay : "); Serial.print(I_I2Cdevice::getAddress(),HEX); Serial.println(" Constructed");
	}
	
	uint8_t RemoteDisplay::testDevice() {
		//Serial.println("RemoteDisplay.testDevice");
		uint8_t dataBuffa[2] = {0x5C, 0x36};
		return write_verify(0x04, 2, dataBuffa); // Write to GPINTEN
	}

	uint8_t RemoteDisplay::initialiseDevice() {
		uint8_t rem_error = _lcd.begin(16, 2);
		if (rem_error) {
			logger().log(" Remote.begin() for display [", getAddress(), "] failed with:", rem_error);
		}
		else displ().print("Start");
		return rem_error;
	}

}