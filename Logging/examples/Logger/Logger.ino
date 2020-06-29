#include <Arduino.h>
#include <Clock.h>
#include <Logging.h>
#include <I2C_Talk.h>
#include <EEPROM.h>
#include <SD.h>

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

EEPROMClass & eeprom() {
	return EEPROM;
}

Clock & clock_() {
	static Clock_EEPROM _clock(EEPROM_CLOCK_ADDR);
	return _clock;
}

#endif

Logger & timelessLogger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

Logger & nullLogger() {
	static Logger _log{};
	return _log;
}

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE, clock_());
	return _log;
}

Logger & sdlogger() {
	//static Serial_Logger _log(SERIAL_RATE);
	static SD_Logger _log("testlog.txt", SERIAL_RATE, clock_());
	return _log;
}

Logger & ramlogger() {
	static RAM_Logger _log("R.txt", 1000, false, clock_());
	return _log;
}

Logger & eplogger() {
	static EEPROM_Logger _log("E.txt", 3000, 4000, false, clock_());
	return _log;
}

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	auto start = millis();
	Serial.begin(SERIAL_RATE); // NOTE! Serial.begin must be called before i2c_clock is constructed.
	auto took = millis() - start;

	start = millis();
	logger(); // 2 mS Due/Mega
	took = millis() - start;
	Serial.print("Logger start took "); Serial.println(took);

	start = millis();
	logger() /*<< L_allwaysFlush */<< " Setup Start" << L_endl; // 0 mS
	took = millis() - start;
	Serial.print("Logger print took "); Serial.println(took);

#if defined(__SAM3X8E__)
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	rtc.restart();
#endif
	logger() << "Notime Logger Message\n";
	start = millis();
	logger() << L_time << L_endl; // 5/6 mS Due/Mega
	took = millis() - start;
	Serial.print("Logger print time took "); Serial.println(took);

	start = millis();
	logger() << L_tabs << "Tabs. Text. Int 125:" << 125 << "Float 1.5:" << 1.5 << L_endl; // 3/4 mS Due/Mega
	took = millis() - start;
	Serial.print("Logger print float took "); Serial.println(took);

	start = millis();
	logger() << L_tabs << L_fixed << "Tabs Fixed 12.75:" << (12 * 256 + 128 + 64) << L_endl;
	took = millis() - start;
	Serial.print("Logger print fixed took "); Serial.println(took);
	
	start = millis();
	sdlogger(); // 85/87 mS Due/Mega
	took = millis() - start;
	Serial.print("SDLogger start took "); Serial.println(took);
	
	start = millis();
	sdlogger() << L_time << L_endl; // 72/2 mS Due/Mega.
	took = millis() - start;
	Serial.print("SDLogger print time took "); Serial.println(took);

	start = millis();
	sdlogger() << L_tabs << L_fixed << "SD tabbed: Fixed 12.75:" << (12 * 256 + 128 + 64) << L_endl;
	sdlogger() << L_time << L_tabs << "sd tabbed" << 5 << L_concat << "untabbed" << 6 << L_endl;
	sdlogger() << L_time << "sd not tabbed" << 5 << L_endl;
	sdlogger() << L_time << "sd log" << 5 << L_tabs << "tabbed" << 6 << L_endl;
	sdlogger() << L_cout << "sd cout" << 5 << L_tabs << "tabbed" << 6 << L_endl;
	sdlogger() << "sd log after cout" << 5 << L_tabs << "tabbed" << 6 << L_endl;
	took = millis() - start;
	Serial.print("SDLogger stuff took "); Serial.println(took); // 8/14 mS Due/Mega. 

	start = millis();
	for (int i = 0; i < 10; ++i) { // 31/32 mS Due/Mega
		sdlogger() << L_time << "sdlogger count " << i << L_endl;
	}
	took = millis() - start;
	Serial.print("sdlogger time x10 took "); Serial.println(took);

	start = millis();
	for (int i = 0; i < 10; ++i) { // 14/14 mS Due/Mega
		sdlogger() << "sdlogger count " << i << L_endl;
	}
	took = millis() - start;
	Serial.print("sdlogger no time x10 took "); Serial.println(took);

	start = millis();
	ramlogger(); // 0mS
	took = millis() - start;
	Serial.print("RAMLogger start took "); Serial.println(took);


	start = millis();
	for (int i = 0; i < 10; ++i) { // 1/6 mS Due/Mega
		ramlogger() << L_time << "ram log count " << i << L_endl;
	}
	took = millis() - start;
	Serial.print("RAMLogger time x10 took "); Serial.println(took);

	start = millis();
	for (int i = 0; i < 10; ++i) { // 0/1 mS Due/Mega
		ramlogger() << "ram log count " << i << L_endl;
	}
	took = millis() - start;
	Serial.print("RAMLogger no time x10 took "); Serial.println(took);

	start = millis();
	for (int i = 0; i < 70; ++i) { // 7/48 mS Due/Mega
		ramlogger() << L_time << "ram log count " << i << L_endl;
	}
	took = millis() - start;
	Serial.print("RAMLogger x70 took "); Serial.println(took);
	
	start = millis();
	ramlogger().readAll(); // 243/148 mS Due/Mega
	took = millis() - start;
	Serial.print("RAMLogger read took "); Serial.println(took);
	
	logger() << "Trigger dump" << L_endl;
	start = millis();
	eplogger(); // 661/223 mS Due/Mega
	took = millis() - start;
	Serial.print("EPLogger start took "); Serial.println(took);
	
	//logger() << "Fill Log" << L_endl;
	//start = millis();
	//for (int i = 0; i < 70; ++i) { // 8/11.7 S Due/Mega
	//	eplogger() << L_time << "EP log count " << i << L_endl;
	//}
	//took = millis() - start;
	//Serial.print("EPLogger x70 took "); Serial.println(took);
	
	logger() << "Request Read" << L_endl;
	start = millis();
	eplogger().readAll(); // 652/180 mS Due/Mega
	took = millis() - start;
	Serial.print("EPLogger read took "); Serial.println(took);
	
	
	timelessLogger() << "timelessLogger\n";
	timelessLogger() << L_time << "Time? timelessLogger\n";

	nullLogger() << "Nothing";
	clock_().saveTime();
	clock_().loadTime();
	timelessLogger() << "End\n";
}

File dataFile;
constexpr int chipSelect = 53;

void loop()
{
	static bool lastState;
	static int i;

	sdlogger() << L_time << "sd " << i << (SD.sd_exists(chipSelect) ? " OK" : " Failed") << L_endl;
	//dataFile = SD.open("SD_Test.txt", FILE_WRITE);
	delay(1000);
}