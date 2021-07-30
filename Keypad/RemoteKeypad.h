#pragma once
#include "Keypad.h"
#include <MultiCrystal.h>
#include <Arduino.h>

namespace HardwareInterfaces {

	class RemoteKeypad : public I_Keypad {
	public:
		RemoteKeypad(MultiCrystal& lcd) : _lcd(&lcd) {}
		int getKey() override;
		//		MultiCrystal& displ() { return *_lcd; }
	private:
		int getKeyCode(int port) override;
		MultiCrystal* _lcd;
	};

}
