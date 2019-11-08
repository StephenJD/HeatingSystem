#include "Logging.h"
#include <../DateTime/src/Date_Time.h>
#include <../Clock/Clock.h>
//#include <Conversions.h>
#include <..\Conversions\Conversions.h>
#include <Arduino.h>

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif
	using namespace GP_LIB;

	Logger::Logger(Clock & clock) : _clock(&clock) {}

	const char * Logger::toFixed(int decimal) {
		int whole = (decimal / 256) * 100;
		int fractional = (decimal % 256) / 16;
		fractional = int(fractional * 6.25);
		return decToString(whole + fractional, 4, 2);
	}
	   
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

	Logger & Serial_Logger::operator <<(Flags flag) {
		//Flags {L_default, L_dec, L_int, L_concat, L_endl, L_time, L_cout = 8, L_hex = 16, L_fixed = 32, L_tabs = 64};
		switch (flag) {
		case L_flush: flush(); // fall through
		case L_endl:  logChar("\n"); // fall through
		case L_default: _flags = L_default; break;
		case L_time: logTime(); break;
		case L_dec:	removeFlag(L_hex); break;	
		case L_int:	removeFlag(L_fixed); break;
		case L_concat:	removeFlag(L_tabs); break;
		default:
			addFlag(flag);
		}
		return *this;
	}	

	Logger & Serial_Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << "No Time";
			}
			else {
				*this << *_clock;
			}
		}
		_mustTabTime = true;
		return *this;
	}

	Logger & Serial_Logger::logChar(const char * msg) {
		if (is_tabs()) Serial.print("\t");
		Serial.print(msg);
		return *this;
	}

	Logger & Serial_Logger::logLong(long val) {
		if (is_tabs()) Serial.print("\t");
		if (is_fixed()) Serial.print(toFixed(val));
		else Serial.print(val);
		return *this;
	}

	////////////////////////////////////
	//            SD_Logger           //
	////////////////////////////////////
	constexpr int chipSelect = 53; // 4; // 53;

	SD_Logger::SD_Logger(const char * fileName, int baudRate) : Serial_Logger(baudRate), _fileName(fileName) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		SD.begin(chipSelect);
		Serial_Logger::logChar("SD_Logger Begun\n");
	}	
	
	SD_Logger::SD_Logger(const char * fileName, int baudRate, Clock & clock) : Serial_Logger(baudRate, clock), _fileName(fileName) {
		// Avoid calling Serial_Logger during construction, in case clock is broken.
		SD.begin(chipSelect);
		Serial_Logger::logChar("SD_Logger Begun\n");
	}

	bool SD_Logger::isWorking() { return SD.sd_exists(chipSelect); }

	File SD_Logger::openSD() {
		if (SD.sd_exists(chipSelect)) {
			_dataFile = SD.open(_fileName, FILE_WRITE); // appends to file
		} 
		return _dataFile;
	}

	Logger & SD_Logger::logChar(const char * msg) {
		Serial_Logger::logChar(msg);
		if (!is_cout() && openSD()) {
			if (is_tabs()) _dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.close();
		}
		return *this;
	}

	Logger & SD_Logger::logLong(long val) {
		Serial_Logger::logLong(val);
		if (!is_cout() && openSD()) {
			if (is_tabs()) _dataFile.print("\t");
			if (is_fixed()) _dataFile.print(toFixed(val));
			else _dataFile.print(val);
			_dataFile.close();
		}
		return *this;
	}

	Logger & SD_Logger::logTime() {
		Serial_Logger::logTime();
		_SD_mustTabTime = true;
	}

