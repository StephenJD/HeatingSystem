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
	class Loop_Logger : public Serial_Logger {
	public:
		Loop_Logger(const char* fileNameStem, uint32_t baudRate, Clock& clock);
		Loop_Logger(const char* fileNameStem, uint32_t baudRate);
		void flush() override; // append loop-file to dated file
		void begin(uint32_t = 0) override; // delete existing loop-file
		Print& stream() override;
		bool mirror_stream(Logger::ostreamPtr& mirrorStream) override { return Serial_Logger::mirror_stream(mirrorStream); }

	private:
		Logger& logTime() override;
		void endl(Print& stream) override {
			stream.print(F("\n")); _loopFile.flush();
		}

		bool open();
		void close() { _loopFile.close(); _loopFile = File{}; }
		size_t write(uint8_t) override;
		size_t write(const uint8_t* buffer, size_t size) override;
		FileNameGenerator _fileNameGenerator;
		File _loopFile;
	};
}