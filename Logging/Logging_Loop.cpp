#include "Logging_Loop.h"
#ifndef __AVR__

#include <Watchdog_Timer.h>

namespace arduino_logger {

	Loop_Logger::Loop_Logger(const char * fileNameStem, uint32_t baudRate, Clock & clock, Flags initFlags)
		: Serial_Logger(baudRate, clock, initFlags)
		, _fileNameGenerator(fileNameStem)
	{
		SD.begin(chipSelect);
	}

	Loop_Logger::Loop_Logger(const char * fileNameStem, uint32_t baudRate)
		: Serial_Logger(baudRate, L_clearFlags)
		, _fileNameGenerator(fileNameStem)
	{
		SD.begin(chipSelect);
	}

	void Loop_Logger::begin(uint32_t) {
		if (is_null()) return;
		close();
		SD.remove(_fileNameGenerator(0));
		*this << L_time << "Begin Loop-File at " << micros() / 1000 << L_endl;
	}

	Print& Loop_Logger::stream() {
		if (is_cout() || !open()) return Serial_Logger::stream();
		else {
			return *this;
		}
	}

	bool Loop_Logger::open() {
		if (!is_null() && SD.sd_exists(chipSelect)) {
			if (!_loopFile) {
				_loopFile = SD.open(_fileNameGenerator(0), FILE_WRITE);
			}
			else {
				if (_loopFile.name()[0] != _fileNameGenerator.stem()[0]) close();
			}
		}
		else if (_loopFile) {
			close();
		}
		return _loopFile;
	}

	size_t Loop_Logger::write(uint8_t chr) {
		_loopFile.print(char(chr));
		return 1;
	}

	size_t Loop_Logger::write(const uint8_t *buffer, size_t size) {
		_loopFile.print((const char*)buffer);
		return size;
	}

	void Loop_Logger::flush() {
		if (SD.begin(chipSelect)) {
			*this << L_time << "Flush Loop-File" << L_endl;
			close();
			auto dataFile = SD.open(_fileNameGenerator(_clock), FILE_WRITE); // appends to file
			_loopFile = SD.open(_fileNameGenerator(0), FILE_READ);
			if (dataFile && _loopFile) {
				*this << L_cout << "Loop-File good" << L_flush;
				uint8_t line[1000];
				size_t n;
				while ((n = _loopFile.read(line, sizeof(line))) > 0) {
					if (dataFile.write(line, n) != n) {
						*this << L_cout << "dataFile bad-write" << L_flush;
						break;
					}
					dataFile.flush();
					Serial.write(line, 10); Serial.println("");
					reset_watchdog();
				}
				close();
				dataFile.close();
				begin();
			}
		}
		Serial_Logger::flush();
	}

	Logger & Loop_Logger::logTime() {
		auto streamPtr = &stream();
		while (mirror_stream(streamPtr)) {
			streamPtr->print(_fileNameGenerator.stem());
			streamPtr->print(" ");
		}
		return Logger::logTime();
	}

}
#endif