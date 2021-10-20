#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_Device.h>
#include "I2C_RecoverRetest.h"
#include <Logging.h>
#include <EEPROM.h>
#include <Watchdog_Timer.h>

#define SERIAL_RATE 115200

//////////////////////////////// Start execution here ///////////////////////////////
using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_allwaysFlush);
		return _log;
	}
}
using namespace arduino_logger;

#if defined(__SAM3X8E__)
	constexpr uint8_t EEPROM_ADDRESS = 0x50;

	I2C_Talk rtc{ Wire1 };

	EEPROMClass& eeprom() {
		static EEPROMClass_T<rtc> _eeprom_obj{ (rtc.ini(Wire1,100000),rtc.extendTimeouts(5000, 5, 1000),EEPROM_ADDRESS) }; // rtc will be referenced by the compiler, but rtc may not be constructed yet.
		return _eeprom_obj;
	}
	EEPROMClass& EEPROM = eeprom();
#else
	EEPROMClass& eeprom() {
		return EEPROM;
	}
#endif

I2C_Talk i2C{};

I2C_Recover_Retest i2c_recover(i2C);
//I2C_Recover i2c_recover(i2C);
I_I2Cdevice_Recovery masters[] = { {i2c_recover, 5},{i2c_recover, 6} };

constexpr int NO_OF_MASTERS = 2;
constexpr int NO_OF_REGISTERS = 8;
auto register_set = i2c_registers::Registers<NO_OF_REGISTERS>{ i2C };
auto nextMaster = 0;
int DELAY_MS = 50;
int MIN_DELAY_MS = 1;
auto sendFlash = millis();

void setI2CAddress(I2C_Talk & i2C) {
	pinMode(5, INPUT_PULLUP);
	pinMode(6, INPUT_PULLUP);
	pinMode(7, INPUT_PULLUP);
	if (!digitalRead(5)) {
		i2C.setAsMaster(5);
		Serial.println(F("Master 5"));
	} else if (!digitalRead(6)) {
		i2C.setAsMaster(6);
		Serial.println(F("Master 6"));
	} else if (!digitalRead(7)) {
		i2C.setAsMaster(7);
		Serial.println(F("Master 7"));
	} else Serial.println(F("Err: None of Pins 5-7 set LOW."));
}

bool shouldBeInitiating() {
	pinMode(10, INPUT_PULLUP);
	return digitalRead(10);
}

void setNextMaster() {
	do {
		++nextMaster;
		if (nextMaster >= NO_OF_MASTERS) nextMaster = 0;
		if (masters[nextMaster].getAddress() == i2C.address()) ++nextMaster;
	} while (nextMaster >= NO_OF_MASTERS);
}

void sendData(int master) {
	static long endFlash = sendFlash + 10;
	if (millis() > sendFlash) {
		digitalWrite(LED_BUILTIN, HIGH);
		sendFlash = millis() + 200;
		endFlash = millis() + 10;
	} else if (millis() > endFlash) {
		digitalWrite(LED_BUILTIN, LOW);
	}  
	logger() << i2C.address() << F(" sending Data with delay ") << DELAY_MS << L_endl;
	for (int i = 0; i < NO_OF_REGISTERS; ++i) {
		register_set.setRegister(i, i2C.address());
	}
	// dont't send to reg[0]. Leave that for owners to set.
	auto status = masters[master].write(1, NO_OF_REGISTERS-1, register_set.reg_ptr(1));
	if (status != _OK) {
		logger() << F("Can't sendData with delay ") << DELAY_MS << I2C_Talk::getStatusMsg(status) << L_endl;
		if (status == _disabledDevice) masters[master].reset();
		++DELAY_MS;
	} else {
		--DELAY_MS;
		if (DELAY_MS < MIN_DELAY_MS) DELAY_MS = MIN_DELAY_MS;
	}
}

void setRegListen() {
	register_set.setRegister(0, 'L');
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(200);
	digitalWrite(LED_BUILTIN, LOW);
}

void requestData(int master) {
	logger() << F("requestData from ") << masters[master].getAddress() << L_endl;
	auto status = masters[master].read(0, NO_OF_REGISTERS, register_set.reg_ptr(0));
	if (status == _disabledDevice) masters[master].reset();
}

void logRegisters(const __FlashStringHelper* msg) {
	logger() << msg << F(" Registers for ") << i2C.address() << L_tabs;
	for (int i = 0; i < NO_OF_REGISTERS; ++i) {
		auto regVal = register_set.getRegister(i);
		logger() << (regVal < 20 ? regVal : char(regVal));
	}
	logger() << L_flush;
}

void setup() {
	Serial.begin(SERIAL_RATE);
	set_watchdog_timeout_mS(8000); // max 16S
	logger().begin(SERIAL_RATE);
	logger() << F("Setup Started") << L_endl;
	setI2CAddress(i2C);
	i2C.setTimeouts(10000, I2C_Talk::WORKING_STOP_TIMEOUT, 20000);
	i2C.setMax_i2cFreq(100000);
	i2C.onReceive(register_set.receiveI2C);
	i2C.onRequest(register_set.requestI2C);
	i2C.begin();
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
	logger() << F("Setup Done") << L_flush;
}

void loop() {
	reset_watchdog();
	if (shouldBeInitiating()) {
		logRegisters(F("Set by other"));
		delay(DELAY_MS);
		sendData(nextMaster);
		requestData(nextMaster);
		logRegisters(F("Set by Me"));
		setNextMaster();
		logger() << L_endl;
		delay(DELAY_MS);
	} else {
		setRegListen();
		logger() << F("Listening...") << L_flush;
		delay(1000);
	}
}