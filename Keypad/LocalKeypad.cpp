#include "LocalKeypad.h"
#include <Logging.h>

//extern unsigned long processStart_mS;

namespace HardwareInterfaces {
	LocalKeypad * localKeypad;
	constexpr int16_t LocalKeypad::adc_LocalKey_val[];
	uint8_t LocalKeypad::interruptPin;
	//Pin_Wag LocalKeypad::_indicatorLED;
	uint8_t LocalKeypad::_indicatorLED;
	uint8_t LocalKeypad::_keyReadPin;
	uint8_t LocalKeypad::_keyHighRefPin;

	LocalKeypad::LocalKeypad(int interrupt_Pin, int keyRead_Pin, int keyHighRef_Pin, Pin_Wag indicator_LED) { //Display_Stream & displ) : Keypad(displ) 
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

#if defined (ZPSIM)
	int LocalKeypad::readKey() {
		int retVal = simKey;
		simKey = -1;
		return retVal;
	}
#else
	int LocalKeypad::readKey() {
		// Cannot do Serial.print or logtoSD in here.
		uint32_t adc_key_in = analogRead(_keyReadPin);  // read the value from the sensor
		int i = 0;
		while (adc_key_in != analogRead(_keyReadPin) && ++i < 100) {
			adc_key_in = analogRead(_keyReadPin);
		}
															 // Convert ADC value to key number
		uint32_t keyPadRefV = analogRead(_keyHighRefPin);
		adc_key_in *= 1024;
		adc_key_in /= keyPadRefV;

		int key = 0;
		for (; key < sizeof(adc_LocalKey_val)/2; ++key) {
			if (adc_key_in > adc_LocalKey_val[key]) break;
		}

		if (key == sizeof(adc_LocalKey_val)/2) key = -1;     // No valid key pressed
		return key;
	}
#endif

	int LocalKeypad::getKey() {
		auto returnKey = getFromKeyQue(keyQue, keyQuePos);
		if (returnKey >= 0) {
//#if !defined (ZPSIM)
			if (_secsToKeepAwake == 0) returnKey = sizeof(adc_LocalKey_val)/2;
//#endif
			wakeDisplay(true);
		}
		return returnKey;
	}

	bool LocalKeypad::isTimeToRefresh() {
		bool doRefresh = I_Keypad::isTimeToRefresh(); // refresh every second
		return doRefresh;
	}


	void localKeyboardInterrupt() { // static or global interrupt handler, no arguments
		static unsigned long nextPermissibleInterruptTime = 0;
		if (millis() > nextPermissibleInterruptTime) {
			// When releasing the switch, it may bounce back on. 
			// But it must first go OFF which this handler will capture. It then prohibits re-capture for 5mS.
#ifndef ZPSIM
			if (digitalRead(LocalKeypad::interruptPin) == LOW) {
#else
			{
#endif
				LocalKeypad::indicatorLED().set();
				auto & keypad = *HardwareInterfaces::localKeypad;
				auto myKey = keypad.readKey();
				while (myKey == 1) { // must keep reading, since a second key-press won't gernerate an interrupt
					auto newKey = keypad.readKey(); // see if a second key is pressed
					if (newKey == 0) {
						myKey = 0; break;
					} else if (newKey == -1) break;
				}
				//processStart_mS = millis();
				putInKeyQue(keypad.keyQue, keypad.keyQuePos, myKey);
			}
			LocalKeypad::indicatorLED().clear();
			nextPermissibleInterruptTime = millis() + 5;
		}
	}
}