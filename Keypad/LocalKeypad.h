#pragma once
#include "Keypad.h"
#include <PinObject.h>
#include <Arduino.h>

namespace HardwareInterfaces {
	// Local Keypad notifies via int 5, 
	// read via analogue 0,

	/// <summary>
	/// Requires begin() in setup() to attach interrupt
	/// </summary>
	class LocalKeypad : public I_Keypad {
	public:
		LocalKeypad(int interrupt_Pin, int keyRead_Pin, int keyHighRef_Pin, Pin_Wag indicator_LED);
		void begin();
		static Pin_Wag& indicatorLED() { return reinterpret_cast<Pin_Wag&>(_indicatorLED); }
		static uint8_t interruptPin;
		KeyOperation getKeyCode() override;
	private:
		// members need to be static to allow attachment to interrupt handler
		static constexpr uint16_t adc_LocalKey_val[] = { 874,798,687,612,551,501,440 };
		//static Pin_Wag _indicatorLED; // static UDT's don't get constructed!
		static uint8_t _indicatorLED;
		static uint8_t _keyReadPin;
		static uint8_t _keyHighRefPin;
	};

	// Functions supporting local interrupt
	extern LocalKeypad* localKeypad;  // to be defined by the client (required by interrup handler)
	void localKeyboardInterrupt(); // must be a static or global
}