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
		: I_Keypad{ 10000, 30 } { //Display_Stream & displ) : Keypad(displ) 
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

	I_Keypad::KeyOperation LocalKeypad::getKeyCode() {
#if defined (ZPSIM)
		Serial.println("SimKey");
		putKey(simKey);
		simKey = NO_KEY;
		return simKey;
	}
#else
		uint32_t keyReading = analogRead(_keyReadPin);
		uint32_t keyPadRefV = analogRead(_keyHighRefPin);
		keyReading *= 1024;
		keyReading /= keyPadRefV;

		int key = 0;
		for (; key < sizeof(adc_LocalKey_val) / sizeof(adc_LocalKey_val[0]); ++key) {
			if (keyReading > adc_LocalKey_val[key]) break;
		}

		//Serial.print(keyReading);
		//Serial.print(F("\t"));
		//Serial.println(key);
		switch (key) {
		case 0: return KEY_INFO;
		case 1: return KEY_UP_OR_INFO;
		case 2: return KEY_LEFT;
		case 3: return KEY_RIGHT;
		case 4: return KEY_DOWN;
		case 5: return KEY_BACK;
		case 6: return KEY_SELECT;
		default: return NO_KEY;
		}
	}
#endif

	void localKeyboardInterrupt() { // static or global interrupt handler, no arguments
		LocalKeypad::indicatorLED().set();
		localKeypad->startRead();
		LocalKeypad::indicatorLED().clear();
	}
}