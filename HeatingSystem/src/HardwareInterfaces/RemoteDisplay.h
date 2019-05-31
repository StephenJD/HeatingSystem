#pragma once
#include "Arduino.h"
#include <I2C_Talk\I2C_Talk.h>
#include <I2C_Talk/I2C_Device.h>
#include "MultiCrystal.h"

namespace HardwareInterfaces {

	class RemoteDisplay : public I_I2Cdevice {
	public:
		RemoteDisplay(I2C_Talk & i2C, int addr);
		// Virtual Functions
		uint8_t testDevice() override;
		uint8_t initialiseDevice() override;
		MultiCrystal & displ() { return _lcd; }
	private:
		MultiCrystal _lcd;
	};

}