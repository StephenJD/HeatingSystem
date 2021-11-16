#include "Logging_SD.h"
#ifndef __AVR_ATmega328P__
#include <Date_Time.h>
#include <Clock.h>
#include <MemoryFree.h>

//#if defined(__SAM3X8E__)
#include <EEPROM.h>
//#endif

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif
using namespace GP_LIB;

namespace arduino_logger {

////////////////////////////////////
//            SD_Logger           //
////////////////////////////////////

FileNameGenerator::FileNameGenerator(const char* fileNameStem) {
	strncpy(_fileNameStem, fileNameStem, 5);
	_fileNameStem[4] = 0;
}

CStr_20 FileNameGenerator::operator()(Clock* clock) {
	CStr_20 fileName;
	strcpy(fileName.str(), _fileNameStem);
	if (clock) {
		_fileDayNo = clock->day();
		strcat(fileName.str(), intToString(clock->month(), 2));
		strcat(fileName.str(), intToString(_fileDayNo, 2));
	}
	strcat(fileName.str(), ".txt");
	return fileName;
}

bool FileNameGenerator::isNewDay(Clock* clock) {
	return clock ? _fileDayNo != clock->day() : false;
}

// On Mega, MISO is internally connected to digital pin 50, MOSI is 51, SCK is 52
// Due SPI header does not use digital pins.
// Chip select is usually connected to pin 53 and is active LOW.
constexpr int chipSelect = 53;

SD_Logger::SD_Logger(const char * fileNameStem, uint32_t baudRate, Flags initFlags) 
	: Serial_Logger(baudRate, initFlags)
	, _fileNameGenerator(fileNameStem) 
{
	// Avoid calling Serial_Logger during construction, in case clock is broken.
	SD.begin(chipSelect);
	Serial.print(F("\nSD_Logger Begun without Clock: "));
	Serial.print(_fileNameGenerator.stem());
	Serial.print(open() ? F(" OK") : F(" Failed"));
	Serial.println();
}	

SD_Logger::SD_Logger(const char * fileNameStem, uint32_t baudRate, Clock & clock, Flags initFlags) 
	: Serial_Logger(baudRate, clock, initFlags)
	, _fileNameGenerator(fileNameStem) 
{
	// Avoid calling Serial_Logger during construction, in case clock is broken.
	SD.begin(chipSelect);
	Serial.print(F("\nSD_Logger Begun with clock: "));
	Serial.print(_fileNameGenerator.stem());
	Serial.print(_fileNameGenerator.dayNo());
	Serial.print(" : ");
	Serial.print(clock.day());
	Serial.print(open() ? F(" OK") : F(" Failed"));
	Serial.println();
}

Print& SD_Logger::stream() {
	if (is_cout() || !open()) return Serial_Logger::stream();
	else return *this;
}

bool SD_Logger::open() {
	if (!is_null() && SD.sd_exists(chipSelect)) {
		if (_fileNameGenerator.isNewDay(_clock)) close();
		if (!_dataFile) {
			_dataFile = SD.open(_fileNameGenerator(_clock), FILE_WRITE); // appends to file. 16mS when OK. 550uS when failed. 
		}
		else {
			if (_dataFile.name()[0] != _fileNameGenerator.stem()[0]) close();
		}
	}
	else if (_dataFile) {
		close();
	}
	return _dataFile;
}

size_t SD_Logger::write(uint8_t chr) {
	_dataFile.print(char(chr));
	return 1;
}

size_t SD_Logger::write(const uint8_t * buffer, size_t size) {
	_dataFile.print((const char *)buffer);
	return size;
}	

Logger & SD_Logger::logTime() {
	auto streamPtr = &stream();
	while (mirror_stream(streamPtr)) {
		streamPtr->print(_fileNameGenerator.stem());
		streamPtr->print(" ");
	}
	Serial_Logger::logTime();
	return *this;
}
}
#endif