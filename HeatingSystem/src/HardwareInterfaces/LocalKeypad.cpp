#include "LocalKeypad.h"
#include "Logging.h"
extern byte LOCAL_INT_PIN;
extern byte KEY_ANALOGUE;

namespace HardwareInterfaces {
	LocalKeypad * localKeypad;
	constexpr int16_t LocalKeypad::adc_LocalKey_val[]; // for analogue keypad

	LocalKeypad::LocalKeypad() { //Display_Stream & displ) : Keypad(displ) 
		logger().log("LocalKeypad Start...");
		localKeypad = this;
		digitalWrite(LOCAL_INT_PIN, HIGH); // turn ON pull-up 
		//attachInterrupt(digitalPinToInterrupt(LOCAL_INT_PIN), localKeyboardInterrupt, FALLING); 
		 attachInterrupt(LOCAL_INT_PIN, localKeyboardInterrupt, CHANGE);
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
		auto adc_key_in = analogReadDelay(KEY_ANALOGUE);  // read the value from the sensor
															 //S2_byte anRef = analogReadDelay(REF_ANALOGUE); // read Voltage Ref
		while (adc_key_in != analogReadDelay(KEY_ANALOGUE)) adc_key_in = analogReadDelay(KEY_ANALOGUE);
		// correct adc_key_in for non 5v ref
		//adc_key_in = S2_byte(adc_key_in * 1023L / anRef);
		// Convert ADC value to key number
		int key = 0;
		for (; key < NUM_LOCAL_KEYS; ++key) {
			if (adc_key_in > adc_LocalKey_val[key]) break;
		}

		if (key >= NUM_LOCAL_KEYS) key = -1;     // No valid key pressed
		return key;
	}
#endif

	int LocalKeypad::analogReadDelay(int an_pin) {
		delayMicroseconds(5000); // delay() does not work during an interrupt
		return analogRead(an_pin);   // read the value from the keypad
	}



	int LocalKeypad::getKey() {
		auto returnKey = getFromKeyQue(keyQue, keyQuePos);
		if (returnKey >= 0) {
#if !defined (ZPSIM)
			if (_timeSince == 0) returnKey = -1;
#endif
			wakeDisplay(true);
			//displayStream()->setBackLight(true);
		}
		return returnKey;
	}

	bool LocalKeypad::isTimeToRefresh() {
		bool doRefresh = I_Keypad::isTimeToRefresh(); // refresh every second
		//if (_timeSince <= 0) displayStream()->setBackLight(false);
		return doRefresh;
	}

}

void localKeyboardInterrupt() { // static or global interrupt handler, no arguments
		
		// lastTime check required to prevent multiple interrupts from each key press.
		static unsigned long lastTime = millis();
		if (millis() < lastTime) return;
		lastTime = millis() + 2;

		auto & keypad = *HardwareInterfaces::localKeypad;
		auto myKey = keypad.readKey();
		auto newKey = myKey;
		while (myKey == 1 && newKey == myKey) {
			newKey = keypad.readKey(); // see if a second key is pressed
			if (newKey >= 0) myKey = newKey;
		}
		putInKeyQue(keypad.keyQue, keypad.keyQuePos, myKey);
	}