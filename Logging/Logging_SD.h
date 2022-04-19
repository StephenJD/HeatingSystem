#pragma once
#ifndef __AVR_ATmega328P__
#include <Arduino.h>
#include <Logging.h>
#include <Conversions.h>
#include <SD.h>
#include <SPI.h>

#ifdef ZPSIM
#include <fstream>
#endif
/*
To reduce timeout on missing SD card from 2s to 7mS modify Sd2Card.cpp:
In Sd2Card::init() on first timeout:  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {...
Set timeout to 1mS.
SD.h/.cpp modified to provide sd_exists();
*/

namespace arduino_logger {
	// Chip select is usually connected to pin 53 and is active LOW.
	constexpr int chipSelect = 53;

	class FileNameGenerator {
	public:
		FileNameGenerator(const char* fileNameStem);
		GP_LIB::CStr_20 operator()(Clock * clock);
		const char* stem() { return _fileNameStem; }
		bool isNewDay(Clock* clock);
		int dayNo() { return _fileDayNo; }
	private:
		char _fileNameStem[5];
		unsigned char _fileDayNo = 0;
	};

	/// <summary>
	/// Per un-timed msg: 1.4mS
	/// Per timed msg: 3mS
	/// </summary>	
	class SD_Logger : public Serial_Logger {
	public:
		SD_Logger(const char* fileNameStem, uint32_t baudRate, Clock& clock, Flags initFlags = L_flush);
		SD_Logger(const char* fileNameStem, uint32_t baudRate, Flags initFlags = L_flush);
		Print& stream() override;
		void flush() override { close(); Serial_Logger::flush(); }
		bool mirror_stream(Logger::ostreamPtr& mirrorStream) override { return Serial_Logger::mirror_stream(mirrorStream); }
		
		bool open() override;
		void close() { _dataFile.close(); _dataFile = File{}; }
	private:
		Logger& logTime() override;
		size_t write(uint8_t) override;
		size_t write(const uint8_t* buffer, size_t size) override;
		FileNameGenerator _fileNameGenerator;
		File _dataFile;
	};

#ifdef ZPSIM
	template<typename MirrorBase = Serial_Logger>
	class File_Logger : public MirrorBase {
	public:
		static constexpr int FILE_NAME_LENGTH = 8;
		File_Logger(const char* fileNameStem, uint32_t baudRate, Clock& clock, Flags initFlags = L_flush);
		File_Logger(const char* fileNameStem, uint32_t baudRate, Flags initFlags = L_flush);
		Print& stream() override;
		void flush() override { close(); MirrorBase::flush(); }
		bool mirror_stream(Logger::ostreamPtr& mirrorStream) override { return MirrorBase::mirror_stream(mirrorStream); }

		bool open() override;
		void close() { _dataFile.close(); }
	protected:
		Logger& logTime() override;
		
		size_t write(uint8_t) override;
		size_t write(const uint8_t* buffer, size_t size) override;

		Logger* _mirror = this;
		std::ofstream _dataFile;
		FileNameGenerator _fileNameGenerator;
	};

	////////////////////////////////////
	//            File_Logger         //
	////////////////////////////////////

	template<typename MirrorBase>
	File_Logger<MirrorBase>::File_Logger(const char* fileNameStem, uint32_t baudRate, Flags initFlags)
		: MirrorBase(baudRate, initFlags), _fileNameGenerator(fileNameStem) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		Serial.print(F("\nFile_Logger Begun without Clock: "));
		Serial.print(_fileNameGenerator.stem());
		Serial.println();
	}

	template<typename MirrorBase>
	File_Logger<MirrorBase>::File_Logger(const char* fileNameStem, uint32_t baudRate, Clock& clock, Flags initFlags)
		: MirrorBase(baudRate, clock, initFlags), _fileNameGenerator(fileNameStem) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		Serial.print(F("\nFile_Logger Begun with clock: "));
		Serial.print(_fileNameGenerator.stem());
		Serial.println();
	}

	template<typename MirrorBase>
	Print& File_Logger<MirrorBase>::stream() {
		if (File_Logger<MirrorBase>::is_cout() || !open()) return MirrorBase::stream();
		else return *this;
	}

	template<typename MirrorBase>
	bool File_Logger<MirrorBase>::open() {
		if (_fileNameGenerator.isNewDay(File_Logger<MirrorBase>::_clock)) _dataFile.close();
		if (!is_null() && !_dataFile.is_open()) {
			_dataFile.open(_fileNameGenerator(File_Logger<MirrorBase>::_clock), std::ios::app);	// Append
		}
		return _dataFile.good();
	}

	template<typename MirrorBase>
	size_t File_Logger<MirrorBase>::write(uint8_t chr) {
		_dataFile.write((char*)&chr, 1);
		return 1;
	}

	template<typename MirrorBase>
	size_t File_Logger<MirrorBase>::write(const uint8_t* buffer, size_t size) {
		_dataFile.write((char*)buffer, size);
		return size;
	}

	template<typename MirrorBase>
	Logger& File_Logger<MirrorBase>::logTime() {
		auto streamPtr = &stream();
		while (mirror_stream(streamPtr)) {
			streamPtr->print(_fileNameGenerator.stem());
			streamPtr->print(" ");
		}
		MirrorBase::logTime();
		return *this;
	}
#endif
}
#endif
