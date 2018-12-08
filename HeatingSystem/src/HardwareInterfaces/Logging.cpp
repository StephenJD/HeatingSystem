#include "Logging.h"
#include "Clock.h"
#include <Date_Time.h>
#include <Convertions.h>
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>

namespace HardwareInterfaces {
	using namespace GP_LIB;
	////////////////////////////////////
	//            Serial_Logger       //
	////////////////////////////////////
	char logTimeStr[18];
	char decTempStr[7];

	const char * Serial_Logger::logTime() {
		//Serial.println("logTime()");
		clock_().refresh();
		if (clock_().day() == 0) {
			strcpy(logTimeStr, "No Time: ");
		}
		else {
			strcpy(logTimeStr, intToString(clock_().day(), 2));
			strcat(logTimeStr, "/");
			strcat(logTimeStr, intToString(clock_().month(), 2));
			strcat(logTimeStr, "/");
			strcat(logTimeStr, intToString(clock_().year(), 2));
			strcat(logTimeStr, " ");
			strcat(logTimeStr, intToString(clock_().hrs(), 2));
			strcat(logTimeStr, ":");
			strcat(logTimeStr, intToString(clock_().mins10(), 1));
			strcat(logTimeStr, intToString(clock_().minUnits(), 1));
		}
		//Serial.println("... got logTime()");
		return logTimeStr;
	}

	void Serial_Logger::logToSD() {
		Serial.println();
	}

	void Serial_Logger::logToSD(const char * msg) {
		Serial.print(logTime());
		Serial.print("\t");
		Serial.println(msg);
	}

	void Serial_Logger::logToSD(const char * msg, long val) {
		Serial.print(logTime());
		Serial.print("\t");
		Serial.print(msg);
		Serial.print("\t");
		Serial.println(val);
	}

	void Serial_Logger::logToSD_notime(const char * msg, long val) {
		Serial.print("\t");
		Serial.print(msg);
		Serial.print("\t");
		Serial.println(val);
	}

	void Serial_Logger::logToSD(const char * msg, long val, const char * name, long val2) {
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

	void Serial_Logger::logToSD(const char * msg, long l1, long l2, long l3, long l4, long l5, long l6, long l7, long decimal, bool pling) {
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

	File SD_Logger::openSD() {
		static decltype(SD.begin()) SD_OK = 0;
		static unsigned long nextTry = millis(); // every 10 secs.
		//Serial.println("openSD()");

		if (millis() >= nextTry) {
			//Serial.println("... try cardMissing()");
			if (SD.cardMissing()) {
				//Serial.println("... try SD.begin()");
				SD_OK = SD.begin();
				if (SD_OK) {
					Serial.println("SD.Begin OK");
				}
				else {
					Serial.println("error opening SD file");
				}
			}
			nextTry = millis() + 10000;
		}
		if (SD_OK) {
			_dataFile = SD.open(_fileName, FILE_WRITE);
		}
		//Serial.print("... done openSD() FileOK?: ");
		//Serial.println(bool(_dataFile));
		return _dataFile;
	}

	void SD_Logger::logToSD() {
		Serial_Logger::logToSD();
		_dataFile = openSD();
		if (_dataFile) {
			_dataFile.println();
			_dataFile.close();
		}
	}

	void SD_Logger::logToSD(const char * msg) {
		Serial_Logger::logToSD(msg);
		_dataFile = openSD();
		if (_dataFile) {
			_dataFile.print(logTime());
			_dataFile.print("\t");
			_dataFile.println(msg);
			_dataFile.close();
		}
	}

	void SD_Logger::logToSD(const char * msg, long val) {
		Serial_Logger::logToSD(msg, val);
		_dataFile = openSD();
		if (_dataFile) {
			_dataFile.print(logTime());
			_dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.print("\t");
			_dataFile.println(val);
			_dataFile.close();
		}
	}

	void SD_Logger::logToSD_notime(const char * msg, long val) {
		Serial_Logger::logToSD_notime(msg, val);
		_dataFile = openSD();
		if (_dataFile) {
			_dataFile.print("\t");
			_dataFile.print(msg);
			_dataFile.print("\t");
			_dataFile.println(val);
			_dataFile.close();
		}
	}

	void SD_Logger::logToSD(const char * msg, long val, const char * name, long val2) {
		Serial_Logger::logToSD(msg, val, name, val2);
		_dataFile = openSD();
		if (_dataFile) {
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

	void SD_Logger::logToSD(const char * msg, long l1, long l2, long l3, long l4, long l5, long l6, long l7, long decimal, bool pling) {
		Serial_Logger::logToSD(msg, l1, l2, l3, l4, l5, l6, l7, decimal, pling);
		_dataFile = openSD();
		int deg = (decimal / 256) * 100;
		int fract = (decimal % 256) / 16;
		fract = int(fract * 6.25);
		if (_dataFile) {
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

}
