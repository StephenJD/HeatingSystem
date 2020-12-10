#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Scan.h>
#include <Logging.h>
#include <EEPROM.h>

#define SERIAL_RATE 115200
#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0
#define RTC_RESET 4

using namespace Date_Time;

#if defined(__SAM3X8E__)
	#include <Wire.h>
	#include <Clock_I2C.h>

	I2C_Talk rtc(Wire1);
	I_I2C_Scan scanner{ rtc };

	EEPROMClass & eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1), 0x50) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}

	Clock & clock_() {
		static Clock_I2C<rtc> _clock(RTC_ADDRESS);
		return _clock;
	}
#else
	#define NO_RTC
	#define EEPROM_CLOCK EEPROM_CLOCK_ADDR
	#include <Clock_EP.h>

	EEPROMClass & eeprom() {
		return EEPROM;
	}
#endif

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE, clock_());
	return _log;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	logger() << L_allwaysFlush << F(" \n\n********** Logging Begun ***********") << L_endl;

#if !defined NO_RTC
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	rtc.begin();
	scanner.show_all();
#endif

	logger() << L_time <<  "\nLoad Time..." << L_endl;
	//clock_().loadTime();
	//logger() << "\nSave Time..." << L_endl;
	//clock_().saveTime();
	//logger() << "\nStart loop..." << L_endl;
}

void loop() {
	logger() << clock_() << L_endl;
	delay(1000);
}