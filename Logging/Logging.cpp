#include "Logging.h"
#include <Date_Time.h>
#include <Clock.h>
#include <Conversions.h>
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

Logger::Logger(Clock& clock, Flags initFlag) : _flags{ initFlag }, _clock(&clock) {}

Logger& Logger::toFixed(int decimal) {
	int whole = (decimal / 256);
	double fractional = (decimal % 256) / 256.;
	print(double(whole + fractional));
	return *this;
}

Logger& Logger::operator <<(Flags flag) {
	if (is_null()) return *this;
	switch (flag) {
	case L_time:	logTime(); break;
	case L_flush:
		_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
		*this << " |F|\n";
		flush();
		break;
	case L_endl:
	{
		if (_flags & L_allwaysFlush) { *this << " |F|"; } else if (_flags == L_startWithFlushing) { *this << " |SF|"; }
		auto streamPtr = &stream();
		do {
			streamPtr->print("\n");
		} while (mirror_stream(streamPtr));
		if (_flags & L_allwaysFlush || _flags == L_startWithFlushing) flush();
	}
	//[[fallthrough]];
	case L_clearFlags:
		if (_flags != L_startWithFlushing) {
			_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
		}
		break;
	case L_allwaysFlush: _flags += L_allwaysFlush; break;
	case L_concat:	removeFlag(L_tabs); break;
	case L_dec:
	case L_int:
	case L_hex:
	case L_fixed:
		removeFlag(L_dec);
		removeFlag(L_int);
		removeFlag(L_hex);
		removeFlag(L_fixed);
		//[[fallthrough]];
	default:
		addFlag(flag);
	}
	return *this;
}

Logger& Logger::logTime() {
	if (_clock) {
		_clock->refresh();
		if (_clock->day() == 0) {
			*this << F("No Time");
		} else {
			*this << *_clock;
		}
		_flags += L_time;
	}
	return *this;
}

////////////////////////////////////
//            Serial_Logger       //
////////////////////////////////////
char logTimeStr[18];
char decTempStr[7];

Serial_Logger::Serial_Logger(uint32_t baudRate, Flags initFlags) : Logger(initFlags) {
	Serial.flush();
	Serial.begin(baudRate);
	auto freeRam = freeMemory();
	Serial.print(F("\nSerial_Logger RAM: ")); Serial.println(freeRam); Serial.flush();
	if (freeRam < 10) { while (true); }
#ifdef DEBUG_TALK
	_flags = L_allwaysFlush;
#endif
}

Serial_Logger::Serial_Logger(uint32_t baudRate, Clock& clock, Flags initFlags) : Logger(clock, initFlags) {
	Serial.flush();
	Serial.begin(baudRate);
	auto freeRam = freeMemory();
	Serial.print(F("\nSerial_Logger RAM: ")); Serial.println(freeRam); Serial.flush();
	if (freeRam < 10) { while (true); }
#ifdef DEBUG_TALK
	_flags = L_allwaysFlush;
#endif
}

void Serial_Logger::begin(uint32_t baudRate) { Serial.begin(baudRate); }

size_t Serial_Logger::write(uint8_t chr) {
	Serial.print(char(chr));
	return 1;
}

