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
enum {
	e_PSU_OFF_V, e_PSU_ON_MIN, e_PSU_ON_MAX, e_PSU_V
	, e_US_ON_TIME, e_US_OFF_TIME, e_US_INT_ON_TIME, e_US_INT_OFF_TIME
	, e_DS_ON_TIME, e_DS_OFF_TIME, e_DS_INT_ON_TIME, e_DS_INT_OFF_TIME
};

auto us_heatRelay = Pin_Wag(e_US_Heat, HIGH);
auto us_coolRelay = Pin_Wag(e_US_Cool, HIGH);
auto ds_heatRelay = Pin_Wag(e_DS_Heat, HIGH);
auto ds_coolRelay = Pin_Wag(e_DS_Cool, HIGH);
auto statusLED = Pin_Wag(e_Status, HIGH);

auto psu_enable = Pin_Wag(e_PSU, LOW, true);
constexpr int SIZE_OF_ARRAY = 300;
uint8_t readings[SIZE_OF_ARRAY];
uint8_t msg[SIZE_OF_ARRAY];
int nextValIndex = 0;
int eeprom_addr = 0;
int psu_offV = 0;
constexpr auto PSU_DIFF = 40;
constexpr auto MAX_TRANSIT = 160;

void setup() {
	logger().begin(SERIAL_RATE);
	logger() << F("Setup Started") << L_flush;
	logResults();
	psu_enable.begin(true);
	us_heatRelay.begin();
	us_coolRelay.begin();
	ds_heatRelay.begin();
	ds_coolRelay.begin();
	statusLED.begin();
	psu_offV = getMaxPSUVoltage(A3);
	writeLog(e_PSU_OFF_V, psu_offV / 4);
	statusLED.set();
	psu_offV = getMaxPSUVoltage(A3);
	writeLog(e_PSU_OFF_V, psu_offV / 4);
	logResults();
	if (psu_offV < 500) psu_offV = 998;
	auto travTime = 0;

	//traverseTo(0, false, false);
	travTime = traverseTo(0, true, false);
	writeLog(e_US_ON_TIME, travTime / 2);
	logResults();
	logger() << F("US OnTime: ") << travTime << L_endl;

	//traverseTo(1, false, false);
	travTime = traverseTo(1, true, false);
	writeLog(e_DS_ON_TIME, travTime / 2);
	logResults();
	logger() << F("DS OnTime: ") << travTime << L_endl;

	travTime = traverseTo(0, false, false);
	writeLog(e_US_OFF_TIME, travTime / 2);
	logResults();
	logger() << F("US OffTime: ") << travTime << L_endl;

	travTime = traverseTo(1, false, false);
	writeLog(e_DS_OFF_TIME, travTime / 2);
	logResults();
	logger() << F("DS OffTime: ") << travTime << L_endl;

	/*travTime = traverseTo(0, true, true);
	writeLog(e_US_INT_ON_TIME, travTime / 2);
	logResults();
	logger() << F("US IntOnTime: ") << travTime << L_endl;

	travTime = traverseTo(0, false, true);
	writeLog(e_US_INT_OFF_TIME, travTime / 2);
	logResults();
	logger() << F("US IntOffTime: ") << travTime << L_endl;

	travTime = traverseTo(1, true, true);
	writeLog(e_DS_INT_ON_TIME, travTime / 2);
	logResults();
	logger() << F("DS IntOnTime: ") << travTime << L_endl;

	travTime = traverseTo(1, false, true);
	writeLog(e_DS_INT_OFF_TIME, travTime / 2);
	logResults();
	logger() << F("DS IntOffTime: ") << travTime << L_endl;*/
	us_heatRelay.clear();
	us_coolRelay.clear();
	ds_heatRelay.clear();
	ds_coolRelay.clear();
}

void loop() {
	//measurePSUVoltage(A1);
	//logResults();
	//measurePSUVoltage(A3);
	//logResults();
	//measurePSUVoltage_uS(A3);
	//measureMaxMinPSUVoltage(A3);
}

int traverseTo(int mixV, bool goHot, bool interrupt) {
	us_heatRelay.clear();
	us_coolRelay.clear();
	ds_heatRelay.clear();
	ds_coolRelay.clear();
	Pin_Wag* activeRelay;
	auto endLogCount = nextValIndex + 100;
	if (mixV == 0) {
		if (goHot) activeRelay = &us_heatRelay;
		else activeRelay = &us_coolRelay;
	}
	else {
		if (goHot) activeRelay = &ds_heatRelay;
		else activeRelay = &ds_coolRelay;
	}
	auto startTime = micros() / 1000000;
	auto endTransition = startTime + MAX_TRANSIT;
	activeRelay->set();
	delay(20);
	int psuV;
	int maxV = 0;
	int minV = 1024;
	auto nextLogTime = Timer_mS(2000);
	bool statusToggle = false;
	do {
		if (interrupt && (micros() / 1000000) % 2 == 0) {
			activeRelay->clear();
			delay(1000);
			++endTransition;
			activeRelay->set();
			delay(20);
		}
		psuV = getMaxPSUVoltage(A3);
		if (psuV < psu_offV - PSU_DIFF) {
			if (psuV < minV)minV = psuV;
			if (psuV > maxV)maxV = psuV;
		}
		if (nextValIndex < endLogCount && nextLogTime) {
			nextLogTime.repeat();
			statusLED.set(statusToggle);
			statusToggle = !statusToggle;
			writeLog(e_PSU_V, psuV / 4);
			logResults();
		}
	} while (micros() / 1000000 < endTransition /*&& psuV < psu_offV - PSU_DIFF*/);
	writeLog(e_PSU_ON_MIN, minV / 4);
	writeLog(e_PSU_ON_MAX, maxV / 4);
	writeLog(e_PSU_OFF_V, psuV / 4);
	logger() << (mixV == 0 ? F("US ") : F("DS ")) << F("PSU_off_V:") << L_tabs << psuV << F("Min:") << minV << F("Max:") << maxV << L_endl;
	return (micros() / 1000000 - startTime);
}

void measurePSUVoltage_mS(int pin) {
	while (nextValIndex < SIZE_OF_ARRAY) {
		auto oneCycleComplete = Timer_mS(25);
		while (nextValIndex < SIZE_OF_ARRAY && !oneCycleComplete) {
			auto millisSince = uint8_t(micros() / 1000);
			int psuV = analogRead(pin);
			if (psuV > 10) {
				writeLog(millisSince, psuV / 4);
				while (millisSince == uint8_t(micros() / 1000));
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
				writeLog(micros() / 100, psuV / 4);
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
			writeLog(psuMaxV / 4, psuMinV / 4);
		}
	}
}

int getMaxPSUVoltage(int pin) {
	auto testComplete = Timer_mS(25);
	int psuMaxV = 0;
	do {
		int thisVoltage = analogRead(pin);
		if (thisVoltage > psuMaxV) psuMaxV = thisVoltage;
	} while (!testComplete);
	//logger() << F("PSUV") << L_tabs << psuMaxV << L_endl;
	return psuMaxV;
}

void writeLog(int msgVal, int val) {
	msg[nextValIndex] = uint8_t(msgVal);
	readings[nextValIndex] = uint8_t(val);
	++nextValIndex;
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
			EEPROM.write(++eeprom_addr, msg[i]);
			EEPROM.write(++eeprom_addr, readings[i]);
			if (eeprom_addr > 1000) break;
		}
		if (eeprom_addr < 1000) nextValIndex = 0;
	}
}