#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Talk_ErrorCodes.h>
#include <Wire.h>
#include <EEPROM.h>
#include <I2C_Scan.h>
#include <I2C_SpeedTest.h>
#include <I2C_RecoverRetest.h>
#include <Clock.h>
#include <Logging.h>
#include <SD.h>
#include <spi.h>
//#include <I2C_eeprom.h>

#define RTC_RESET 4
using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;

I2C_Talk rtc(Wire1,100000);

EEPROMClass & eeprom() {
	static EEPROMClass_T<rtc> _eeprom_obj{0x50};
	return _eeprom_obj;
}
EEPROMClass & EEPROM = eeprom();

//I2C_eeprom ee(0x50, 0x8000, Wire1);

int st_index;
I_I2C_Scan scan{rtc};

Clock & clock_() {
	static Clock _clock;
	return _clock;
}

Logger & logger() {
	static Serial_Logger _log(9600, clock_());
	return _log;
}

uint8_t resetRTC(I2C_Talk & i2c, int) {
	Serial.println("Power-Down RTC...");
	digitalWrite(RTC_RESET, HIGH);
	delayMicroseconds(5000);
	digitalWrite(RTC_RESET, LOW);
	i2c.restart();
	delayMicroseconds(200000);
	return I2C_Talk_ErrorCodes::_OK;
}

const char poem[] = {
"Twas brillig, and the slithy toves \nDid gyre and gimble in the wabe;\nAll mimsy were the borogoves,\
\nAnd the mome raths outgrabe. \n\nBeware the Jabberwock, my son! \nThe jaws that bite, the claws that catch!\
\nBeware the Jubjub bird, and shun \nThe frumious Bandersnatch!\
\n\nHe took his vorpal sword in hand: \nLong time the manxome foe he sought -\nSo rested he by the Tumtum tree,\
\nand stood awhile in thought.\n\nAnd as in uffish thought he stood, \nThe Jabberwock, with eyes of flame,\
\nCame whiffling through the tulgey wood,\nAnd burbled as it came!\n" };

char readPoem[sizeof(poem)];
const char blankLines[sizeof(poem)] = { '\0' };
//////////////////////////////// Start execution here ///////////////////////////////
void setup() {
	Serial.begin(9600);
	Serial.println("Serial Begun");
	pinMode(RTC_RESET, OUTPUT);
	digitalWrite(RTC_RESET, LOW); // reset pin
	//rtc_test.setTimeoutFn(resetRTC);
	resetRTC(rtc, 0x50);
	Serial.print("Wire addr   "); Serial.println((long)&Wire);
	Serial.print("Wire 1 addr "); Serial.println((long)&Wire1);

	//Serial.print("Scan Wire 1 addr "); Serial.println((long)&Wire1);

	scan.show_all();

	uint8_t status = rtc.status(0x50);
	Serial.print("EEPROM Status"); Serial.println(rtc.getStatusMsg(status));
	
	status = rtc.status(0x68);
	Serial.print("Clock Status"); Serial.println(rtc.getStatusMsg(status));
	// **** EP Read ****
	uint8_t dataBuffa[20] = { 0 };
	Serial.println("\nRead I2C_EEPROM :");
	status = EEPROM.readEP(0, sizeof(dataBuffa), dataBuffa);
	if (status) {
		Serial.print("EEPROM.readEP failed with"); Serial.println(rtc.getStatusMsg(status));
	}	
	
	for (auto d : dataBuffa) Serial.println(d, DEC);
	Serial.println("\nFinished Data readEP");
}

void loop() {
	auto status = rtc.status(0x50);
	if (status) {
		Serial.print("\nEEPROM.status failed with"); Serial.println(rtc.getStatusMsg(status));
	}
	//EEPROM.writeEP(30, sizeof(blankLines), (uint8_t *)blankLines);

	//Serial.println("\nTEST: writeByte\n");
	//uint32_t runTime = micros();
	//for (int i = 0; i < sizeof(poem); ++i) {
	//	status = ee.writeByte(30 + i, uint8_t(poem[i]));
	//	if (status != 0) break;
	//}
	//runTime = micros() - runTime;
	//Serial.print("\n\twriteByte took uS "); Serial.print(runTime); Serial.println(EEPROM.getStatusMsg(status)); Serial.flush();

	//Serial.println("\nTEST: readBlock\n");
	//for (char & chr : readPoem) { chr = 0; }
	//runTime = micros();	
	//int readCount = ee.readBlock(30, (uint8_t *)readPoem, sizeof(readPoem));
	//Serial.print("Chars written: "); Serial.println(sizeof(readPoem));
	//Serial.print("Chars read: "); Serial.println(readCount);

	//if (readCount == sizeof(readPoem)) status = _OK; else status = _Insufficient_data_returned;
	//runTime = micros() - runTime;
	//Serial.println(readPoem);
	//Serial.print("\n\treadBlock took uS "); Serial.print(runTime); Serial.println(EEPROM.getStatusMsg(status)); Serial.flush();

	Serial.println("\nTEST: Write whole poem");
	EEPROM.writeEP(30, sizeof(blankLines), (uint8_t *)blankLines);
	auto runTime = micros();
	status = EEPROM.writeEP(30, sizeof(poem), poem);
	runTime = micros() - runTime;

	Serial.print("\n\tWriting whole poem took mS "); Serial.print(runTime/1000); Serial.println(EEPROM.getStatusMsg(status)); Serial.flush();

	Serial.println("\nTEST: Read whole poem\n");
	for (char & chr : readPoem) { chr = 0; }
	runTime = micros();
	status = EEPROM.readEP(30, sizeof(readPoem), readPoem);
	runTime = micros() - runTime;

	Serial.println();
	Serial.println(readPoem);
	
	Serial.print("\n\tReading whole poem took mS "); Serial.print(runTime/1000); Serial.println(EEPROM.getStatusMsg(status)); Serial.flush();

	Serial.println("\nTEST: Write poem by char\n"); Serial.flush();
	EEPROM.writeEP(30, sizeof(blankLines), (uint8_t *)blankLines);
	runTime = micros();
	int i = 0; 
	for (; i < sizeof(poem); ++i) {
		status = EEPROM.write(30 + i, poem[i]);
		if (status != 0) break;
	}	
	runTime = micros() - runTime;
	Serial.print("\n\tWrote chars: "); Serial.print(i);
	Serial.print("\n\tWriting poem by char took mS "); Serial.print(runTime/1000); Serial.println(EEPROM.getStatusMsg(status)); Serial.flush();

	Serial.println("\nTEST: Read poem by char\n"); Serial.flush();
	for (char & chr : readPoem) { chr = 0; }
	runTime = micros();
	i = 0;
	for (; i < sizeof(readPoem); ++i) {
		status = EEPROM.readEP(30 + i, 1, &readPoem[i]);
		if (status != 0) break;
	}
	runTime = micros() - runTime;
	
	Serial.println();
	Serial.println(readPoem);
	Serial.print("\n\tReading poem by char took mS "); Serial.print(runTime / 1000); ; Serial.print(" Status: ");  Serial.print(status); Serial.println(EEPROM.getStatusMsg(status)); Serial.flush();
	//while (true);
}