size_t Serial_Logger::write(const uint8_t* buffer, size_t size) {
	Serial.print((char*)buffer);
	return size;
}

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
	if (SD.sd_exists(chipSelect)) {
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

/*
////////////////////////////////////
//            RAM_Logger       //
////////////////////////////////////

	RAM_Logger::RAM_Logger(const char * fileNameStem, uint16_t ramFile_size, bool keepSaving, Clock & clock)
		: Logger(clock)
		, _ramFile(new uint8_t[ramFile_size])
		, _ramFile_size(ramFile_size)
		, _keepSaving(keepSaving)
	{
		*_ramFile = 0u;
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
	}

	RAM_Logger::RAM_Logger(const char * fileNameStem, uint16_t ramFile_size, bool keepSaving)
		: Logger()
		, _ramFile(new uint8_t[ramFile_size])
		, _ramFile_size(ramFile_size)
		, _keepSaving(keepSaving)
	{
		*_ramFile = 0u;
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
	}

	size_t RAM_Logger::write(uint8_t chr) {
		if (_filePos >= _ramFile_size) {
			if (_keepSaving) readAll();
			_filePos = 0;
		}

		_ramFile[_filePos+1] = 0u;
		_ramFile[_filePos] = chr;
		++_filePos;
		return 1;
	}

	size_t RAM_Logger::write(const uint8_t * buffer, size_t size) {
		if (_filePos + size >= _ramFile_size) {
			while (++_filePos < _ramFile_size) {
				_ramFile[_filePos] = 0u;
			}
			if (_keepSaving) readAll();
			_filePos = 0;
		}

		auto endPos = buffer + size;
		_ramFile[_filePos + size] = 0u;
		for (; buffer < endPos; ++_filePos, ++buffer) {
			_ramFile[_filePos] = *buffer;
		}
		return size;
	}

	void RAM_Logger::readAll() {
		auto start = millis();
		if (SD.begin(chipSelect)) {
			auto _dataFile = SD.open(generateFileName(_fileNameStem, _clock), FILE_WRITE); // appends to file
			// Write from _filePos to end
			Serial.print((const char *)(_ramFile + _filePos + 1));
			_dataFile.print((const char *)(_ramFile + _filePos + 1));
			//Serial.println("\n * from start * ");
			// Write from 0 to _filePos
			Serial.print((const char *)(_ramFile));
			_dataFile.print((const char *)(_ramFile));

			Serial.println("\nClose");
			_dataFile.close();
		}
		Serial.print("Ram Save took mS "); Serial.println(millis() - start);
	}

	Logger & RAM_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << F("No Time");
			}
			else {
				*this << *_clock;
			}
			_ram_mustTabTime = true;
		}
		return *this;
	}

////////////////////////////////////
//            EEPROM_Logger       //
////////////////////////////////////

	EEPROM_Logger::EEPROM_Logger(const char * fileNameStem, uint16_t startAddr, uint16_t endAddr, bool keepSaving, Clock & clock)
		: Logger(clock)
		, _startAddr(startAddr)
		, _endAddr(endAddr)
		, _currentAddress(startAddr)
		, _keepSaving(keepSaving)
	{
		findStart();
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
		//saveOnFirstCall();
	}

	EEPROM_Logger::EEPROM_Logger(const char * fileNameStem, uint16_t startAddr, uint16_t endAddr, bool keepSaving)
		: Logger()
		, _startAddr(startAddr)
		, _endAddr(endAddr)
		, _currentAddress(startAddr)
		, _keepSaving(keepSaving)
	{
		findStart();
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
		//saveOnFirstCall();
	}

	void EEPROM_Logger::begin(uint32_t) { saveOnFirstCall(); }

	void EEPROM_Logger::saveOnFirstCall() {
		if (!_hasWrittenToSD) {
			Serial.println(F("* Initial Save EEPROM *"));
			findStart();
			*this << F("* End of Log before Restart *\n\n");
			readAll();
			_currentAddress = _startAddr;
			eeprom().write(_currentAddress, 0u);
			_hasWrittenToSD = true;
		}
	}

	void EEPROM_Logger::findStart() {
		_currentAddress = _startAddr;
		auto thisChar = eeprom().read(_currentAddress);
		while (thisChar != 0u && _currentAddress < _endAddr) {
			thisChar = eeprom().read(++_currentAddress);
		}
	}

	size_t EEPROM_Logger::write(uint8_t chr) {
		//saveOnFirstCall();
		if (_currentAddress >= _endAddr) {
			if (_keepSaving) readAll();
			_currentAddress = _startAddr;
			_hasRecycled = true;
		}

		eeprom().write(_currentAddress+1, 0u);
		eeprom().write(_currentAddress, chr);
		++_currentAddress;
		return 1;
	}

	size_t EEPROM_Logger::write(const uint8_t *buffer, size_t size) {
		//saveOnFirstCall();
		if (_currentAddress + size >= _endAddr) {
			while (++_currentAddress < _endAddr) {
				eeprom().write(_currentAddress, 0u);
			}
			if (_keepSaving) readAll();
			_currentAddress = _startAddr;
			_hasRecycled = true;
		}

		auto endPos = buffer + size;
		eeprom().write(_currentAddress + size, 0u);
		for (; buffer < endPos; ++_currentAddress, ++buffer) {
			eeprom().write(_currentAddress, *buffer);
		}
		return size;
	}

	void EEPROM_Logger::readAll() {
		auto start = millis();
		if (SD.begin(chipSelect)) {
			auto _dataFile = SD.open(generateFileName(_fileNameStem,_clock), FILE_WRITE); // appends to file
			// Write from _currentAddress to end
			//Serial.print(F("\n * from last end addr: ")); Serial.println(_currentAddress + 1); Serial.print(F(" to end addr: ")); Serial.println(_endAddr);
			//_dataFile.print(F("\n * from curr addr: ")); _dataFile.print(_currentAddress + 1); _dataFile.print(F(" to end addr: ")); _dataFile.print(_endAddr); _dataFile.print("\n");
			if (_hasRecycled) {
				for (auto addr = _currentAddress + 1; addr < _endAddr; ++addr) {
					auto thisChar = char(eeprom().read(addr));
					if (thisChar == '\0' || thisChar > char(127)) break;
					Serial.print(thisChar);
					_dataFile.print(thisChar);
				}
			}
			//_dataFile.print(F("\n end of prev log at addr: ")); _dataFile.print(addr);
			//Serial.print(F("\n * from start addr: ")); Serial.println(_startAddr);
			//_dataFile.print(F("\n * from start addr: ")); _dataFile.print(_startAddr); _dataFile.print(F(" to curr addr: ")); _dataFile.print(_currentAddress); _dataFile.print("\n");
			// Write from 0 to _currentAddress
			for (auto addr = _startAddr; addr < _currentAddress; ++addr) {
				auto thisChar = char(eeprom().read(addr));
				Serial.print(thisChar);
				_dataFile.print(thisChar);
			}
			//Serial.println(F("\nClose"));
			//_dataFile.print(F("\nClose"));
			_dataFile.close();
		}
		Serial.print("EEPROM Save took mS "); Serial.println(millis() - start);
	}

	Logger & EEPROM_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << F("No Time");
			}
			else {
				*this << *_clock;
			}
			_ee_mustTabTime = true;
		}
		return *this;
	}
	*/
}