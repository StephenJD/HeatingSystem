#include "Logging.h"
#include <Date_Time.h>
#include <Clock.h>
#include <Conversions.h>
#include <MemoryFree.h>
//
// add 
// compiler.warning_flags.default= -Wno-attributes
// to C:\Users\Stephen\AppData\Local\arduino15\packages\arduino\hardware\sam\1.6.12\platform.txt
// 
//#if defined(__SAM3X8E__)
#include <EEPROM_RE.h>
//#endif

#ifdef ZPSIM
#include <iostream>
using namespace std;
#endif
using namespace GP_LIB;

namespace arduino_logger {

	Logger::Logger(Clock& clock, Flags initFlag) : _flags{ initFlag }, _clock(&clock) {}

	Logger& Logger::toFixed(int decimal) {
		int whole = (decimal / 256);
		double fractional = (decimal % 256) / 256.;
		print(double(whole + fractional));
		return *this;
	}

	Logger& Logger::operator <<(Flags flag) {
		if (is_null()) return *this;
		//Serial.print("<<flag:"); Serial.print(flag); Serial.print(" Flag:"); Serial.print(_flags); Serial.print(" FlshF:"); Serial.println(_flags & L_startWithFlushing); Serial.flush();
		switch (flag) {
		case L_time:	logTime(); break;
		case L_flush:
			*this << F(" |F|\n");
			if (is_cout()) Serial.flush();
			else flush();
			_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
			break;
		case L_endl:
		{
			if (_flags & L_allwaysFlush) { *this << F(" |F|"); }
			else if ((_flags & L_startWithFlushing) == L_startWithFlushing) { *this << F(" |SF|"); }
			auto streamPtr = &stream();
			do {
				endl(*streamPtr);
			} while (mirror_stream(streamPtr));
			if (_flags & L_allwaysFlush || ((_flags & L_startWithFlushing) == L_startWithFlushing)) flush();
		}
		[[fallthrough]];
		case L_clearFlags:
		{
			bool isStartWithFlushing = (_flags & L_startWithFlushing) == L_startWithFlushing;
			_flags = static_cast<Flags>(_flags & L_allwaysFlush); // all zero's except L_allwaysFlush if set.
			if (isStartWithFlushing) _flags += L_startWithFlushing;
		}
		break;
		case L_allwaysFlush: _flags += L_allwaysFlush; break;
		case L_concat:	removeFlag(L_tabs); break;
		case L_dec:
		case L_int:
			removeFlag(L_fixed);
			removeFlag(L_hex);
			//removeFlag(L_dec);
			//removeFlag(L_int);
			//Serial.print("dec/int Flag:"); Serial.println(_flags); Serial.flush();
			break;
		case L_hex:
		case L_fixed:
			[[fallthrough]];
		default:
			addFlag(flag);
			//Serial.print("AddFlag:"); Serial.println(_flags); Serial.flush();
		}
		return *this;
	}

	Logger& Logger::logTime() {
		if (_clock) {
			_clock->refresh();
			if (_clock->day() == 0) {
				*this << F("No Time");
			}
			else {
				*this << *_clock;
			}
			_flags += L_time;
		}
		return *this;
	}

	////////////////////////////////////
	//            Serial_Logger       //
	////////////////////////////////////
	char logTimeStr[18];
	char decTempStr[7];

	Serial_Logger::Serial_Logger(uint32_t baudRate, Flags initFlags) : Logger(initFlags) {
		Serial.flush();
		Serial.begin(baudRate);
		auto freeRam = freeMemory();
		Serial.print(F("\nRAM: ")); Serial.println(freeRam); Serial.flush();
		//if (freeRam < 10) { while (true); }
#ifdef DEBUG_TALK
		_flags = L_allwaysFlush;
#endif
	}

	Serial_Logger::Serial_Logger(uint32_t baudRate, Clock& clock, Flags initFlags) : Logger(clock, initFlags) {
		Serial.flush();
		Serial.begin(baudRate);
		auto freeRam = freeMemory();
		Serial.print(F("\nRAM: ")); Serial.println(freeRam); Serial.flush();
		if (freeRam < 10) { while (true); }
#ifdef DEBUG_TALK
		_flags = L_allwaysFlush;
#endif
	}

	void Serial_Logger::begin(uint32_t baudRate) {
		if (baudRate == 0) Serial.println(F("Baud Rate required"));
		else {
			Serial.begin(baudRate);
			//Serial.print(F("Baud Rate: ")); Serial.println(baudRate);
		}
	}

	size_t Serial_Logger::write(uint8_t chr) {
		Serial.print(char(chr));
		return 1;
	}

	size_t Serial_Logger::write(const uint8_t* buffer, size_t size) {
		Serial.print((char*)buffer);
		return size;
	}

}