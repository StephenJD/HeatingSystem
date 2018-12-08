#pragma once
#include "I_Keypad.h"
#include <Arduino.h>

namespace HardwareInterfaces {
	// Local Keypad notifies via int 5, 
	// read via analogue 0,

	class LocalKeypad : public I_Keypad
	{
	public:
		LocalKeypad();
		int getKey() override;
		int readKey() override;
		bool isTimeToRefresh() override;
	private:
		enum {NUM_LOCAL_KEYS = 7, LOCAL_INT_PIN = 18, KEY_ANALOGUE = 1};
		// members need to be static to allow attachment to interrupt handler
		static int16_t adc_LocalKey_val[NUM_LOCAL_KEYS];
		static int analogReadDelay(int an_pin);
	};

	// Functions supporting local interrupt
	void localKeyboardInterrupt(); // must be a static or global
	extern LocalKeypad * localKeypad;  // to be defined by the client (required by interrup handler)
}
