#pragma once
#include "I_Keypad.h"
#include <MultiCrystal.h>
#include <Arduino.h>

namespace HardwareInterfaces {

	class RemoteKeypad : public I_Keypad
	{
	public:
		RemoteKeypad(MultiCrystal & lcd) : _lcd(&lcd) {}
		int getKey() override;
		int readKey() override;
	private:
		int getKeyCode(int gpio);
		MultiCrystal * _lcd;
	};

}
