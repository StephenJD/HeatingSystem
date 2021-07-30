#include "RemoteKeypadMaster.h"
//#include <Logging.h>

//using namespace arduino_logger;

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

	int RemoteKeypadMaster::getKeyCode(int port) {
		auto getKeyCodeForPort = [](int pinNo) {
			int myKey;
			switch (pinNo) {
			case UP_PIN: myKey = KEY_UP; break; // Up
			case DOWN_PIN: myKey = KEY_DOWN; break; // Down
			case LEFT_PIN: myKey = KEY_LEFT; break; // Left
			case RIGHT_PIN: myKey = KEY_RIGHT; break; // Right
			case 0:		 myKey = NO_KEY; break;
			default:
				myKey = BAD_KEY; // multiple keys
			}

			return myKey;
		};

		for (auto& pin : KeyPins) {
			if (pin.pinChanged() == 1) {
				return getKeyCodeForPort(getKeyCode(pin.port()));
			}
		}
		return NO_KEY;
	}
}