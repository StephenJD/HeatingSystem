#pragma once
#include "I_Keypad.h"
#include <MultiCrystal.h>
#include <Arduino.h>

namespace HardwareInterfaces {
	// Local Keypad notifies via int 5, 
	// read via analogue 0,

	class RemoteKeypad : public I_Keypad
	{
	public:
		RemoteKeypad(MultiCrystal & lcd) : _lcd(&lcd) {}
		int getKey() override;
		int readKey() override;
	private:
		enum { NUM_LOCAL_KEYS = 7, LOCAL_INT_PIN = 18, KEY_ANALOGUE = 1 };
		// members need to be static to allow attachment to interrupt handler
		static int16_t adc_LocalKey_val[NUM_LOCAL_KEYS];
		int getKeyCode(int gpio);
		MultiCrystal * _lcd;
	};

}
