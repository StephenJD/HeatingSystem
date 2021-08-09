#include <Arduino.h>
#include <I2C_Talk.h>
//#include <Logging.h>
#include <avr/interrupt.h>
#include "OLED_Master_Display.h"
#include <Clock.h>

#define SERIAL_RATE 115200

//using namespace I2C_Talk_ErrorCodes;
//int portb;
//int keyPin;

constexpr uint8_t PROGRAMMER_I2C_ADDR = 0x11;
constexpr uint8_t DS_REMOTE_MASTER_I2C_ADDR = 0x12;

Clock& clock_() {
  static Clock _clock{};
  return _clock;
}

//namespace arduino_logger {
//	Logger& logger() {
//		static Serial_Logger _log(SERIAL_RATE, L_flush);
//		return _log;
//	}
//}
//using namespace arduino_logger;

I2C_Talk i2C(DS_REMOTE_MASTER_I2C_ADDR, Wire, 100000); // default 400kHz

void setup() {
  Serial.begin(SERIAL_RATE);
  //logger() << F("Start") << L_endl;
  for (int pin = 0; pin < 18; ++pin) pinMode(pin, INPUT_PULLUP);
  i2C.begin();
  OLED_Master_Display::begin();
}

void loop() {
	OLED_Master_Display::processKeys();
}