#include <PinObject.h>
#include <Logging.h>
#include <Timer_mS_uS.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <EEPROM.h>

using namespace HardwareInterfaces;

constexpr uint32_t SERIAL_RATE = 115200;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE);
		return _log;
	}
}
using namespace arduino_logger;

enum { e_PSU = 6, e_Slave_Sense = 7 };

enum { e_US_Heat = 11, e_US_Cool = 10, e_DS_Heat = 12, e_DS_Cool = A0, e_Status = 13 };

auto us_coolRelay = Pin_Wag(e_US_Cool, HIGH);
auto psu_enable = Pin_Wag(e_PSU, LOW, true);
constexpr int SIZE_OF_ARRAY = 300;
uint8_t readings[SIZE_OF_ARRAY];
uint8_t timings[SIZE_OF_ARRAY];
int nextValIndex = 0;
int eeprom_addr = 0;

void setup() {
	logger().begin(SERIAL_RATE);
	logger() << F("Setup Started") << L_flush;
	psu_enable.begin(true);
	us_coolRelay.set();
}

void loop() {
	//measurePSUVoltage(A1);
	//logResults();
	//measurePSUVoltage(A3);
	logResults();
	//measurePSUVoltage_uS(A3);
	measureMaxMinPSUVoltage(A3);
}

void measurePSUVoltage_mS(int pin) {
	while (nextValIndex < SIZE_OF_ARRAY) {
		auto oneCycleComplete = Timer_mS(25);
		while (nextValIndex < SIZE_OF_ARRAY && !oneCycleComplete) {
			auto millisSince = uint8_t(micros() / 1000);
			int psuV = analogRead(pin);
			if (psuV > 10) {
				timings[nextValIndex] = millisSince;
				readings[nextValIndex] = psuV/4;
				while (millisSince == uint8_t(micros() / 1000));
				++nextValIndex;
			}
		}
	}
}

void measurePSUVoltage_uS(int pin) {
	while (nextValIndex < SIZE_OF_ARRAY) {
		//logger() << F("PSUV") << L_tabs << nextValIndex << L_endl;
		auto oneCycleComplete = Timer_mS(25);
		while (nextValIndex < SIZE_OF_ARRAY && !oneCycleComplete) {
			int psuV = analogRead(pin);
			if (psuV > 10) {
				timings[nextValIndex] = micros()/100;
				readings[nextValIndex] = psuV/4;
				++nextValIndex;
			}
		}
	}
}

void measureMaxMinPSUVoltage(int pin) {
	while (nextValIndex < SIZE_OF_ARRAY) {
		auto testComplete = Timer_mS(25);
		int psuMaxV = 0;
		int psuMinV = 1024;
		do {
			int thisVoltage = analogRead(pin);
			if (thisVoltage < psuMinV) psuMinV = thisVoltage;
			if (thisVoltage > psuMaxV) psuMaxV = thisVoltage;
		} while (!testComplete);

		if (psuMaxV > 10 && psuMaxV < 1024) {
			//logger() << F("PSUV") << L_tabs << psuV << nextValIndex << L_endl;
			timings[nextValIndex] = psuMaxV / 4;
			readings[nextValIndex] = psuMinV /4;
			++nextValIndex;
		}
	}
}

void logResults() {
	//logger() << F("Log") << L_tabs << nextValIndex << eeprom_addr << L_endl;
	if (nextValIndex == 0 || (eeprom_addr > 1000 && nextValIndex <= SIZE_OF_ARRAY)) {
		for (int i = 1; i < 1000; ++i) {
			logger() << i << L_tabs << EEPROM.read(i) << EEPROM.read(++i) << eeprom_addr << L_endl;
		}
		if (eeprom_addr < 1000) nextValIndex = 1; else nextValIndex = SIZE_OF_ARRAY + 1;
	}
	else if (eeprom_addr < 1000) {
		logger() << F("Write") << L_endl;
		for (int i = 0; i < nextValIndex; ++i) {
			EEPROM.write(++eeprom_addr, timings[i]);
			EEPROM.write(++eeprom_addr, readings[i]);
			if (eeprom_addr > 1000) break;
		}
		if (eeprom_addr < 1000) nextValIndex = 0;
	}
}
