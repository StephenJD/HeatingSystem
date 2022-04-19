#pragma once
#include <Logging_SD.h>
#include <Arduino.h>
#include <Type_Traits.h>

#ifdef ZPSIM
#include <fstream>
#endif
/*
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
*/
class EEPROMClassRE;
EEPROMClassRE & eeprom();
class Clock;

namespace arduino_logger {

	/// <summary>
	/// Per msg: 115mS Due/ 170mS Mega
	/// Save 1KB to SD: 660mS Due / 230mS Mega
	/// </summary>
	class EEPROM_Logger : public Logger {
	public:
		EEPROM_Logger(const char* fileNameStem, uint16_t startAddr, uint16_t endAddr, bool keepSaving, Clock& clock);
		EEPROM_Logger(const char* fileNameStem, uint16_t startAddr, uint16_t endAddr, bool keepSaving);
		void flush() override;
		void begin(uint32_t = 0) override;
	private:
		Logger& logTime() override;
		size_t write(uint8_t) override;
		size_t write(const uint8_t* buffer, size_t size) override;
		void saveOnFirstCall();
		void findStart();
		uint16_t _startAddr;
		uint16_t _endAddr;
		uint16_t _currentAddress = 0;
		bool _hasWrittenToSD = false;
		bool _hasRecycled = false;
		bool _keepSaving;
		FileNameGenerator _fileNameGenerator;
	};

	Logger& logger(); // to be defined by the client
}