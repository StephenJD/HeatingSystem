#include <Clock.h>
#include <Logging.h>
#include <Date_Time.h>
#include <I2C_Helper.h>
#include <Conversions.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>

#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0

#define RTC_RESET 4
using namespace Date_Time;

#if defined(__SAM3X8E__)
#include <EEPROM.h>
EEPROMClass & eeprom() {
	static EEPROMClass _eeprom_obj{ 0, 0x50 };
	return _eeprom_obj;
}


EEPROMClass & EEPROM = eeprom();
#endif

I2C_Helper & rtc() {
  static I2C_Helper_Auto_Speed<2> _rtc{ Wire1 };
  return _rtc;
}

Clock & clock_() {
  static I2C_Clock _clock(&rtc(), RTC_ADDRESS);
  return _clock;
}

Logger & timelessLogger() {
  static Serial_Logger _log(9600);
  return _log;
}

Logger & logger() {
  static Serial_Logger _log(9600, clock_());
  return _log;
}

Logger & sdlogger() {
  static SD_Logger _log("testlog.txt", 9600, clock_());
  return _log;
}

uint8_t resetRTC(I2C_Helper & i2c, int) {
  Serial.println("Reset RTC...");
  digitalWrite(RTC_RESET, HIGH);
  delayMicroseconds(10000);
  digitalWrite(RTC_RESET, LOW);
  delayMicroseconds(10000);
  i2c.restart();
  return i2c.result.error;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
  Serial.begin(9600); // NOTE! Serial.begin must be called before i2c_clock is constructed.
  logger().log(" Setup Start");
  pinMode(RTC_RESET, OUTPUT);
  digitalWrite(RTC_RESET, LOW); // reset pin
  rtc().setTimeoutFn(resetRTC);
  auto & i2cClock = static_cast<I2C_Clock&>(clock_());
  i2cClock.i2C_speedTest();
  logger().log_notime("Notime Logger Message");
  logger().log("Timed Logger Message");
  sdlogger().log("SD Timed Logger Started");
  i2cClock.saveTime();
  i2cClock.loadTime();
}

void loop()
{
  delay(5000);
  sdlogger().log("SD Timed Logger...");
  timelessLogger().log("timelessLogger");
}