#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <MultiCrystal.h>
#include <i2c_Device.h>
#include <I2C_Recover.h>

namespace HardwareInterfaces {

	class RemoteDisplay : public I_I2Cdevice_Recovery {
	public:
		RemoteDisplay(I2C_Recovery::I2C_Recover & recovery, int addr);
		RemoteDisplay(int addr) : RemoteDisplay(*I_I2Cdevice_Recovery::set_recover, addr) {}
		// Virtual Functions
		I2C_Talk_ErrorCodes::error_codes testDevice() override;
		I2C_Talk_ErrorCodes::error_codes initialiseDevice() override;
		MultiCrystal & displ() { return _lcd; }
	private:
		MultiCrystal _lcd;
	};

}