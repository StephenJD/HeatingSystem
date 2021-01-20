#include "RemoteKeypad.h"
#include <Logging.h>
namespace HardwareInterfaces {

#if defined (ZPSIM)
	int RemoteKeypad::readKey() {
		auto keyName = [](int keyCode) -> const __FlashStringHelper* {
			switch (keyCode) {
			case -1: return F("None");
			case -2: return F("Multiple");
			case 2: return F("Left");
			case 4: return F("Down");
			case 1: return F("Up");
			case 3: return F("Right");
			default: return F("Err");
			}
		};

		auto remName = [](int addr) -> const __FlashStringHelper* {
			if (addr == 0x24) return F("US");
			if (addr == 0x25) return F("Fl");
			if (addr == 0x26) return F("DS");
		};		
		
		auto myKey = getKeyCode(_lcd->readI2C_keypad());
		myKey = simKey;
		if (putInKeyQue(keyQue, keyQuePos, myKey)) {
			logger() << remName(_lcd->i2cAddress()) << F(" Remote Keypad ")  << keyName(myKey) << L_endl;
		}
		simKey = -1;
		return myKey;
	}
	int RemoteKeypad::getKeyCode(int gpio) { return 0; }
#else
	int RemoteKeypad::readKey() {
		auto keyName = [](int keyCode) -> const __FlashStringHelper* {
			switch (keyCode) {
			case -1: return F("None");
			case -2: return F("Multiple");
			case 2: return F("Left");
			case 4: return F("Down");
			case 1: return F("Up");
			case 3: return F("Right");
			default: return F("Err");
			}
		};

		auto remName = [](int addr) -> const __FlashStringHelper* {
			if (addr == 0x24) return F("US");
			if (addr == 0x25) return F("Fl");
			if (addr == 0x26) return F("DS");
		};

		auto myKey = getKeyCode(_lcd->readI2C_keypad());
		if (myKey == -2) {
			logger() << remName(_lcd->i2cAddress()) << F(" Remote Keypad: multiple keys detected.") << L_endl;
		}
		if (putInKeyQue(keyQue, keyQuePos, myKey)) {
			logger() << remName(_lcd->i2cAddress()) << F(" Remote Keypad ") << keyName(myKey) << L_endl;
		}
		return myKey;
	}

	int RemoteKeypad::getKeyCode(int gpio) { // readI2C_keypad() returns 16 bit register
		int myKey;
		switch (gpio) {
		case 0x01: myKey = 2; break;
		case 0x20: myKey = 4; break;
		case 0x40: myKey = 1; break;
		case 0x80: myKey = 3; break;
		case 0:    myKey = -1; break;
		default: myKey = -2; // multiple keys
		}
		//rc->clear(); rc->print(keyName(myKey));
		return myKey;
	}
#endif

	int RemoteKeypad::getKey() {
		auto gotKey = readKey();
		if (gotKey >= 0) {
			gotKey = getFromKeyQue(keyQue, keyQuePos);
		}
		return gotKey;
	}
}