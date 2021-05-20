#pragma once
#include "Keypad.h"
#include <MultiCrystal.h>
#include <Arduino.h>
#include <PinObject.h>

namespace HardwareInterfaces {
	const uint8_t US_REMOTE_MASTER_I2C_ADDR = 0x12;
	const uint8_t DS_REMOTE_MASTER_I2C_ADDR = 0x13;
	const uint8_t FL_REMOTE_MASTER_I2C_ADDR = 0x14;


	class RemoteKeypadMaster : public I_Keypad
	{
	public:
//		MultiCrystal& displ() { return *_lcd; }
		int getKeyCode(int gpio);
		enum KeyNames { UP_PIN = 10, DOWN_PIN, LEFT_PIN, RIGHT_PIN=14, NO_OF_KEYS = 4 }; // leave 13 for LED.
		static Pin_Watch_Debounced KeyPins[NO_OF_KEYS];
	private:
		//MultiCrystal * _lcd;
	};

	extern RemoteKeypadMaster remoteKeypad;  // to be defined by the client (required by interrup handler)

}
