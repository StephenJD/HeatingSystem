#pragma once
#include <SD.h>
#include <SPI.h>
#include <Arduino.h>
#include "..\type_traits\type_traits.h"
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
	class Clock;
	enum Flags {L_default, L_dec, L_int, L_concat, L_endl, L_time, L_flush, L_cout = 8, L_hex = 16, L_fixed = 32, L_tabs = 64};
	inline Flags operator +(Flags l_flag, Flags r_flag) {return static_cast<Flags>(l_flag | r_flag);}

	class Logger : public Print {
	public:
		Logger() = default;
		Logger(const char * fileName, int baudRate, Clock & clock) {}

		Flags addFlag(Flags flag) { _flags = _flags + flag; return _flags; }
		Flags removeFlag(Flags flag) { _flags = static_cast<Flags>(_flags & ~flag); return _flags; }
		Logger(Clock & clock);
		size_t write(uint8_t) override { return 1; }
		size_t write(const uint8_t *buffer, size_t size) override { return size; }

		virtual bool isWorking() { return true; };
		virtual void readAll() {};
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
		virtual void flush() {}
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
		return streamToPrint(stream, value,  typename _Is_integer<T>::_Integral());
		//stream.print(value);
		//return stream;
	}

	template<class T>
	Logger & streamToPrint(Logger & stream, T value, __true_type)
	{
		return stream.streamToPrintInt(value);
	}

	template<class T>
	Logger & streamToPrint(Logger & stream, T value, __false_type)
	{
		stream.print(value);
		return stream;
	}

	class EEPROM_Logger : public Logger {
	public:
		EEPROM_Logger(unsigned int startAddr, unsigned int endAddr) : _startAddr(startAddr), _endAddr(endAddr), _currentAddress(startAddr) {}
		void resetStart() { _currentAddress = _startAddr; }
		unsigned int currentAddress() { return _currentAddress; }
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
		void readAll() override;
	private:
		Logger & logTime() override;
		bool is_tabs() override { return _ee_mustTabTime ? (_ee_mustTabTime = false, true) : _flags & L_tabs; }
		bool _ee_mustTabTime = false;
		unsigned int _startAddr;
		unsigned int _endAddr;
		unsigned int _currentAddress = 0;
	};

	class Serial_Logger : public Logger {
	public:
		Serial_Logger(int baudRate, Clock & clock);
		Serial_Logger(int baudRate);
		Serial_Logger() = default;
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
	protected:
		Logger & logTime() override;
		void flush() override { Serial.flush(); }
		bool is_tabs() override { return _mustTabTime ? (_mustTabTime = false, true) : _flags & L_tabs; }
		bool _mustTabTime = false;
	};

	class SD_Logger : public Serial_Logger {
	public:
		SD_Logger(const char * fileName, int baudRate, Clock & clock);
		SD_Logger(const char * fileName, int baudRate);
		bool isWorking() override;
		size_t write(uint8_t) override;
		size_t write(const uint8_t *buffer, size_t size) override;
	private:
		File openSD();
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
