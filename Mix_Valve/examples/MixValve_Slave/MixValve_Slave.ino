//#include <MemoryFree.h>
#include "I2C_Talk.h"
#include "I2C_Recover.h"
#include "I2C_RecoverRetest.h"
#include "Mix_Valve.h"
#include <TempSensor.h>
#include <PinObject.h>
#include <Logging.h>
#include <EEPROM.h>
#include <Watchdog_Timer.h>
#include <Timer_mS_uS.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>

// ******** NOTE select Arduino Pro Mini 328P 3.3v 8MHz **********
// Arduino Pro Mini must be upgraded with  boot-loader to enable watchdog timer.
// File->Preferences->Additional Boards Manager URLs: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
// Tools->Board->Boards Manager->Install MiniCore
// 1. In Arduino: Load File-> Examples->ArduinoISP->ArduinoISP
// 2. Upload this to any board, including another Mini Pro.
// 3. THEN connect SPI pins from the ISP board to the Mini Pro: MOSI(11/51)->11, MISO(12/50)->12, SCK(13/52)->13, 10->Reset on MiniPro, Vcc and 0c pins.
// 4. Tools->Board->MiniCore Atmega 328
// 5. External 8Mhz clock, BOD 2.7v, LTO enabled, Variant 328P, UART0.
// 6. Tools->Programmer->Arduino as ISP
// 7. Burn Bootloader
// 8. Then upload your desired sketch to ordinary ProMini board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

// ********** NOTE: ***************
// If watchdog is being used, set_watchdog_timeout_mS() must be set at the begining of startup, before any calls to delay()
// Otherwise, depending on how cleanly the Arduino sht down, it may time-out before it is reset.
// ********************************

// ********** NOTE: ***************
// To open two serial monitors
// You must start a second Arduino session from the desktop icon
// NOT from within an existing Arduino session.
// ********************************

using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;

constexpr uint32_t SERIAL_RATE = 115200;
constexpr int noOfMixers = 2;
constexpr uint8_t eeprom_firstRegister_addr = 0x2;
uint8_t version_a = 0x2; // change to force re-application of defaults incl temp-sensor addresses
uint8_t version_b = 0x0;

enum { e_PSU = 6, e_Slave_Sense = 7 };
enum { ds_mix, us_mix };

enum { e_US_Heat = 11, e_US_Cool = 10, e_DS_Heat = 12, e_DS_Cool = A0, e_Status = 13 };

enum { e_DS_TempAddr = 0x4f, e_US_TempAddr = 0x2C }; // only used when version-changed
//enum { e_DS_TempAddr = 0x29, e_US_TempAddr = 0x37 }; // only used when version-changed
enum Role role = e_Master;

I2C_Talk & i2C() {
	static I2C_Talk _i2C; // not initialised until this translation unit initialised.
	return _i2C;
}

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

EEPROMClass & eeprom() {
	return EEPROM;
}

uint8_t regSet = 0;
uint8_t reg = 0;
void roleChanged(Role newRole);
const __FlashStringHelper * showErr(Mix_Valve::Error err);

void ui_yield() {}

auto ds_heatRelay = Pin_Wag(e_DS_Heat, HIGH);
auto ds_coolRelay = Pin_Wag(e_DS_Cool, HIGH);
auto us_heatRelay = Pin_Wag(e_US_Heat, HIGH);
auto us_coolRelay = Pin_Wag(e_US_Cool, HIGH);
auto psu_enable = Pin_Wag(e_PSU, LOW, true);

auto led_Status = Pin_Wag(e_Status, HIGH);
auto led_US_Heat = Pin_Wag(e_US_Heat, HIGH);
auto led_US_Cool = Pin_Wag(e_US_Cool, HIGH);
auto led_DS_Heat = Pin_Wag(e_DS_Heat, HIGH);
auto led_DS_Cool = Pin_Wag(e_DS_Cool, HIGH);

I2C_Recover i2c_recover(i2C());

auto ds_TempSens = TempSensor(i2c_recover, e_DS_TempAddr); // addr only used when version-changed
auto us_TempSens = TempSensor(i2c_recover, e_US_TempAddr);

