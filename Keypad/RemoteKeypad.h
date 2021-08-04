#pragma once
#include "Keypad.h"
#include <MultiCrystal.h>
#include <Arduino.h>

namespace HardwareInterfaces {

	class RemoteKeypad : public I_Keypad {
	public:
		RemoteKeypad(MultiCrystal& lcd) : I_Keypad{ 50 }, _lcd(&lcd) {}
		void startRead() override;
//#if defined (ZPSIM)
//		void readKey();
//#endif
	private:
		KeyOperation getKeyCode() override;
		MultiCrystal* _lcd;
	};

}