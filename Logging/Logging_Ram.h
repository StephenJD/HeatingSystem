#pragma once
#include <Logging_SD.h>

namespace arduino_logger {
	/// <summary>
	/// Per msg: 100uS Due/ 700uS Mega
	/// Save 1KB to SD: 250mS Due / 150mS Mega
	/// </summary>
	class RAM_Logger : public Logger {
	public:
		RAM_Logger(const char* fileNameStem, uint16_t ramFile_size, bool keepSaving, Clock& clock);
		RAM_Logger(const char* fileNameStem, uint16_t ramFile_size, bool keepSaving);
		void begin(uint32_t = 0) override; // Clear Ram-array
		size_t write(uint8_t) override;
		size_t write(const uint8_t* buffer, size_t size) override;
		void flush() override; // append ram-array to dated file
		const char* c_str() { return (const char*)_ramFile; }
	private:
		//Logger& logTime() override;
		uint8_t* _ramFile = 0;
		FileNameGenerator _fileNameGenerator;
		uint16_t _ramFile_size;
		uint16_t _filePos = 0;
		bool _keepSaving;
	};

}