Mix_Valve  mixValve[noOfMixers] = {
	{ ds_TempSens, ds_heatRelay, ds_coolRelay, eeprom(), 0, 55 }
	, {us_TempSens, us_heatRelay, us_coolRelay, eeprom(), Mix_Valve::reg_size, 55}
};

void setup() {
	set_watchdog_timeout_mS(8000);
	Serial.begin(SERIAL_RATE);
	Serial.println("Start");
	psu_enable.begin(true);
	logger() << F("Setup Started") << L_endl;
	role = e_Master;

	i2C().setTimeouts(10000, I2C_Talk::WORKING_STOP_TIMEOUT);
	i2C().setMax_i2cFreq(100000);
	i2C().onReceive(receiveI2C);
	i2C().onRequest(requestEventI2C);
	i2C().begin();

	pinMode(e_Slave_Sense, INPUT);

	if (mixValve[ds_mix].getRegister(Mix_Valve::request_temp) == 0) {
		mixValve[ds_mix].setRegister(Mix_Valve::request_temp, 55);
	}

	if (mixValve[us_mix].getRegister(Mix_Valve::request_temp) == 0) {
		mixValve[us_mix].setRegister(Mix_Valve::request_temp, 55);
	}

	for (int i = 0; i < 10; ++i) { // signal a reset
		led_Status.set();
		delay(50);
		led_Status.clear();
		delay(50);
	}

	roleChanged(e_Master);
	logger() << F("Setup complete") << L_endl;
	delay(500);
	//psu_enable.clear();
}

// Called when data is sent by Master, telling slave how many bytes have been sent.
void receiveI2C(int howMany) {  
	// must not do a Serial.print when it receives a register address prior to a read.
	//	delayMicroseconds(1000); // for testing only
	uint8_t msgFromMaster[3]; // = [register, data-0, data-1]
	if (howMany > 3) howMany = 3;
	int noOfBytesReceived = i2C().receiveFromMaster(howMany, msgFromMaster);
	if (noOfBytesReceived) {
		regSet = msgFromMaster[0] / 16;
		if (regSet >= noOfMixers) regSet = 0;
		reg = msgFromMaster[0] % 16;
		if (reg >= Mix_Valve::reg_size) reg = 0;
		// If howMany > 1, we are receiving data, otherwise Master has selected a register ready to read from.
		// All writable registers are single-byte, so just read first byte sent.
		if (howMany > 1) mixValve[regSet].setRegister(reg, msgFromMaster[1]); 
	}
}

// Called when data is requested.
// The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
// To keep things simple, we will send a max of two-bytes.
// Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
// But I2C devices use big-endianness: MSB at the smallest address: So a uint16_t is [MSB, LSB].
// A single read returns the MSB of a 2-byte value (as in a Temp Sensor), 2-byte read is required to get the LSB.
// To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most. 
// getRegister() returns uint8_t as uint16_t which puts LSB at the lower address, which is what we want!
// We can send the address of the returned value to i2C().write().
// 
void requestEventI2C() { 
//	delayMicroseconds(1000); // for testing only
	uint16_t regVal = mixValve[regSet].getRegister(reg);
	uint8_t * valPtr = (uint8_t *)&regVal;
	// Mega OK with logging, but DUE master is not!
	//logger() << F("requestEventI2C: mixV[") << int(regSet) << F("] Reg: ") << int(reg) << F(" Sent:") << regVal << F(" [") << *(valPtr++) << F(",") << *(valPtr) << F("]") << L_endl;
	i2C().write((uint8_t*)&regVal, 2);
}

