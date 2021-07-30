#include "RemoteKeypad.h"
//#include <Logging.h>

//using namespace arduino_logger;

namespace HardwareInterfaces {

#if defined (ZPSIM)
	void RemoteKeypad::readKey(int) {
		auto myKey = getKeyCode(_lcd->readI2C_keypad());
		myKey = simKey;
		putKey(myKey);
		simKey = -1;
	}
	int RemoteKeypad::getKeyCode(int gpio) { return 0; }
#else
	void RemoteKeypad::readKey(int) {
		auto myKey = -1;
		if (timeToRead) {
			myKey = getKeyCode(_lcd->readI2C_keypad());
			putKey(myKey);
			timeToRead.repeat();
		}
	}

	int RemoteKeypad::getKeyCode(int port) { // readI2C_keypad() returns 16 bit register
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

		int myKey;
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

	int RemoteKeypad::getKey() {
		wakeDisplay(); // Prevent display sleeping.
		auto gotKey = I_Keypad::getKey();
		if (gotKey > NO_KEY) {
			//logger() << F(" Remote-Master Key: ") << gotKey << L_endl;
		}
		if (gotKey == NO_KEY) {
			readKey(_lcd->readI2C_keypad());
			gotKey = I_Keypad::getKey();
		}
		return gotKey;
	}
}