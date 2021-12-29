#pragma once
#include <Keypad.h>
//#include <MultiCrystal.h>
#include <Arduino.h>
#include <PinObject.h>

namespace HardwareInterfaces {

	class RemoteKeypadMaster : public I_Keypad {
	public:
		RemoteKeypadMaster(int re_read_interval_mS = 50, int wakeTime_S = 30) : I_Keypad{ re_read_interval_mS, wakeTime_S } {}
		KeyOperation getKeyCode() override;
		enum { UP_PIN = A0, DOWN_PIN, RIGHT_PIN, LEFT_PIN, NO_OF_KEYS = 4 };
		static Pin_Watch KeyPins[NO_OF_KEYS];
	private:
	};

	extern RemoteKeypadMaster remoteKeypad;  // to be defined by the client (required by interrup handler)
}
