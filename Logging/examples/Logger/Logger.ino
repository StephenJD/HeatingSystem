#include <Arduino.h>
#include <Clock.h>
#include <Logging.h>
#include <I2C_Talk.h>
#include <EEPROM.h>

#define RTC_ADDRESS 0x68
#define EEPROM_ADDR 0x50
#define EEPROM_CLOCK_ADDR 0

#define RTC_RESET 4

using namespace Date_Time;

//#if defined(__SAM3X8E__)



I2C_Talk rtc(Wire1);

EEPROMClass & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{ 0x50 };
	return _eeprom_obj;
}

EEPROMClass & EEPROM = eeprom();
//#endif

Clock & clock_() {
	//static Clock_EEPROM _clock(EEPROM_CLOCK_ADDR);
	static Clock_I2C<rtc> _clock(RTC_ADDRESS);
	return _clock;
}

Logger & timelessLogger() {
	static Serial_Logger _log(9600);
	return _log;
}

Logger & nullLogger() {
	static Logger _log{};
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

Logger & eplogger() {
	static EEPROM_Logger _log(6000, 6100);
	return _log;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	logger() << " Setup Start" << L_flush;
#if defined(__SAM3X8E__)
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	rtc.restart();
#endif
	logger() << "Notime Logger Message\n";
	logger() << L_time << "Timed Logger Message\n";
	logger() << L_tabs << "Tabs. Text. Int 125:" << 125 << "Float 1.5:" << 1.5 << L_endl;
	logger() << L_tabs << L_fixed << "Tabs Fixed 12.75:" << (12 * 256 + 128 + 64) << L_flush;
	sdlogger() << L_time << "SD Timed Logger Started\n";
	sdlogger() << L_tabs << L_fixed << "SD tabbed: Fixed 12.75:" << (12 * 256 + 128 + 64) << L_endl;
	sdlogger() << L_time << L_tabs << "sd tabbed" << 5 << L_concat << "untabbed" << 6 << L_endl;
	sdlogger() << L_time << "sd not tabbed" << 5 << L_endl;
	sdlogger() << L_time << "sd log" << 5 << L_tabs << "tabbed" << 6 << L_endl;
	sdlogger() << L_cout << "sd cout" << 5 << L_tabs << "tabbed" << 6 << L_endl;
	sdlogger() << "sd log after cout" << 5 << L_tabs << "tabbed" << 6 << L_endl;
	logger() << "SD finished" << L_flush;
	eplogger() << "ep Text." << L_endl;
	eplogger() << L_tabs << "ep Text. Int 125:" << 125 << "Float 1.5:" << 1.5 << L_fixed << "Fixed 12.75:" << (12 * 256 + 128 + 64) << L_endl;
	clock_().saveTime();
	clock_().loadTime();
	timelessLogger() << "timelessLogger\n";
	nullLogger() << "Nothing";
	eplogger().readAll();
}

void loop()
{
}