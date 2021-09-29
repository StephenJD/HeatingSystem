#include <Arduino.h>
//#include <I2C_Talk.h>
//#include <Logging.h>
//#include <avr/interrupt.h>
#include "OLED_Master_Display.h"
#include <Clock.h>

#define SERIAL_RATE 115200

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

void setup() {
  Serial.begin(SERIAL_RATE);
  //logger() << F("Start") << L_endl;
  for (int pin = 0; pin < 18; ++pin) pinMode(pin, INPUT_PULLUP);
  OLED_Master_Display::begin();
}

void loop() {
	OLED_Master_Display::processKeys();
}