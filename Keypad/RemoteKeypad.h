#pragma once
#include "Keypad.h"
#include <MultiCrystal.h>
#include <Arduino.h>

namespace HardwareInterfaces {

	class RemoteKeypad : public I_Keypad
	{
	public:
		RemoteKeypad(MultiCrystal & lcd) : _lcd(&lcd) {}
		int getKey() override;
		int readKey() override;
		bool wakeDisplay(bool) override { return true; }
//		MultiCrystal& displ() { return *_lcd; }
	private:
		int getKeyCode(int gpio);
		MultiCrystal * _lcd;
	};

}
