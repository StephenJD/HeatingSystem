#include "RemoteKeypadMaster.h"
//#include <Logging.h>

//using namespace arduino_logger;
constexpr uint8_t US_REMOTE_MASTER_I2C_ADDR = 0x12;
constexpr uint8_t DS_REMOTE_MASTER_I2C_ADDR = 0x13;
constexpr uint8_t FL_REMOTE_MASTER_I2C_ADDR = 0x14;

namespace HardwareInterfaces {
	namespace {
		auto remName(int addr) -> const __FlashStringHelper* {
			if (addr == US_REMOTE_MASTER_I2C_ADDR) return F("UpSt");
			if (addr == DS_REMOTE_MASTER_I2C_ADDR) return F("DnSt");
			return F("Flat");
		};
	}

	Pin_Watch RemoteKeypadMaster::KeyPins[] = { {UP_PIN, LOW}, {DOWN_PIN, LOW}, {LEFT_PIN, LOW}, {RIGHT_PIN, LOW} };

	I_Keypad::KeyOperation RemoteKeypadMaster::getKeyCode() {
		auto active_port = 0;
		for (auto& pin : KeyPins) {
			if (pin.logicalState()) {
				active_port = pin.port();
				break;
			}
		}

		switch (active_port) {
			case UP_PIN: return KEY_UP;
			case DOWN_PIN: return KEY_DOWN;
			case LEFT_PIN: return KEY_LEFT;
			case RIGHT_PIN: return KEY_RIGHT;
			default: return NO_KEY;
		}
	}
}