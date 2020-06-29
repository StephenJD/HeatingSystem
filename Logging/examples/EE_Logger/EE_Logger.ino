#include <Arduino.h>
#include <Clock.h>
#include <Logging.h>
#include <I2C_Talk.h>
#include <EEPROM.h>
#include <SD.h>
#include <Watchdog_Timer.h>

#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0
#define SERIAL_RATE 115200
#define RTC_RESET 4

using namespace Date_Time;

/*
				 Due		Mega
SD Time Msg  :	3mS			3mS
SD NoTimeMsg :	1.4mS		1.4mS
Ram log Msg  :  100uS		700uS
Ram to SD/KB :	243mS		150mS
EP log Msg	 :	114mS		170mS
EP  to SD/KB :	660mS		230mS

*/

#if defined(__SAM3X8E__)

	I2C_Talk rtc(Wire1);

	EEPROMClass & eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ EEPROM_ADDR };
		return _eeprom_obj;
	}
	EEPROMClass & EEPROM = eeprom();

	Clock & clock_() {
		static Clock_I2C<rtc> _clock(RTC_ADDRESS);
		return _clock;
	}
#else
	#include <avr/wdt.h>

	uint8_t mcusr_mirror __attribute__((section(".noinit")));

	void wdt_init(void) __attribute__((naked, used, section(".init3")));
	// Function Implementation

	void wdt_init(void) // ensures WDT is disabled on re-boot to prevent repeated re-boots after a timeout
	{
		mcusr_mirror = MCUSR;
		MCUSR = 0;
		wdt_disable();
		return;
	}

	EEPROMClass & eeprom() {
		return EEPROM;
	}

	Clock & clock_() {
		static Clock_EEPROM _clock(EEPROM_CLOCK_ADDR);
		return _clock;
	}

#endif

Logger & logger() {
	//static Serial_Logger _log(SERIAL_RATE, clock_());
	static EEPROM_Logger _log("E.txt", 3000, 3500, false, clock_());
	return _log;
}

Logger & Slogger() {
	static Serial_Logger _log(SERIAL_RATE, clock_());
	return _log;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	//Slogger() << L_time << "Start" << L_endl;

#if defined(__SAM3X8E__)
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	rtc.restart();
#endif
	
	logger().begin();

	logger() << L_time << "Start" << L_endl;
	set_watchdog_timeout_mS(8000);
}

File dataFile;
constexpr int chipSelect = 53;

void loop()
{
	logger() << L_time << L_endl;
	Slogger() << L_time << L_endl;
	clock_().loadTime();
	delay(1000);
}