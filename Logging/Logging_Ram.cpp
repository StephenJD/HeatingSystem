#include "Logging_Ram.h"

namespace arduino_logger {

	RAM_Logger::RAM_Logger(const char* fileNameStem, uint16_t ramFile_size, bool keepSaving, Clock& clock)
		: Logger(clock, L_clearFlags)
		, _ramFile(new uint8_t[ramFile_size])
		, _ramFile_size(ramFile_size)
		, _keepSaving(keepSaving)
		, _fileNameGenerator(fileNameStem)
	{
		// after a reset, the ram is not cleared, so we can recover what was in the ram-file.
		_filePos = strlen((const char*)_ramFile);
		if (_filePos >= ramFile_size) {
			_filePos = 0;
			*_ramFile = 0u;
		}
		logger() << "In Ram: "  << L_cout << c_str() << "End Ram" << L_endl;
	}

	RAM_Logger::RAM_Logger(const char* fileNameStem, uint16_t ramFile_size, bool keepSaving)
		: Logger(L_clearFlags)
		, _ramFile(new uint8_t[ramFile_size])
		, _ramFile_size(ramFile_size)
		, _keepSaving(keepSaving)
		, _fileNameGenerator(fileNameStem)
	{
		_filePos = strlen((const char*)_ramFile);
		if (_filePos >= ramFile_size) {
			_filePos = 0;
			*_ramFile = 0u;
		}
	}

	void RAM_Logger::begin(uint32_t) {
		if (is_null()) return;
		*_ramFile = 0u;
		_filePos = 0;
		*this << L_time << "Begin Ram-File" << L_endl;
	}

	size_t RAM_Logger::write(uint8_t chr) {
		if (_filePos >= _ramFile_size) {
			if (_keepSaving) flush();
			_filePos = 0;
		}

		_ramFile[_filePos + 1] = 0u;
		_ramFile[_filePos] = chr;
		++_filePos;
		return 1;
	}

	size_t RAM_Logger::write(const uint8_t* buffer, size_t size) {
		if (_filePos + size >= _ramFile_size) {
			while (++_filePos < _ramFile_size) {
				_ramFile[_filePos] = 0u;
			}
			if (_keepSaving) flush();
			_filePos = 0;
		}

		auto endPos = buffer + size;
		_ramFile[_filePos + size] = 0u;
		for (; buffer < endPos; ++_filePos, ++buffer) {
			_ramFile[_filePos] = *buffer;
		}
		return size;
	}

	void RAM_Logger::flush() {
		if (SD.begin(chipSelect)) {
			auto dataFile = SD.open(_fileNameGenerator(_clock), FILE_WRITE); // appends to file
			if (dataFile) {
				*this << L_time << "Flush Loop-File: " << _filePos << L_endl;
				dataFile.write(_ramFile, _filePos);
				dataFile.close();
				begin();
			}
		}
	}

}