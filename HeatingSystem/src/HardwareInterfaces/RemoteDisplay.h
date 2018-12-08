#pragma once
#include "Arduino.h"
#include <I2C_Helper.h>
#include "MultiCrystal.h"

namespace HardwareInterfaces {

	class RemoteDisplay : public I2C_Helper::I_I2Cdevice {
	public:
		RemoteDisplay(I2C_Helper & i2c, int addr);
		// Virtual Functions
		uint8_t testDevice(I2C_Helper & i2c, int addr) override;
		uint8_t initialiseDevice() override;
	private:
		MultiCrystal _lcd;
	};

}