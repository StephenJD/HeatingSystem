#include "Logging.h"
#include <Date_Time.h>
#include <Clock.h>
#include <Conversions.h>

//#if defined(__SAM3X8E__)
#include <EEPROM.h>
//#endif

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif
	using namespace GP_LIB;

	Logger::Logger(Clock & clock) : _clock(&clock) {}

	Logger & Logger::toFixed(int decimal) {
		int whole = (decimal / 256);
		double fractional = (decimal % 256) / 256.;
		print(double(whole + fractional));
		return *this;
	}

	Logger & Logger::operator <<(Flags flag) {
		//Flags {L_default, L_dec, L_int, L_concat, L_endl, L_time, L_cout = 8, L_hex = 16, L_fixed = 32, L_tabs = 64};
		switch (flag) {
		case L_flush:	flush(); // fall through
		case L_endl:	(*this) << "\n"; // fall through
		case L_default:
			_flags = L_default;
			break;
		case L_time:	logTime(); break;
		case L_dec:		removeFlag(L_hex); break;
		case L_int:		removeFlag(L_fixed); break;
		case L_concat:	removeFlag(L_tabs); break;
		default:
			addFlag(flag);
		}
		return *this;
	}
	 
	////////////////////////////////////
	//            EEPROM_Logger       //
	////////////////////////////////////

	size_t EEPROM_Logger::write(uint8_t chr) {
		if (_currentAddress >= _endAddr) _currentAddress = _startAddr; // return 0;
		EEPROM.write(_currentAddress, chr);
		++_currentAddress;
		return 1;
	}

	size_t EEPROM_Logger::write(const uint8_t *buffer, size_t size) {
		if (_currentAddress >= _endAddr) _currentAddress = _startAddr; // return 0;
		auto endPos = buffer + size;
		for (; buffer < endPos; ++_currentAddress, ++buffer) {
			EEPROM.write(_currentAddress, *buffer);
	    }
		EEPROM.write(_currentAddress, '\0');
		return size;
	}

	void EEPROM_Logger::readAll() {
		for (auto addr = _startAddr; addr < _endAddr; ++addr) {
			auto thisChar = char(EEPROM.read(addr));
			Serial.print(thisChar);
			if (thisChar > char(127)) break;
		}
	}

	Logger & EEPROM_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << "No Time";
			}
			else {
				*this << *_clock;
			}
		}
		_ee_mustTabTime = true;
		return *this;
	}


	////////////////////////////////////
	//            Serial_Logger       //
	////////////////////////////////////
	char logTimeStr[18];
	char decTempStr[7];

	Serial_Logger::Serial_Logger(uint32_t baudRate) : Logger() {
		Serial.flush();
		Serial.begin(baudRate);
		Serial.println(" Serial_Logger Begun");
	}	
	
	Serial_Logger::Serial_Logger(uint32_t baudRate, Clock & clock) : Logger(clock) {
		Serial.flush();
		Serial.begin(baudRate);
		Serial.println(" Serial_Logger Begun");
	}

	Logger & Serial_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << "No Time";
			}
			else {
				*this << *_clock;
			}
		}
		_mustTabTime = true;
		return *this;
	}

	size_t Serial_Logger::write(uint8_t chr) {
		Serial.print(char(chr));
		return 1;
	}	
	
	size_t Serial_Logger::write(const uint8_t *buffer, size_t size) {
		Serial.print((char*)buffer);
		return size;
	}	
	
	////////////////////////////////////
	//            SD_Logger           //
	////////////////////////////////////

	// On Mega, MISO is internally connected to digital pin 50, MOSI is 51, SCK is 52
	// Due SPI header does not use digital pins.
	// Chip select is usually connected to pin 53 and is active LOW.
	constexpr int chipSelect = 53;

	Serial_Logger SD_Logger::_serial;

	SD_Logger::SD_Logger(const char * fileName, uint32_t baudRate) : Serial_Logger(baudRate), _fileName(fileName) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		SD.begin(chipSelect);
		Serial_Logger::print("\nSD_Logger Begun\n");
	}	
	
	SD_Logger::SD_Logger(const char * fileName, uint32_t baudRate, Clock & clock) : Serial_Logger(baudRate, clock), _fileName(fileName) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		SD.begin(chipSelect);
		Serial_Logger::print("\nSD_Logger Begun\n");
	}

	bool SD_Logger::isWorking() { return SD.sd_exists(chipSelect); }

	File SD_Logger::openSD() {
		if (SD.sd_exists(chipSelect)) {
			_dataFile = SD.open(_fileName, FILE_WRITE); // appends to file
		} 
		return _dataFile;
	}

	size_t SD_Logger::write(uint8_t chr) {
		if (!is_cout() && openSD()) {
			_dataFile.print(char(chr));
			_dataFile.close();
		}
		return 1;
	}		
	
	size_t SD_Logger::write(const uint8_t * buffer, size_t size) {
		if (!is_cout() && openSD()) {
			_dataFile.print((const char *)buffer);
			_dataFile.close();
		}
		return size;
	}	
	
	Logger & SD_Logger::logTime() {
		Serial_Logger::logTime();
		_SD_mustTabTime = true;
		return *this;
	}

