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

	CStr_20 generateFileName(const char * fileNameStem, const Clock * clock) {
		CStr_20 fileName;
		strcpy(fileName.str(), fileNameStem);
		if (clock) {
			strcat(fileName.str(), intToString(clock->month(), 2));
			strcat(fileName.str(), intToString(clock->day(), 2));
		} 
		strcat(fileName.str(), ".txt");
		return fileName;
	}

	Logger::Logger(Clock & clock) : _clock(&clock) {}

	Logger & Logger::toFixed(int decimal) {
		int whole = (decimal / 256);
		double fractional = (decimal % 256) / 256.;
		print(double(whole + fractional));
		return *this;
	}

	Logger & Logger::operator <<(Flags flag) {
		//enum Flags { L_default, L_dec, L_int, L_concat, L_endl, L_time, L_flush, L_cout = 8, L_hex = 16, L_fixed = 32, L_tabs = 64, L_allwaysFlush = 128 };

		switch (flag) {
		case L_flush:	flush(); 
			_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
			break;
		case L_endl: // fall through	
			if (_flags & L_allwaysFlush) { (*this) << F(" |F|\n"); flush(); }
			(*this) << F("\n"); // fall through
		case L_default:
			_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
			break;
		case L_time:	logTime(); break;
		case L_concat:	removeFlag(L_tabs); break;
		case L_dec:
		case L_int:		
		case L_hex:
		case L_fixed:
			removeFlag(L_dec);
			removeFlag(L_int);
			removeFlag(L_hex);
			removeFlag(L_fixed); // fall through
		default:
			addFlag(flag);
		}
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
		auto freeRam = freeMemory();
		Serial.print(F("\nSerial_Logger RAM: ")); Serial.println(freeRam); Serial.flush();
		if (freeRam < 10) {  while (true); }
		#ifdef DEBUG_TALK
		_flags = L_allwaysFlush;
		#endif
	}	
	
	Serial_Logger::Serial_Logger(uint32_t baudRate, Clock & clock) : Logger(clock) {
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


	Logger & Serial_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << F("No Time");
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

	SD_Logger::SD_Logger(const char * fileNameStem, uint32_t baudRate) : Serial_Logger(baudRate) {
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		SD.begin(chipSelect);
		_dataFile = SD.open(generateFileName(_fileNameStem, _clock), FILE_WRITE); // appends to file. 16mS when OK. 550uS when failed. 
		Serial.print(F("\nSD_Logger Begun without Clock: "));
		Serial.print(_fileNameStem);
		Serial.print(isWorking() ? F(" OK") : F(" Failed"));
		Serial.println();
	}	
	
	SD_Logger::SD_Logger(const char * fileNameStem, uint32_t baudRate, Clock & clock) : Serial_Logger(baudRate, clock) {
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		SD.begin(chipSelect);
		_dataFile = SD.open(generateFileName(_fileNameStem, _clock), FILE_WRITE); // appends to file. 16mS when OK. 550uS when failed. 
		Serial.print(F("\nSD_Logger Begun with clock: "));
		Serial.print(_fileNameStem);
		Serial.print(isWorking() ? F(" OK") : F(" Failed"));
		Serial.println();
	}

	bool SD_Logger::isWorking() { return SD.sd_exists(chipSelect); }

	File SD_Logger::openSD() {
		if (SD.sd_exists(chipSelect)) {
			if (!_dataFile) {
				//auto t0 = micros();
				_dataFile = SD.open(generateFileName(_fileNameStem, _clock), FILE_WRITE); // appends to file. 16mS when OK. 550uS when failed. 
				//auto d = micros() - t0;
				//Serial.print(generateFileName(_fileNameStem, _clock)); Serial.println(" File re - opened"); Serial.flush();
			}
			else {
				if (_dataFile.name()[0] != _fileNameStem[0]) _dataFile = File{};
				//Serial.println("File OK");
			}
		}
		else if (_dataFile) {
			//Serial.println("File closed on fail");
			_dataFile = File{};
		}
		return _dataFile;
	}

	size_t SD_Logger::write(uint8_t chr) {
		if (!is_cout() && openSD()) {
			_dataFile.print(char(chr));
		}
		return 1;
	}

	size_t SD_Logger::write(const uint8_t * buffer, size_t size) {
		if (!is_cout() && openSD()) {
			_dataFile.print((const char *)buffer);
		}
		return size;
	}	
	
	Logger & SD_Logger::logTime() {
		if(_clock->time().asInt() == 0 && _clock->minUnits() == 0 && _clock->seconds() == 0) _dataFile = File{};
		Serial_Logger serial(static_cast<Serial_Logger&>(*this));
		serial << _fileNameStem;
		Serial_Logger::logTime();
		_SD_mustTabTime = true;
		return *this;
	}

#ifdef ZPSIM
	using namespace std;
	////////////////////////////////////
	//            File_Logger           //
	////////////////////////////////////

	// On Mega, MISO is internally connected to digital pin 50, MOSI is 51, SCK is 52
	// Due SPI header does not use digital pins.
	// Chip select is usually connected to pin 53 and is active LOW.

	File_Logger::File_Logger(const char * fileNameStem, uint32_t baudRate) : Serial_Logger(baudRate) {
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		Serial.print(F("\nFile_Logger Begun without Clock: "));
		Serial.print(_fileNameStem);
		Serial.println();
	}	
	
	File_Logger::File_Logger(const char * fileNameStem, uint32_t baudRate, Clock & clock) : Serial_Logger(baudRate, clock) {
		strncpy(_fileNameStem, fileNameStem, 5);
		_fileNameStem[4] = 0;
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		Serial.print(F("\nFile_Logger Begun with clock: "));
		Serial.print(_fileNameStem);
		Serial.println();
	}

	bool File_Logger::isWorking() { return true; }

	bool File_Logger::openSD() {
		if (!_dataFile.good()) {
			_dataFile.open(generateFileName(_fileNameStem, _clock), ios::ate);	// Append
		}
		return _dataFile.good();
	}

	size_t File_Logger::write(uint8_t chr) {
		if (!is_cout() && openSD()) {
			_dataFile.write((char*)&chr, 1);
		}
		return 1;
	}

	size_t File_Logger::write(const uint8_t * buffer, size_t size) {
		if (!is_cout() && openSD()) {
			_dataFile.write((char*)buffer, size);
		}
		return size;
	}	
	
	Logger & File_Logger::logTime() {
		if(_clock->time().asInt() == 0 && _clock->minUnits() == 0 && _clock->seconds() == 0) _dataFile.close();
		Serial_Logger serial(static_cast<Serial_Logger&>(*this));
		serial << _fileNameStem;
		Serial_Logger::logTime();
		_SD_mustTabTime = true;
		return *this;
	}
#endif
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