#include "RemoteKeypadMaster.h"
#include <Logging.h>

using namespace arduino_logger;

namespace HardwareInterfaces {
	namespace {
		auto remName(int addr) -> const __FlashStringHelper* {
			if (addr == US_REMOTE_MASTER_I2C_ADDR) return F("UpSt");
			if (addr == DS_REMOTE_MASTER_I2C_ADDR) return F("DnSt");
			return F("Flat");
		};
	}

	void keyInterrupt();

	Pin_Watch_Debounced RemoteKeypadMaster::KeyPins[] = { {UP_PIN, LOW}, {DOWN_PIN, LOW}, {LEFT_PIN, LOW}, {RIGHT_PIN, LOW} };

	int RemoteKeypadMaster::getKeyCode(int gpio) {
		int myKey;
		switch (gpio) {
		case LEFT_PIN: myKey = 2; break; // Left
		case DOWN_PIN: myKey = 4; break; // Down
		case UP_PIN: myKey = 1; break; // Up
		case RIGHT_PIN: myKey = 3; break; // Right
		case 0:		 myKey = -1; break;
		default:
			logger() << L_time << /*remName(_lcd->i2cAddress()) <<*/ F(" Remote Keypad: MultipleKeys: 0x") << L_hex << gpio << L_endl;
			myKey = -2; // multiple keys
		}

		//if (myKey > -1) {
			//logger() << L_time << /*remName(_lcd->i2cAddress()) <<*/ F(" Remote Keypad ") << myKey << L_endl;
		//} 
		//if (myKey == 2 || myKey == 3) myKey = -1;
		return myKey;
	}
}