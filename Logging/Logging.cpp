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
	//Serial.print("<<flag:"); Serial.print(flag); Serial.print(" Flag:"); Serial.print(_flags); Serial.print(" FlshF:"); Serial.println(_flags & L_startWithFlushing); Serial.flush();
	switch (flag) {
	case L_time:	logTime(); break;
	case L_flush:
		_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
		*this << " |F|\n";
		flush();
		break;
	case L_endl:
	{
		if (_flags & L_allwaysFlush) { *this << " |F|"; } else if ((_flags & L_startWithFlushing) == L_startWithFlushing) { *this << " |SF|"; }
		auto streamPtr = &stream();
		do {
			streamPtr->print("\n");
		} while (mirror_stream(streamPtr));
		if (_flags & L_allwaysFlush || ((_flags & L_startWithFlushing) == L_startWithFlushing)) flush();
	}
	[[fallthrough]];
	case L_clearFlags:
	{
		bool isStartWithFlushing = (_flags & L_startWithFlushing) == L_startWithFlushing;
		_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
		if (isStartWithFlushing) _flags += L_startWithFlushing;
	}
		break;
	case L_allwaysFlush: _flags += L_allwaysFlush; break;
	case L_concat:	removeFlag(L_tabs); break;
	case L_dec:
	case L_int:
		removeFlag(L_fixed);
		removeFlag(L_hex);
		//removeFlag(L_dec);
		//removeFlag(L_int);
		//Serial.print("dec/int Flag:"); Serial.println(_flags); Serial.flush();
		break;
	case L_hex:
	case L_fixed:
		[[fallthrough]];
	default:
		addFlag(flag);
		//Serial.print("AddFlag:"); Serial.println(_flags); Serial.flush();
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
	Serial.print(F("\n\n\nSerial_Logger RAM: ")); Serial.println(freeRam); Serial.flush();
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

void Serial_Logger::begin(uint32_t baudRate) { 
	if (baudRate == 0) Serial.println(F("Baud Rate must be provided"));
	else {
		Serial.begin(baudRate);
		Serial.print(F("Baud Rate: ")); Serial.println(baudRate);
	}
}

size_t Serial_Logger::write(uint8_t chr) {
	Serial.print(char(chr));
	return 1;
}

size_t Serial_Logger::write(const uint8_t* buffer, size_t size) {
	Serial.print((char*)buffer);
	return size;
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