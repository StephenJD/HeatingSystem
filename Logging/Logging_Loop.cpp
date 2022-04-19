#include "Logging_Loop.h"
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
//            Loop_Logger       //
////////////////////////////////////

	Loop_Logger::Loop_Logger(const char * fileNameStem, uint32_t baudRate, Clock & clock)
		: Serial_Logger(baudRate, clock, L_clearFlags)
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
		SD.remove(_fileNameGenerator(0));
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
				while (_loopFile.available()) {
					dataFile.write(_loopFile.read());
				}
				close();
				dataFile.close();
				SD.remove(_fileNameGenerator(0));
			}
		}
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