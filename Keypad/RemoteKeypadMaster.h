#pragma once
#include "Keypad.h"
//#include <MultiCrystal.h>
#include <Arduino.h>
#include <PinObject.h>

namespace HardwareInterfaces {
	const uint8_t US_REMOTE_MASTER_I2C_ADDR = 0x12;
	const uint8_t DS_REMOTE_MASTER_I2C_ADDR = 0x13;
	const uint8_t FL_REMOTE_MASTER_I2C_ADDR = 0x14;


	class RemoteKeypadMaster : public I_Keypad {
	public:
		RemoteKeypadMaster(int re_read_interval = 50) : I_Keypad{ re_read_interval } {}
		//		MultiCrystal& displ() { return *_lcd; }
		KeyOperation getKeyCode() override;
		enum { UP_PIN = A0, DOWN_PIN, RIGHT_PIN, LEFT_PIN, NO_OF_KEYS = 4 };
		static Pin_Watch KeyPins[NO_OF_KEYS];
	private:
	};

	extern RemoteKeypadMaster remoteKeypad;  // to be defined by the client (required by interrup handler)

}
