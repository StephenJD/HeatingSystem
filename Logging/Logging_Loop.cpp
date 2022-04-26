#include "Logging_Loop.h"

namespace arduino_logger {

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
		if (is_null()) return;
		close();
		SD.remove(_fileNameGenerator(0));
		*this << L_time << "Begin Loop-File" << L_endl;
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
				*this << L_cout << "Loop-File good" << L_endl;
				uint8_t line[100];
				size_t n;
				while ((n = _loopFile.read(line, sizeof(line))) > 0) {
					dataFile.write(line,n);
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