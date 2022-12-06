#include "Logging_EP.h"
#ifndef __AVR_ATmega328P__

#include "Logging_SD.h"
#include <Date_Time.h>
#include <Clock.h>
#include <Conversions.h>
#include <MemoryFree.h>

//#if defined(__SAM3X8E__)
#include <EEPROM_RE.h>
//#endif

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif
using namespace GP_LIB;

namespace arduino_logger {

////////////////////////////////////
//            EEPROM_Logger       //
////////////////////////////////////

	EEPROM_Logger::EEPROM_Logger(const char * fileNameStem, uint16_t startAddr, uint16_t endAddr, bool keepSaving, Clock & clock)
		: Logger(clock, L_clearFlags)
		, _startAddr(startAddr)
		, _endAddr(endAddr)
		, _currentAddress(startAddr)
		, _keepSaving(keepSaving)
		, _fileNameGenerator(fileNameStem)
	{
		findStart();
		//saveOnFirstCall();
	}

	EEPROM_Logger::EEPROM_Logger(const char * fileNameStem, uint16_t startAddr, uint16_t endAddr, bool keepSaving)
		: Logger(L_clearFlags)
		, _startAddr(startAddr)
		, _endAddr(endAddr)
		, _currentAddress(startAddr)
		, _keepSaving(keepSaving)
		, _fileNameGenerator(fileNameStem)
	{
		findStart();
		//saveOnFirstCall();
	}

	void EEPROM_Logger::begin(uint32_t) { saveOnFirstCall(); }

	void EEPROM_Logger::saveOnFirstCall() {
		if (!_hasWrittenToSD) {
			Serial.println(F("* Initial Save EEPROM *"));
			findStart();
			*this << F("* End of Log before Restart *\n\n");
			flush();
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
			if (_keepSaving) flush();
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
			if (_keepSaving) flush();
			_currentAddress = _startAddr;
			_hasRecycled = true;
		}

		auto endPos = buffer + size;
		eeprom().write(int(_currentAddress + size), 0u);
		for (; buffer < endPos; ++_currentAddress, ++buffer) {
			eeprom().write(_currentAddress, *buffer);
		}
		return size;
	}

	void EEPROM_Logger::flush() {
		auto start = millis();
		if (SD.begin(chipSelect)) {
			auto _dataFile = SD.open(_fileNameGenerator(_clock), FILE_WRITE); // appends to file
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
		auto streamPtr = &stream();
		while (mirror_stream(streamPtr)) {
			streamPtr->print(_fileNameGenerator.stem());
			streamPtr->print(" ");
		}
		return Logger::logTime();

		//if (_clock) {
		//	_clock->refresh();
		//	if (_clock->day() == 0) {
		//		*this << F("No Time");
		//	}
		//	else {
		//		*this << *_clock;
		//	}
		//	_ee_mustTabTime = true;
		//}
		//return *this;
	}

}
#endif