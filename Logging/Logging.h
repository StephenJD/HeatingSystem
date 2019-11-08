#pragma once
#include <SD.h>
#include <SPI.h>
//#pragma message( "Logging.h loaded" )
/*
To reduce timeout on missing SD card from 2s to 7mS modify Sd2Card.cpp:
In Sd2Card::init() on first timeout:  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {...
Set timeout to 1mS.
SD.h/.cpp modified to provide sd_exists();

*/
	class Clock;
	enum Flags {L_default, L_dec, L_int, L_concat, L_endl, L_time, L_flush, L_cout = 8, L_hex = 16, L_fixed = 32, L_tabs = 64};
	inline Flags operator +(Flags l_flag, Flags r_flag) {return static_cast<Flags>(l_flag | r_flag);}
	class Logger {
	public:
		Logger() = default;
		Logger(const char * fileName, int baudRate, Clock & clock) {}

		Flags addFlag(Flags flag) { _flags = _flags + flag; return _flags; }
		Flags removeFlag(Flags flag) { _flags = static_cast<Flags>(_flags & ~flag); return _flags; }
		Logger(Clock & clock);
		virtual bool isWorking() { return true; };
		virtual Logger & operator <<(const char *) { return *this; }
		virtual Logger & operator <<(long) { return *this; }
		virtual Logger & operator <<(Flags) { return *this; }
	protected:
		virtual Logger & logChar(const char * msg) { return *this; }
		virtual Logger & logLong(long val) {return *this; }
		virtual Logger & logTime() { return *this; }
		virtual void flush() {}
		bool is_cout() { return _flags & L_cout; }
		bool is_fixed() { return _flags & L_fixed; }
		virtual bool is_tabs() { return false; }
		const char * toFixed(int decimal);
		Clock * _clock = 0;
		Flags _flags = L_default;
	};

	class Serial_Logger : public Logger {
	public:
		Serial_Logger(int baudRate, Clock & clock);
		Serial_Logger(int baudRate);
		Serial_Logger() = default;
		Logger & operator <<(const char * msg) override { return logChar(msg); }
		Logger & operator <<(long val) override { return logLong(val); }
		Logger & operator <<(Flags flag) override;

	protected:
		Logger & logChar(const char * msg) override;
		Logger & logLong(long val) override;
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
	private:
		File openSD();
		Logger & logChar(const char * msg) override;
		Logger & logLong(long val) override;
		Logger & logTime() override;
		void flush() override { Serial.flush(); }
		bool is_tabs() override { return _SD_mustTabTime ? (_SD_mustTabTime = false, true) : _flags & L_tabs; }

		File _dataFile;
		const char * _fileName;
		bool _SD_mustTabTime = false;
	};

	Logger & logger(); // to be defined by the client
