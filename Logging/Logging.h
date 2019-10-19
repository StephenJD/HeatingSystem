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

	class Logger {
	public:
		Logger(Clock & clock);
		virtual bool isWorking() { return true; };
		virtual void log() {};
		virtual void log(const char * msg) {}
		virtual void log(const char * msg, long val) {}
		virtual void log_notime(const char * msg, long val = 0) {}
		virtual void log(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD) {}
		virtual void log(const char * msg, long l1, long l2, long l3 = 0, long l4 = 0, long l5 = 0, long l6 = 0, long l7 = 0, long decimal = 0, bool pling = false) {}
	protected:
		Logger() = default;
		Clock * _clock = 0;
	};

	class Serial_Logger : public Logger {
	public:
		Serial_Logger(int baudRate, Clock & clock);
		Serial_Logger(int baudRate);
		void log() override;
		void log(const char * msg) override;
		void log(const char * msg, long val) override;
		void log_notime(const char * msg, long val = 0) override;
		void log(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD) override;
		void log(const char * msg, long l1, long l2, long l3 = 0, long l4 = 0, long l5 = 0, long l6 = 0, long l7 = 0, long decimal = 0, bool pling = false) override;
	protected:
		const char * logTime();
	};

	class SD_Logger : public Serial_Logger {
	public:
		SD_Logger(const char * fileName, int baudRate, Clock & clock);
		SD_Logger(const char * fileName, int baudRate);
		bool isWorking() override;
		void log() override;
		void log(const char * msg) override;
		void log(const char * msg, long val) override;
		void log_notime(const char * msg, long val = 0) override;
		void log(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD) override;
		void log(const char * msg, long l1, long l2, long l3 = 0, long l4 = 0, long l5 = 0, long l6 = 0, long l7 = 0, long decimal = 0, bool pling = false) override;
	private:
		File openSD();

		File _dataFile;
		const char * _fileName;
	};

	Logger & logger(); // to be defined by the client
