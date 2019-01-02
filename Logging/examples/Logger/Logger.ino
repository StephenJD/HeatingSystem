#include <Arduino.h>
#include <Wire.h>
#include <Conversions.h>
#include <Date_Time.h>
#include <I2C_Helper.h>
#include <Clock.h>
#include <EEPROM.h>
#include <SD.h>
#include <SPI.h>
#include <Logging.h>

#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0

#define RTC_RESET 4

I2C_Helper_Auto_Speed<2> * rtc{ 0 };
using namespace Date_Time;

Clock & clock_() {
	static I2C_Clock _clock(*rtc, RTC_ADDRESS);
	return _clock;
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
	Serial.begin(9600);
	Serial.println(" Serial Begun");
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin

	Serial.println(" Create rtc Helper");
	rtc = new I2C_Helper_Auto_Speed<2>(Wire1);
	rtc->setTimeoutFn(resetRTC);
	static_cast<I2C_Clock&>(clock_()).i2C_speedTest();

	logger().log_notime("Notime Logger Started");
	logger().log("Timed Logger Started");
	sdlogger().log("SD Timed Logger Started");
}

void loop()
{
	delay(10000);
	sdlogger().log("SD Timed Logger...");
}
