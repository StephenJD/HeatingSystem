#include "RemoteKeypad.h"
//#include <Logging.h>

//using namespace arduino_logger;

namespace HardwareInterfaces {

#if defined (ZPSIM)
	I_Keypad::KeyOperation RemoteKeypad::getKeyCode() {
		wakeDisplay();
		if (displayIsAwake()) {
			putKey(simKey);
		}
		simKey = NO_KEY; 
		return simKey;
	}
#else

	I_Keypad::KeyOperation RemoteKeypad::getKeyCode() { // readI2C_keypad() returns 16 bit register
		// Lambda
		auto keyName = [](int keyCode) -> const __FlashStringHelper* {
			switch (keyCode) {
			case NO_KEY: return F("None");
			case BAD_KEY: return F("Multiple");
			case KEY_LEFT: return F("Left");
			case KEY_DOWN: return F("Down");
			case KEY_UP: return F("Up");
			case KEY_RIGHT: return F("Right");
			default: return F("Err");
			}
		};

		// Algorithm
		wakeDisplay();
		auto port = _lcd->readI2C_keypad();
		KeyOperation myKey;
		switch (port) {
		case 0x0100: myKey = KEY_LEFT; break;
		case 0x2000: myKey = KEY_DOWN; break;
		case 0x4000: myKey = KEY_UP; break;
		case 0x8000: myKey = KEY_RIGHT; break;
		case 0:		 myKey = NO_KEY; break;
		default:
			//logger() << L_time << remName(_lcd->i2cAddress()) << F(" Remote Keypad: MultipleKeys: 0x") << L_hex << gpio << L_endl;
			myKey = BAD_KEY; // multiple keys
		}

		if (myKey > NO_KEY) {
			//logger() << L_time << remName(_lcd->i2cAddress()) << F(" Remote Keypad ") << keyName(myKey) << L_endl;
		}
		if (myKey / 2 == MODE_LEFT_RIGHT) myKey = NO_KEY;
		return myKey;
	}
#endif

	void RemoteKeypad::startRead() {
		putKey(getKeyCode());
	}

}