#include "RemoteDisplay.h"
#include <Logging.h>

using namespace I2C_Talk_ErrorCodes;

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
		//logger() << F("RemoteDisplay : 0x") << L_hex << I_I2Cdevice::getAddress() << F(" Constructed") << L_endl;
	}
	
	Error_codes RemoteDisplay::testDevice() { // non-recovery test
		uint8_t dataBuffa[2] = {0x5C, 0x36};
		return I_I2Cdevice::write_verify(0x04, 2, dataBuffa); // Write to GPINTEN
	}

	Error_codes RemoteDisplay::initialiseDevice() {
		if (runSpeed() > 100000) set_runSpeed(100000);
		auto rem_error = _lcd.begin(16, 2);
		if (rem_error != _OK) {
			logger() << F(" Remote.begin() failed for display 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(rem_error) << " at progress " << _lcd.getError() << L_endl;
		}
		return Error_codes(rem_error);
	}

	void RemoteDisplay::sendToDisplay() {
		//logger() << L_time << F("Write Remote Display 0x") << L_hex << getAddress() << L_endl;
		auto rem_error = displ().checkI2C_Failed();
		if (rem_error) {
			logger() << L_time << F("Restarting Remote Display 0x") << L_hex << getAddress() << I2C_Talk::getStatusMsg(rem_error) << L_endl;
			initialiseDevice();
		}
		displ().clear();
		displ().print(buff());
		//logger() << L_time << F("Write Remote Done 0x") << L_hex << getAddress() << L_endl;
	}

}