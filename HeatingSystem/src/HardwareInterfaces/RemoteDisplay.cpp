#include "RemoteDisplay.h"
#include "MultiCrystal.h"
#include <Logging.h>

using namespace I2C_Talk_ErrorCodes;

namespace HardwareInterfaces {
	using namespace I2C_Recovery;

	RemoteDisplay::RemoteDisplay(I2C_Recover & recovery, int addr)
		: I_I2Cdevice_Recovery(recovery, addr),
		_lcd(7, 6, 5,
		4, 3, 2, 1,
		0, 4, 3, 2,
		this, addr,
		0xFF, 0x1F) {
		logger() << "RemoteDisplay : 0x" << L_hex << I_I2Cdevice::getAddress() << " Constructed" << L_endl;
	}
	
	error_codes RemoteDisplay::testDevice() {
		//Serial.println("RemoteDisplay.testDevice");
		uint8_t dataBuffa[2] = {0x5C, 0x36};
		return write_verify(0x04, 2, dataBuffa); // Write to GPINTEN
	}

	error_codes RemoteDisplay::initialiseDevice() {
		auto rem_error = _lcd.begin(16, 2) == 0 ? _OK : _unknownError;
		if (rem_error != _OK) {
			logger() << " Remote.begin() for display [0x" << L_hex << getAddress() << "] failed with:" << I2C_Talk::getStatusMsg(rem_error) << L_endl;
		}
		else displ().print("Start ");
		return rem_error;
	}

}