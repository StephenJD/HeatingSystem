#include "RemoteKeypad.h"
#include <Logging.h>
namespace HardwareInterfaces {

#if defined (ZPSIM)
	int RemoteKeypad::readKey() {
		auto myKey = simKey;
		putInKeyQue(keyQue, keyQuePos, myKey);
		simKey = -1;
		return myKey;
	}
	int RemoteKeypad::getKeyCode(int gpio) { return 0; }
#else
	int RemoteKeypad::readKey() {
		auto myKey = getKeyCode(_lcd->readI2C_keypad());
		if (myKey == -2) {
			logger() << F("Remote Keypad: multiple keys detected") << L_endl;
		}
		if (putInKeyQue(keyQue, keyQuePos, myKey)) {
			logger() << F("Remote Keypad Read Key. Add : ") << myKey << L_endl;
		}
		return myKey;
	}

	int RemoteKeypad::getKeyCode(int gpio) { // readI2C_keypad() returns 16 bit register
		int myKey;
		switch (gpio) {
		case 0x01: myKey = 2; break; //rc->clear(); rc->print("Left"); break;
		case 0x20: myKey = 4; break; // rc->clear(); rc->print("Down"); break;
		case 0x40: myKey = 1; break; // rc->clear(); rc->print("Up"); break;
		case 0x80: myKey = 3; break; // rc->clear(); rc->print("Right"); break;
		case 0:    myKey = -1; break;
		default: myKey = -2; // multiple keys
		}
		return myKey;
	}
#endif

	int RemoteKeypad::getKey() {
		auto gotKey = readKey();
		if (gotKey >= 0) {
			wakeDisplay(true);
			gotKey = getFromKeyQue(keyQue, keyQuePos);
		}
		return gotKey;
	}
}