#pragma once
#include <SD.h>
#include <SPI.h>
#include <Arduino.h>
#include <Type_Traits.h>
//#include <Streaming.h>
//#pragma message( "Logging.h loaded" )
/*
To reduce timeout on missing SD card from 2s to 7mS modify Sd2Card.cpp:
In Sd2Card::init() on first timeout:  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {...
Set timeout to 1mS.
SD.h/.cpp modified to provide sd_exists();
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
*/
	class EEPROMClass;
	EEPROMClass & eeprom();
	class Clock;

	enum Flags {L_default, L_dec, L_int, L_concat, L_endl, L_time, L_flush, L_cout = 8, L_hex = 16, L_fixed = 32, L_tabs = 64, L_allwaysFlush = 128 };
	inline Flags operator +(Flags l_flag, Flags r_flag) {return static_cast<Flags>(l_flag | r_flag);}

	class Logger : public Print {
	public:
		Logger() = default;
		Logger(const char * fileName, uint32_t baudRate, Clock & clock) {}

		Flags addFlag(Flags flag) { _flags = _flags + flag; return _flags; }
		Flags removeFlag(Flags flag) { _flags = static_cast<Flags>(_flags & ~flag); return _flags; }
		Logger(Clock & clock);
		size_t write(uint8_t) override { return 1; }
		size_t write(const uint8_t *buffer, size_t size) override { return size; }

		virtual bool isWorking() { return true; }
		virtual void readAll() {}
		virtual void flush() {}
		virtual void begin(uint32_t baudRate = 0) {}
		Logger & operator <<(Flags);
		
		template<class T>
		Logger & streamToPrintInt(T value) {
			if (is_fixed()) toFixed(value);
			else print(value, base());
			return *this;
		}
	protected:
		template<class T> friend Logger & operator <<(Logger & stream, T value);
		virtual bool is_tabs() { return false; }
		virtual void setBaseTimeTab(bool timeTab) {}
		virtual Logger & prePrint() {return *this;}
		virtual Logger & logTime() { return *this; }
		bool is_cout() { return _flags & L_cout; }
		bool is_fixed() { return _flags & L_fixed; }
		int base() {return _flags & L_hex ? HEX : DEC;}
		Logger & toFixed(int decimal);
		Clock * _clock = 0;
		Flags _flags = L_default;
	};

	// Streaming templates	
	template<class T>
	Logger & operator <<(Logger & stream, T value)
	{
		auto & baseStream = stream.prePrint();
		if (&baseStream != &stream) {
			baseStream << value;
			stream.setBaseTimeTab(false);
		}
		if (stream.is_tabs()) stream.print("\t");
		return streamToPrint(stream, value,  typename typetraits::_Is_integer<T>::_Integral());
		//stream.print(value);
		//return stream;
	}

	template<class T>
	Logger & streamToPrint(Logger & stream, T value, typetraits::__true_type)
	{
		return stream.streamToPrintInt(value);
	}

	template<class T>
	Logger & streamToPrint(Logger & stream, T value, typetraits::__false_type)
	{
		stream.print(value);
		return stream;
	}

	/// <summary>
	/// Per msg: 100uS Due/ 700uS Mega
	/// Save 1KB to SD: 250mS Due / 150mS Mega
	/// </summary>
	class RAM_Logger : public Logger {
	public:
		RAM_Logger(const char * fileName, uint16_t ramFile_size, bool keepSaving, Clock & clock);
		RAM_Logger(const char * fileName, uint16_t ramFile_size, bool keepSaving);
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
		void readAll() override;
	private:
		Logger & logTime() override;
		bool is_tabs() override { return _ram_mustTabTime ? (_ram_mustTabTime = false, true) : _flags & L_tabs; }
		bool _ram_mustTabTime = false;
		uint8_t * _ramFile = 0;
		const char * _fileName;
		uint16_t _ramFile_size;
		uint16_t _filePos = 0;
		bool _keepSaving;
	};

	/// <summary>
	/// Per msg: 115mS Due/ 170mS Mega
	/// Save 1KB to SD: 660mS Due / 230mS Mega
	/// </summary>
	class EEPROM_Logger : public Logger {
	public:
		EEPROM_Logger(const char * fileName, uint16_t startAddr, uint16_t endAddr, bool keepSaving, Clock & clock);
		EEPROM_Logger(const char * fileName, uint16_t startAddr, uint16_t endAddr, bool keepSaving);
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
		void readAll() override;
		void begin(uint32_t = 0) override;
	private:
		Logger & logTime() override;
		bool is_tabs() override { return _ee_mustTabTime ? (_ee_mustTabTime = false, true) : _flags & L_tabs; }
		void saveOnFirstCall();
		void findStart();
		bool _ee_mustTabTime = false;
		const char * _fileName;
		uint16_t _startAddr;
		uint16_t _endAddr;
		uint16_t _currentAddress = 0;
		bool _hasWrittenToSD = false;
		bool _hasRecycled = false;
		bool _keepSaving;
	};

	class Serial_Logger : public Logger {
	public:
		Serial_Logger(uint32_t baudRate, Clock & clock);
		Serial_Logger(uint32_t baudRate);
		Serial_Logger() = default;
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
		void flush() override { Serial.flush(); }
		void begin(uint32_t baudRate) override;
	protected:
		Logger & logTime() override;
		bool is_tabs() override { return _mustTabTime ? (_mustTabTime = false, true) : _flags & L_tabs; }
		bool _mustTabTime = false;
	};

	/// <summary>
	/// Per un-timed msg: 1.4mS
	/// Per timed msg: 3mS
	/// </summary>	
	class SD_Logger : public Serial_Logger {
	public:
		SD_Logger(const char * fileName, uint32_t baudRate, Clock & clock);
		SD_Logger(const char * fileName, uint32_t baudRate);
		bool isWorking() override;
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
		File openSD();
		void flush() override {
			Serial_Logger::flush(); _dataFile.close(); _dataFile = File{}; }
	private:
		Logger & logTime() override;
		bool is_tabs() override { return _SD_mustTabTime ? (_SD_mustTabTime = false, true) : _flags & L_tabs; }
		void setBaseTimeTab(bool timeTab) override { _mustTabTime = timeTab; }
		Logger & prePrint() override {
			_serial = *this;
			return _serial;
		}

		static Serial_Logger _serial;
		File _dataFile;
		const char * _fileName;
		bool _SD_mustTabTime = false;
	};

	Logger & logger(); // to be defined by the client
