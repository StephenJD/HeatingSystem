#include <Arduino.h>
#include <Wire.h>
#include <Conversions.h>
#include <Date_Time.h>
#include <I2C_Talk.h>
#include <Clock.h>

#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0

#define RTC_RESET 4

I2C_Talk_Auto_Speed<2> * rtc(0);
using namespace Date_Time;

#if defined(__SAM3X8E__)
//EEPROMClass eeprom_obj{ i2C_EEPROM, EEPROM_ADDR, RTC_ADDRESS };
//EEPROMClass & EEPROM = eeprom_obj;
#endif

Clock & clock_() {
#if defined(__SAM3X8E__)
	static I2C_Clock _clock(*rtc, RTC_ADDRESS);
#else
	static EEPROM_Clock _clock(EEPROM_CLOCK_ADDR);
	#define NO_RTC
#endif
	return _clock;
}

uint8_t resetRTC(I2C_Talk & i2c, int) {
	Serial.println("Reset RTC...");
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(10000);
	digitalWrite(RTC_RESET, LOW);
	delayMicroseconds(10000);
	i2c.restart();
	return I2C_Talk::_OK;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	Serial.println(" Serial Begun");

#if !defined NO_RTC
  pinMode(RTC_RESET, OUTPUT);
  digitalWrite(RTC_RESET, LOW); // reset pin
  Serial.println(" Create rtc Helper");
	rtc = new I2C_Talk_Auto_Speed<2>(Wire1);
	//rtc->setTimeoutFn(resetRTC);

	static_cast<I2C_Clock&>(clock_()).i2C_speedTest();
#endif
  Serial.println(" Save Time");
	clock_().saveTime();
	clock_().loadTime();
}

void loop() {
	// put your main code here, to run repeatedly:
	auto dt = DateTime{ clock_() };
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
	clock_().refresh();
}
