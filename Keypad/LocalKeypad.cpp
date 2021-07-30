#include "LocalKeypad.h"
#include <Logging.h>
#include <MsTimer2.h>

//extern unsigned long processStart_mS;

namespace HardwareInterfaces {
	LocalKeypad* localKeypad;
	constexpr uint16_t LocalKeypad::adc_LocalKey_val[];
	uint8_t LocalKeypad::interruptPin;
	//Pin_Wag LocalKeypad::_indicatorLED;
	uint8_t LocalKeypad::_indicatorLED;
	uint8_t LocalKeypad::_keyReadPin;
	uint8_t LocalKeypad::_keyHighRefPin;

	LocalKeypad::LocalKeypad(int interrupt_Pin, int keyRead_Pin, int keyHighRef_Pin, Pin_Wag indicator_LED)
		: I_Keypad{ 10 } { //Display_Stream & displ) : Keypad(displ) 
		localKeypad = this;
		interruptPin = interrupt_Pin;
		_keyReadPin = keyRead_Pin;
		_keyHighRefPin = keyHighRef_Pin;
		_indicatorLED = uint8_t(indicator_LED);
		//begin(); // causes lock-up!
	}

	void LocalKeypad::begin() {
		indicatorLED().begin();
		pinMode(interruptPin, INPUT_PULLUP); // turn ON pull-up
		attachInterrupt(digitalPinToInterrupt(interruptPin), localKeyboardInterrupt, CHANGE);
	}

	I_Keypad::KeyOperation LocalKeypad::getAnalogueKey(uint32_t analogueReading) {
		//uint32_t adc_key_in = analogRead(_keyReadPin);  // read the value from the sensor
		//int i = 0;
		//while (adc_key_in != analogRead(_keyReadPin) && ++i < 100) {
		//	adc_key_in = analogRead(_keyReadPin);
		//}
															 // Convert ADC value to key number
		uint32_t keyPadRefV = analogRead(_keyHighRefPin);
		analogueReading *= 1024;
		analogueReading /= keyPadRefV;

		int key = 0;
		for (; key < sizeof(adc_LocalKey_val) / sizeof(adc_LocalKey_val[0]); ++key) {
			if (analogueReading > adc_LocalKey_val[key]) break;
		}

		switch (key) {
		case 0: return KEY_INFO;
		case 1: return KEY_UP;
		case 2: return KEY_LEFT;
		case 3: return KEY_RIGHT;
		case 4: return KEY_DOWN;
		case 5: return KEY_BACK;
		case 6: return KEY_SELECT;
		default: return NO_KEY;
		}
	};

#if defined (ZPSIM)
	int LocalKeypad::getKeyCode(int port) {
		int retVal = simKey;
		simKey = -1;
		return retVal;
	}
#else
	int LocalKeypad::getKeyCode(int) {
		// Cannot do Serial.print or logtoSD in here.
		MsTimer2::set(RE_READ_PERIOD_mS, getStableReading);
		MsTimer2::start();
		return NO_KEY;
	}
#endif

	void localKeyboardInterrupt() { // static or global interrupt handler, no arguments
		//static unsigned long nextPermissibleInterruptTime = 0;
		//if (millis() > nextPermissibleInterruptTime) {
			// When releasing the switch, it may bounce back on. 
			// But it must first go OFF which this handler will capture. It then prohibits re-capture for 5mS.
#ifndef ZPSIM
			//if (digitalRead(LocalKeypad::interruptPin) == LOW) {
#else
			//{
#endif
		LocalKeypad::indicatorLED().set();
		auto& keypad = *HardwareInterfaces::localKeypad;
		keypad.getKeyCode(0);
		//}
		LocalKeypad::indicatorLED().clear();
		//nextPermissibleInterruptTime = millis() + 5;
	//}
	}
}