#include "RemoteKeypad.h"
#include <Logging.h>
namespace HardwareInterfaces {
	namespace {
		auto remName(int addr) -> const __FlashStringHelper* {
			if (addr == 0x24) return F("UpSt");
			if (addr == 0x25) return F("Flat");
			return F("DnSt");
		};
	}

#if defined (ZPSIM)
	int RemoteKeypad::readKey() {
		auto myKey = getKeyCode(_lcd->readI2C_keypad());
		myKey = simKey;
		putInKeyQue(keyQue, keyQuePos, myKey);
		simKey = -1;
		return myKey;
	}
	int RemoteKeypad::getKeyCode(int gpio) { return 0; }
#else
	int RemoteKeypad::readKey() {
		auto myKey = getKeyCode(_lcd->readI2C_keypad());
		putInKeyQue(keyQue, keyQuePos, myKey);
		return myKey;
	}

	int RemoteKeypad::getKeyCode(int gpio) { // readI2C_keypad() returns 16 bit register
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
		
		int myKey;
		switch (gpio) {
		case 0x0100: myKey = 2; break; // Left
		case 0x2000: myKey = 4; break; // Down
		case 0x4000: myKey = 1; break; // Up
		case 0x8000: myKey = 3; break; // Right
		case 0:		 myKey = -1; break;
		default:
			logger() << remName(_lcd->i2cAddress()) << F(" Remote Keypad: GPIO err: 0x") << L_hex << gpio << L_endl;
			myKey = -2; // multiple keys
		}

		if (myKey > -1) {
			logger() << L_time << remName(_lcd->i2cAddress()) << F(" Remote Keypad ") << keyName(myKey) << L_endl;
		} 
		if (myKey == 2 || myKey == 3) myKey = -1;
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