#include <Arduino.h>
#include <I2C_Talk.h>
#include <Logging.h>
#include <avr/interrupt.h>
#include "RemoteKeypadMaster.h"
#include <Clock.h>
#include <Timer_mS_uS.h>
#define SERIAL_RATE 115200

//////////////////////////////// Start execution here ///////////////////////////////
namespace HardwareInterfaces { RemoteKeypadMaster remoteKeypad; }

using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
//int portb;
//int keyPin;

const uint8_t PROGRAMMER_I2C_ADDR = 0x11;

Clock& clock_() {
	static Clock _clock{};
	return _clock;
}

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_flush);
		return _log;
	}
}
using namespace arduino_logger;

I2C_Talk i2C(DS_REMOTE_MASTER_I2C_ADDR, Wire, 100000); // default 400kHz

//Pin_Wag led{ LED_BUILTIN, HIGH};

void setup()
{
  logger() << F("Start") << L_endl;
  for (int pin = 0; pin < 18; ++pin ) pinMode(pin, INPUT_PULLUP);
  i2C.begin();
  //led.begin();
}
	
auto keyName(int keyCode) -> const __FlashStringHelper* {
		switch (keyCode) {
		case -1: return F("None");
		case -2: return F("Multiple");
		case 2: return F("Left");
		case 4: return F("Down");
		case 1: return F("Up");
		case 3: return F("Right");
		default: return F("Err");
		}
}

Timer_mS readKey{ 10 };

void loop()
{
	if (readKey) {
		readKey.repeat();
		for (auto& pin : RemoteKeypadMaster::KeyPins) {
			auto keyState = pin.pinChanged();
			if (keyState == 1) {
				auto myKey = remoteKeypad.getKeyCode(pin.port());
				remoteKeypad.putKey(myKey);
			} else if (keyState == -1) {
				//led.clear();
				logger() << "Release\n";
			}
		}

	}

	auto key = remoteKeypad.getKey();
	if (key >= 0) {
		while (key >= 0) {
			//led.set();
			i2C.write(PROGRAMMER_I2C_ADDR, 0, key);
			logger() << "Send " << keyName(key) << L_endl;
			key = remoteKeypad.getKey();
		}
	}
}