void loop() {
	static auto nextSecond = Timer_mS(1000);
	static Mix_Valve::Error err = Mix_Valve::e_OK;

	if (nextSecond) { // once per second
		nextSecond.repeat();
		const auto newRole = getRole();
		if (newRole != role) roleChanged(newRole); // Interrupt detection is not reliable!
		err = Mix_Valve::e_OK;

		if (role == e_Master) { // not allowed to query the I2C lines in slave mode
			if (ds_TempSens.getStatus() != _OK) {
				logger() << F("DS TS at 0x") << L_hex << ds_TempSens.getAddress() << L_endl;
				err = Mix_Valve::e_DS_Temp_failed;
			} else {
				ds_TempSens.readTemperature();
				mixValve[ds_mix].setRegister(Mix_Valve::flow_temp, ds_TempSens.get_temp());
			}
			if (us_TempSens.getStatus() != _OK) {
				logger() << F("US TS at 0x") << L_hex << us_TempSens.getAddress() << L_endl;
				err = static_cast<Mix_Valve::Error>(err | Mix_Valve::e_US_Temp_failed);
			}
			if (us_TempSens.lastError() == _BusReleaseTimeout) {
				logger() << F("BusReleaseTimeout") << L_endl;
				err = static_cast<Mix_Valve::Error>(err | Mix_Valve::e_I2C_failed);
			} else {
				us_TempSens.readTemperature();
				mixValve[us_mix].setRegister(Mix_Valve::flow_temp, us_TempSens.get_temp());
			}
		}

		if (err) {
			logger() << showErr(err) << L_endl;
		} else {
			reset_watchdog();
			mixValve[ds_mix].check_flow_temp();
			mixValve[us_mix].check_flow_temp();
		}
	} else showErr(err);
}

void enableRelays(bool enable) {
	if (role == e_Slave) {
		psu_enable.set(enable);
	}
}

Role getRole() {
	return digitalRead(e_Slave_Sense) ? e_Slave : e_Master;
}

void roleChanged(Role newRole) {
	if (e_Slave == newRole) {
		psu_enable.clear();
		i2C().setAsSlave(MIX_VALVE_I2C_ADDR);
		logger() << F("Set to Slave") << L_endl;
	}
	else if (role != newRole) { // changed to Master
		logger() << F("Set to Master - trigger PSU-Restart") << L_endl;
		i2C().setAsMaster(MIX_VALVE_I2C_ADDR);
		mixValve[ds_mix].setRequestTemp();
		mixValve[us_mix].setRequestTemp();
		psu_enable.clear();
	}
	role = newRole;
	logger() << F("DS Request: ") << mixValve[ds_mix].getRegister(Mix_Valve::request_temp) / 256 << L_endl;
	logger() << F("US Request: ") << mixValve[us_mix].getRegister(Mix_Valve::request_temp) / 256 << L_endl;
}

const __FlashStringHelper * showErr(Mix_Valve::Error err) { // 1-4 errors, 5-9 status
	// 5 short flashes == start-up.
	// 1s on / 1s off == Programmer Action
	// 3s on / 1s off == Manual Action
	// 1 flash / 1s off == DS TS failed
	// 2 flash / 1s off == US TS failed
	// 3 flash / 1s off == Both TS failed
	static int lastStatus;
	static int flashCount = 0;

	unsigned long statusLed = millis() / 250UL; // quarter-second count
	int statusOn = statusLed % 2; // changes every 1/4 second
	if (lastStatus != statusOn) {
		lastStatus = statusOn;

		if (err == Mix_Valve::e_OK) {
			statusLed /= 4; // 1111,2222,3333,4444 changes every second
			statusOn = (statusLed % 4);// 0-1-2-3, 0-1-2-3 : on three-in-four seconds
			if (role == e_Slave) {
				statusOn = statusOn % 2; // 0-1, 0-1 : on two-in-four seconds
			}
			led_Status.set(statusOn);
		}
		else {
			static auto pause1S = Timer_mS(1000);
			if (flashCount == 0) {
				pause1S.restart(); // don't flash for 1 sec
				flashCount = err * 2;
				led_Status.clear();
			}
			else if (pause1S) {
				--flashCount;
				led_Status.set(statusOn);
			}
		}
	}

	switch (err) {
	case Mix_Valve::e_DS_Temp_failed:
		ds_heatRelay.clear();
		ds_coolRelay.clear();
		return F("DS TS Failed");
	case Mix_Valve::e_US_Temp_failed:
		us_heatRelay.clear();
		us_coolRelay.clear();
		return F("US TS Failed");
	case Mix_Valve::e_I2C_failed:
		i2C().begin();
		// fall-through
	case Mix_Valve::e_Both_TS_Failed:
		ds_heatRelay.clear();
		ds_coolRelay.clear();
		us_heatRelay.clear();
		us_coolRelay.clear();
		return F("US/DS TS Failed");
	default:
		return F("");
	}
}