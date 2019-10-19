#include <Arduino.h>
#include <I2C_Talk.h>
#include <Wire.h>
#include <EEPROM.h>
#include <I2C_Scan.h>
#include <I2C_SpeedTest.h>
#include <I2C_RecoverRetest.h>
#include <Clock.h>
#include <Logging.h>

#define RTC_RESET 4
using namespace I2C_Recovery;


I2C_Talk rtc_o(Wire1);

I2C_Talk & rtc() {
	static I2C_Talk _rtc{ Wire1 };
	return _rtc;
}

int st_index;
//I2C_Scan scan{ rtc() };              // works fine
//I2C_SpeedTest speed{ rtc() };        // works fine
//I2C_Recover_Retest recover{ rtc() }; // works fine

I2C_Scan scan{ rtc_o };              // works fine
I2C_SpeedTest speed{ rtc_o };        // works fine
I2C_Recover_Retest recover{ rtc_o }; // works fine

Clock & clock_() {
	static Clock _clock;
	return _clock;
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

EEPROMClass & eeprom() {
	static EEPROMClass _eeprom_obj{ rtc(), 0x50 };
	return _eeprom_obj;
}

uint8_t resetRTC(I2C_Talk & i2c, int) {
	Serial.println("Power-Down RTC...");
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(5000);
	digitalWrite(RTC_RESET, LOW);
	i2c.restart();
	return I2C_Talk_ErrorCodes::_OK;
}

EEPROMClass & EEPROM = eeprom();

char poem[] = {
"Twas brillig, and the slithy toves \nDid gyre and gimble in the wabe;\nAll mimsy were the borogoves,\
\nAnd the mome raths outgrabe. \n\nBeware the Jabberwock, my son! \nThe jaws that bite, the claws that catch!\
\nBeware the Jubjub bird, and shun \nThe frumious Bandersnatch!\
\n\nHe took his vorpal sword in hand: \nLong time the manxome foe he sought -\nSo rested he by the Tumtum tree,\
\nand stood awhile in thought.\n\nAnd as in uffish thought he stood, \nThe Jabberwock, with eyes of flame,\
\nCame whiffling through the tulgey wood,\nAnd burbled as it came!\n" };

//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println("Serial Begun");
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	//rtc_test.setTimeoutFn(resetRTC);
	resetRTC(rtc(), 0x50);
	//rtc().restart();
	Serial.print("Wire addr   "); Serial.println((long)&Wire);
	Serial.print("Wire 1 addr "); Serial.println((long)&Wire1);

	scan.show_all();
	speed.showAll_fastest();
	recover.showAll_fastest();

	uint8_t status = rtc().status(0x50);
	Serial.print("EEPROM Status"); Serial.println(rtc().getStatusMsg(status));
	
	status = rtc().status(0x68);
	Serial.print("Clock Status"); Serial.println(rtc().getStatusMsg(status));
	// **** EP Read ****
	uint8_t dataBuffa[20] = { 0 };
	Serial.println("\nRead I2C_EEPROM :");
	//uint8_t status = EEPROM.readEP(0, sizeof(dataBuffa), dataBuffa);
	status = rtc().readEP(0x50, 0, sizeof(dataBuffa), dataBuffa);
	if (status) {
		Serial.print("EEPROM.readEP failed with"); Serial.println(rtc().getStatusMsg(status));
	}	
	
	for (auto d : dataBuffa) Serial.println(d, DEC);
	Serial.println("\nFinished Data readEP");

	Serial.println("Write Poem:");
	Serial.println(poem);
	
	//status = EEPROM.writeEP(30, sizeof(poem), poem);
	status = rtc().writeEP(0x50, 30, sizeof(poem), poem);
	if (status) {
		Serial.print("writeEP failed with"); Serial.println(rtc().getStatusMsg(status));
	}
	for (char & chr : poem) { chr = 0; }

	Serial.println("\nRead I2C_EEPROM :");
	//status = EEPROM.readEP(30, sizeof(poem), poem);
	status = rtc().readEP(0x50, 30, sizeof(poem), poem);
	if (status) {
		Serial.print("readEP failed with"); Serial.println(rtc().getStatusMsg(status));
		rtc().restart();	
	}

	Serial.println(poem);
	Serial.println("\nFinished read poem");
}

void loop() {
	for (char & chr : poem) { chr = 0; }

	Serial.println("\nRead I2C_EEPROM :");
	//status = EEPROM.readEP(30, sizeof(poem), poem);
	auto status = rtc().readEP(0x50, 30, sizeof(poem), poem);
	if (status) {
		Serial.print("readEP failed with"); Serial.println(rtc().getStatusMsg(status));
		rtc().restart();
	}

	Serial.println(poem);
	Serial.println();
}