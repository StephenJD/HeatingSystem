#include <Arduino.h>
#include <Wire.h>
#include <Conversions.h>
#include <Date_Time.h>
#include <I2C_Talk.h>
#include <I2C_Scan.h>
#include <Clock.h>
#include <Logging.h>
#include <SD.h>
#include <SPI.h>
#include <EEPROM.h>

#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0

#define RTC_RESET 4

I2C_Talk rtc(Wire1, 100000);
I_I2C_Scan scanner{ rtc };

using namespace Date_Time;

#if defined(__SAM3X8E__)
//EEPROMClass eeprom_obj{ i2C_EEPROM, EEPROM_ADDR, RTC_ADDRESS };
//EEPROMClass & EEPROM = eeprom_obj;
#endif

Clock & clock_() {
#if defined(__SAM3X8E__)
	static Clock_I2C<rtc> _clock(RTC_ADDRESS);
#else
	static Clock_EEPROM _clock(EEPROM_CLOCK_ADDR);
#define NO_RTC
#endif
	return _clock;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	Serial.println(" Serial Begun");

#if !defined NO_RTC
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	rtc.restart();

#endif
	scanner.show_all();

	Serial.println(" Save Time");
	clock_().saveTime();
	clock_().loadTime();
}

void loop() {
	// put your main code here, to run repeatedly:
	auto dt = clock_().now();
	Serial.print(dt.getDayStr());
	Serial.print(" ");
	Serial.print(dt.day());
	Serial.print("/");
	Serial.print(dt.getMonthStr());
	Serial.print("/20");
	Serial.print(dt.year());
	Serial.print(" ");
	Serial.print(dt.hrs());
	Serial.print(":");
	Serial.print(dt.mins10());
	Serial.print(clock_().minUnits());
	Serial.print(":");
	Serial.println(clock_().seconds());
	delay(1000);
}