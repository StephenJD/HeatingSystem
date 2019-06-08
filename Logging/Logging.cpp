#include "Logging.h"
#include <../Clock/Clock.h>
#include <../DateTime/src/Date_Time.h>
#include <Conversions.h>
#include <Arduino.h>

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif
	using namespace GP_LIB;

	Logger::Logger(Clock & clock) : _clock(&clock) {}

	////////////////////////////////////
	//            Serial_Logger       //
	////////////////////////////////////
	char logTimeStr[18];
	char decTempStr[7];

	Serial_Logger::Serial_Logger(int baudRate) : Logger() {
		Serial.flush();
		Serial.begin(baudRate);
		Serial.println(" Serial_Logger Begun");
	}	
	
	Serial_Logger::Serial_Logger(int baudRate, Clock & clock) : Logger(clock) {
		Serial.flush();
		Serial.begin(baudRate);
		Serial.println(" Serial_Logger Begun");
	}

	const char * Serial_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				strcpy(logTimeStr, "No Time: ");
			}
			else {
				strcpy(logTimeStr, intToString(_clock->day(), 2));
				strcat(logTimeStr, "/");
				strcat(logTimeStr, intToString(_clock->month(), 2));
				strcat(logTimeStr, "/");
				strcat(logTimeStr, intToString(_clock->year(), 2));
				strcat(logTimeStr, " ");
				strcat(logTimeStr, intToString(_clock->hrs(), 2));
				strcat(logTimeStr, ":");
				strcat(logTimeStr, intToString(_clock->mins10(), 1));
				strcat(logTimeStr, intToString(_clock->minUnits(), 1));
			}
			Serial.flush();
			//Serial.println("... got logTime()");
		} else strcpy(logTimeStr, "");
		return logTimeStr;
	}

	void Serial_Logger::log() {
		Serial.println();
	}

	void Serial_Logger::log(const char * msg) {
		Serial.print(logTime());
		Serial.print("\t");
		Serial.println(msg);
	}

	void Serial_Logger::log(const char * msg, long val) {
		Serial.print(logTime());
		Serial.print("\t");
		Serial.print(msg);
		Serial.print("\t");
		Serial.println(val);
	}

	void Serial_Logger::log_notime(const char * msg, long val) {
		Serial.print("\t");
		Serial.print(msg);
		Serial.print("\t");
		Serial.println(val);
	}

	void Serial_Logger::log(const char * msg, long val, const char * name, long val2) {
		Serial.print(logTime());
		Serial.print("\t");
		Serial.print(msg);
		Serial.print("\t");
		Serial.print(val);
		Serial.print("\t");
		Serial.print(name);
		if (val2 != 0xFFFFABCD) {
			Serial.print("\t");
			Serial.println(val2);
		}
		else Serial.println();
	}

	void Serial_Logger::log(const char * msg, long l1, long l2, long l3, long l4, long l5, long l6, long l7, long decimal, bool pling) {
		int deg = (decimal / 256) * 100;
		int fract = (decimal % 256) / 16;
		fract = int(fract * 6.25);

		Serial.print(logTime());
		Serial.print("\t");
		Serial.print(msg);
		Serial.print(l1);
		Serial.print("\t");
		Serial.print(l2);
		Serial.print("\t");
		Serial.print(l3);
		Serial.print("\t");
		Serial.print(l4);
		Serial.print("\t");
		Serial.print(l5);
		Serial.print("\t");
		Serial.print(l6);
		Serial.print("\t");
		Serial.print(l7);
		Serial.print("\t");
		Serial.print(decToString(deg + fract, 4, 2));
		Serial.print("\t");
		Serial.println(pling ? "!" : "");
	};
	
	////////////////////////////////////
	//            SD_Logger           //
	////////////////////////////////////

	SD_Logger::SD_Logger(const char * fileName, int baudRate) : Serial_Logger(baudRate), _fileName(fileName) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		openSD();
		Serial.println("SD_Logger Begun");
	}	
	
	SD_Logger::SD_Logger(const char * fileName, int baudRate, Clock & clock) : Serial_Logger(baudRate, clock), _fileName(fileName) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		openSD();
		Serial.println("SD_Logger Begun");
	}

	File SD_Logger::openSD() {
		static auto SD_OK = SD.begin();
		static auto nextTry = millis(); 
		//Serial.println("openSD()");

		if (millis() >= nextTry) { // every 10 secs.
			//Serial.println("... check if card present.");
			if (SD.cardMissing()) {
				//Serial.println("... try SD.begin()");
				SD_OK = SD.begin();
				if (!SD_OK) {
					Serial.println("SD Card Missing");
				}
			}
			nextTry = millis() + 10000;
		}
		if (SD_OK) {
			_dataFile = SD.open(_fileName, FILE_WRITE);
		} 
		if (!_dataFile) {
			Serial.println("Unable to open file");
		}
		return _dataFile;
	}

	void SD_Logger::log() {
		Serial_Logger::log();
		if (openSD()) {
			_dataFile.println();
			_dataFile.close();
		}
	}

	void SD_Logger::log(const char * msg) {
		Serial_Logger::log(msg);		
		if (openSD()) {
			_dataFile.print(logTime());
			_dataFile.print("\t");
			_dataFile.println(msg);
			_dataFile.close();
		}
	}

	void SD_Logger::log(const char * msg, long val) {
		Serial_Logger::log(msg, val);		
		if (openSD()) {
			_dataFile.print(logTime());
			_dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.print("\t");
			_dataFile.println(val);
			_dataFile.close();
		}
	}

	void SD_Logger::log_notime(const char * msg, long val) {
		Serial_Logger::log_notime(msg, val);	
		if (openSD()) {
			_dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.print("\t");
			_dataFile.println(val);
			_dataFile.close();
		}
	}

	void SD_Logger::log(const char * msg, long val, const char * name, long val2) {
		Serial_Logger::log(msg, val, name, val2);
		if (openSD()) {
			_dataFile.print(logTime());
			_dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.print("\t");
			_dataFile.print(val);
			_dataFile.print("\t");
			_dataFile.print(name);
			if (val2 != 0xFFFFABCD) {
				_dataFile.print("\t");
				_dataFile.println(val2);
			}
			else _dataFile.println();
			_dataFile.close();
		}
	}

	void SD_Logger::log(const char * msg, long l1, long l2, long l3, long l4, long l5, long l6, long l7, long decimal, bool pling) {
		Serial_Logger::log(msg, l1, l2, l3, l4, l5, l6, l7, decimal, pling);
		
		int deg = (decimal / 256) * 100;
		int fract = (decimal % 256) / 16;
		fract = int(fract * 6.25);
		if (openSD()) {
			_dataFile.print(logTime());
			_dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.print(l1);
			_dataFile.print("\t");
			_dataFile.print(l2);
			_dataFile.print("\t");
			_dataFile.print(l3);
			_dataFile.print("\t");
			_dataFile.print(l4);
			_dataFile.print("\t");
			_dataFile.print(l5);
			_dataFile.print("\t");
			_dataFile.print(l6);
			_dataFile.print("\t");
			_dataFile.print(l7);
			_dataFile.print("\t");
			_dataFile.print(decToString(deg + fract, 4, 2));
			_dataFile.print("\t");
			_dataFile.println(pling ? "!" : "");
			_dataFile.close();
		}
	};
