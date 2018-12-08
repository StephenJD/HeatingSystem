#include "A__Constants.h"
#include <MultiCrystal.h>
#include <SPI.h>
#include <SD.h>
#include "A_Stream_Utilities.h"
#include "DateTime_Stream.h"
//#include "D_Factory.h"

template <int noDevices> class I2C_Helper_Auto_Speed;
extern File dataFile;
extern const char * LOG_FILE;
extern I2C_Helper_Auto_Speed<27> * i2C;
extern I2C_Helper * rtc;
extern SdVolume volume;
extern Sd2Card card;
extern int sdCardSelect;

//class FactoryObjects;
//extern FactoryObjects * f;

//MultiCrystal * mainLCD = new MultiCrystal(46,44,42, 40,38,36,34, 32,30,28,26);
//MultiCrystal * hallLCD = new MultiCrystal(
//		7,6,5,
//		4,3,2,1,
//		0,4,3,2,
//		i2C,DS_REMOTE_ADDRESS,
//		0xFF,0x1F);

bool temp_sense_hasError = false;
U1_byte lineNo = 0;
bool processReset = false;

void iniPrint(char * msg){
	if (lineNo >= MAX_NO_OF_ROWS) lineNo = 0;
	mainLCD->setCursor(0,lineNo++);
	U1_byte len = strlen(msg);
	mainLCD->print(msg);
	Serial.println(msg);
	for (U1_byte i = len; i<MAX_LINE_LENGTH;++i)
		mainLCD->print(" ");
};

void iniPrint(char * msg, U4_byte val){
	U1_byte len = strlen(msg);
	mainLCD->print(msg);
	Serial.print(msg);
	mainLCD->print(val);
	Serial.println(val);
};

using namespace Utils;
#include "DateTime_A.h"
char logTimeStr[18];
char decTempStr[7];



const char * logTime() {
	DTtype time = currentTime(true);
	if (time.getDay() == 0) {
		strcpy(logTimeStr, "No Time: ");
		strcat(logTimeStr, cIntToStr((int)rtc,5,'0'));
	} else {
		strcpy(logTimeStr, cIntToStr(time.getDay(),2,'0'));
		strcat(logTimeStr, "/");
		strcat(logTimeStr, cIntToStr(time.getMnth(),2,'0'));
		strcat(logTimeStr, "/");
		strcat(logTimeStr,cIntToStr(time.getYr(),2,'0'));
		strcat(logTimeStr, " ");
		strcat(logTimeStr, cIntToStr(time.getHrs(),2,'0'));
		strcat(logTimeStr, ":");
		strcat(logTimeStr, cIntToStr(time.get10Mins(),1,'0'));
		strcat(logTimeStr, cIntToStr(currTime().minUnits(),1,'0'));
	}
	return logTimeStr;
}

File openSD() {
	static bool SD_OK = false;
	static unsigned long nextTry = millis(); // every 10 secs.

	if (millis() >= nextTry) {
		if (SD.cardMissing()) {
			SD_OK = SD.begin();			
			if (SD_OK) {
				Serial.println("SD.Begin OK");
			} else {
				Serial.println("error opening SD file");
			}
		}
		nextTry = millis() + 10000;	
	}
	if (SD_OK) {
		dataFile = SD.open(LOG_FILE, FILE_WRITE);
	} 
	return dataFile;
}

void logToSD(){
	dataFile = openSD();
	if (dataFile) {
		dataFile.println();
		dataFile.close();
	}
	Serial.println();
}

void logToSD(const char * msg){
	dataFile = openSD();
	if (dataFile) {
		dataFile.print(logTime());
		dataFile.print("\t");
		dataFile.println(msg);
		dataFile.close();
	}
	Serial.print(logTime());
	Serial.print("\t");
	Serial.println(msg);
}

void logToSD(const char * msg, long val) {
	dataFile = openSD();
	if (dataFile) {
		dataFile.print(logTime());
		dataFile.print("\t");
		dataFile.print(msg);
		dataFile.print("\t");
		dataFile.println(val);
		dataFile.close();
	}
	Serial.print(logTime());
	Serial.print("\t");
	Serial.print(msg);
	Serial.print("\t");
	Serial.println(val);
}

void logToSD_notime(const char * msg, long val){
	dataFile = openSD();
	if (dataFile) {
		dataFile.print("\t");
		dataFile.print(msg);
		dataFile.print("\t");
		dataFile.println(val);
		dataFile.close();
	}
	Serial.print("\t");
	Serial.print(msg);
	Serial.print("\t");
	Serial.println(val);
}

void logToSD(const char * msg, long val, const char * name, long val2) {
	dataFile = openSD();
	if (dataFile) {
		dataFile.print(logTime());
		dataFile.print("\t");
		dataFile.print(msg);
		dataFile.print("\t");
		dataFile.print(val);
		dataFile.print("\t");
		dataFile.print(name);
		if (val2 != 0xFFFFABCD) {
			dataFile.print("\t");
			dataFile.println(val2);
		} else dataFile.println();
		dataFile.close();
	}
	Serial.print(logTime());
	Serial.print("\t");
	Serial.print(msg);
	Serial.print("\t");
	Serial.print(val);
	Serial.print("\t");
	Serial.print(name);
	if (val2 != 0xFFFFABCD) {
		Serial.print("\t");
		Serial.println(val2);
	} else Serial.println();
}

void logToSD(const char * msg, long index, long call, long autoCall, long offset, long outside, long setback, long req, int temp, bool isHeating) {
	dataFile = openSD();
	int deg = (temp/256) * 100;
	int fract = (temp % 256) / 16;
	fract = int(fract * 6.25); 
	if (dataFile) {
		dataFile.print(logTime());
		dataFile.print("\t");
		dataFile.print(msg);
		dataFile.print(index);
		dataFile.print("\t");
		dataFile.print(call);
		dataFile.print("\t");
		dataFile.print(autoCall);
		dataFile.print("\t");
		dataFile.print(offset);
		dataFile.print("\t");
		dataFile.print(outside);
		dataFile.print("\t");
		dataFile.print(setback);
		dataFile.print("\t");
		dataFile.print(req);
		dataFile.print("\t");
		dataFile.print(cDecToStr(deg+fract, 4, 2));
		dataFile.print("\t");
		dataFile.println(isHeating ? "!" : "");
		dataFile.close();
	}
	Serial.print(logTime());
	Serial.print("\t");
	Serial.print(msg);
	Serial.print(index);
	Serial.print("\t");
	Serial.print(call);
	Serial.print("\t");
	Serial.print(autoCall);
	Serial.print("\t");
	Serial.print(offset);
	Serial.print("\t");
	Serial.print(outside);
	Serial.print("\t");
	Serial.print(setback);
	Serial.print("\t");
	Serial.print(req);
	Serial.print("\t");
	Serial.print(cDecToStr(deg+fract, 4, 2));
	Serial.print("\t");
	Serial.println(isHeating ? "!" : "");
};

long actualLoopTime = 0;
long mixCheckTime = 0;
long zoneCheckTime = 0;
long storeCheckTime = 0;

