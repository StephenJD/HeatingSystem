#pragma once
#include <SD.h>

namespace HardwareInterfaces {
	class Null_Logger {
	public:
		virtual void logToSD() {}
		virtual void logToSD(const char * msg) {}
		virtual void logToSD(const char * msg, long val) {}
		virtual void logToSD_notime(const char * msg, long val = 0) {}
		virtual void logToSD(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD) {}
		virtual void logToSD(const char * msg, long l1, long l2, long l3 = 0, long l4 = 0, long l5 = 0, long l6 = 0, long l7 = 0, long decimal = 0, bool pling = false) {}
	};

	class Serial_Logger : public Null_Logger {
	public:
		void logToSD() override;
		void logToSD(const char * msg) override;
		void logToSD(const char * msg, long val) override;
		void logToSD_notime(const char * msg, long val = 0) override;
		void logToSD(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD) override;
		void logToSD(const char * msg, long l1, long l2, long l3 = 0, long l4 = 0, long l5 = 0, long l6 = 0, long l7 = 0, long decimal = 0, bool pling = false) override;
	protected:
		const char * logTime();
	};

	class SD_Logger : public Serial_Logger {
	public:
		SD_Logger(const char * fileName) : _fileName(fileName) {
			SD.begin();
			Serial.println("Logger Constructed");
		}
		void logToSD() override;
		void logToSD(const char * msg) override;
		void logToSD(const char * msg, long val) override;
		void logToSD_notime(const char * msg, long val = 0) override;
		void logToSD(const char * msg, long val, const char * name, long val2 = 0xFFFFABCD) override;
		void logToSD(const char * msg, long l1, long l2, long l3 = 0, long l4 = 0, long l5 = 0, long l6 = 0, long l7 = 0, long decimal = 0, bool pling = false) override;
	private:
		File openSD();

		File _dataFile;
		const char * _fileName;
	};

	Null_Logger & log(); // to be defined by the client
}