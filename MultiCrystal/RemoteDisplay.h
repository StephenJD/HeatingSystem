#pragma once
#include "Arduino.h"
#include <I2C_Device.h>
#include <MultiCrystal.h>
#include <LCD_Display.h>

namespace I2C_Recovery { class I2C_Recover; }

namespace HardwareInterfaces {

	class RemoteDisplay : public LCD_Display_Buffer<16, 2>, public I_I2Cdevice_Recovery {
	public:
		RemoteDisplay(I2C_Recovery::I2C_Recover & recovery, int addr);
		RemoteDisplay(int addr) : RemoteDisplay(*I_I2Cdevice_Recovery::set_recover, addr) {}
		// Virtual Functions
		I2C_Talk_ErrorCodes::Error_codes testDevice() override;
		I2C_Talk_ErrorCodes::Error_codes initialiseDevice() override;
		void sendToDisplay() override;

		MultiCrystal & displ() { return _lcd; }
	private:
		MultiCrystal _lcd;
	};

}