#include "RemoteDisplay.h"
#include <Logging.h>

using namespace I2C_Talk_ErrorCodes;
using namespace arduino_logger;

namespace HardwareInterfaces {
	using namespace I2C_Recovery;

	RemoteDisplay::RemoteDisplay(I2C_Recover & recovery, int addr)
		: I_I2Cdevice_Recovery(recovery, addr)
		,_lcd(7, 6, 5,
		4, 3, 2, 1,
		0, 4, 3, 2,
		this, addr,
		0xFF, 0x1F) 
	{ // Constructor logging causes freeze!
		//logger() << F("RemoteDisplay device 0x") << L_hex << I_I2Cdevice::getAddress() << F(" Constructed") << L_endl;
	}
	
	Error_codes RemoteDisplay::testDevice() { // non-recovery test
		uint8_t dataBuffa[2] = {0xE1, 0x00};
		return I_I2Cdevice::write_verify(0x04, 2, dataBuffa); // Write to GPINTEN
	}

	Error_codes RemoteDisplay::initialiseDevice() {
		auto rem_error = _lcd.begin(16, 2);
		LCD_Display::reset();
		displ().clear();
		if (rem_error != _OK) {
			logger() << F(" RemoteDisplay.begin() device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(rem_error) << " at progress " << _lcd.getError() << L_endl;
		}
		return Error_codes(rem_error);
	}

	void RemoteDisplay::sendToDisplay() {
		//logger() << L_time << F("Write Remote Display device 0x") << L_hex << getAddress() << L_endl;
		auto rem_error = displ().checkI2C_Failed();
		//auto rem_error = _OK;
		if (rem_error != _disabledDevice) {
			if (rem_error) {
				rem_error = initialiseDevice();
				logger() << L_time << F("Restarted RemoteDisplay device 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(rem_error) << L_endl;
			}
			clearFromEnd();
			displ().setCursor(0, 0);
			//displ().clear();
			displ().print(buff());
			//logger() << L_time << F("Write Remote Done device 0x") << L_hex << getAddress() << L_endl;
		}
	}

}