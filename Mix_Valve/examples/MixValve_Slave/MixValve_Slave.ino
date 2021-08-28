//#include <MemoryFree.h>
#include <Mix_Valve.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h>
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
// Otherwise, depending on how cleanly the Arduino shuts down, it may time-out before it is reset.
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

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_startWithFlushing);
		return _log;
	}
}
using namespace arduino_logger;

EEPROMClass & eeprom() {
	return EEPROM;
}

constexpr auto slave_requesting_initialisation = 0;

extern const uint8_t version_month;
extern const uint8_t version_day;
extern const uint8_t eeprom_firstRegister_addr;
const uint8_t version_month = 8; // change to force re-application of defaults incl temp-sensor addresses
const uint8_t version_day = 25;
const uint8_t eeprom_firstRegister_addr = sizeof(version_month) + sizeof(version_day);

enum { e_PSU = 6, e_Slave_Sense = 7 };
enum { ds_mix, us_mix };

enum { e_US_Heat = 11, e_US_Cool = 10, e_DS_Heat = 12, e_DS_Cool = A0, e_Status = 13 };

enum Role { e_Slave, e_Master }; // controls PSU enable
Role role = e_Slave;

I2C_Talk& i2C() {
  static I2C_Talk _i2C{};
  return _i2C;
}

void roleChanged(Role newRole);
Role getRole();
const __FlashStringHelper * showErr(Mix_Valve::Error err);

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

// register[0] is offset for writing to Programmer
constexpr int mix_register_offset = 0;
auto mixV_register_set = i2c_registers::Registers<1 + Mix_Valve::mixValve_all_register_size * NO_OF_MIXERS>{i2C()};
i2c_registers::I_Registers& mixV_registers = mixV_register_set;

auto ds_TempSens = TempSensor(i2c_recover, DS_TEMPSENS_ADDR); // addr only used when version-changed
auto us_TempSens = TempSensor(i2c_recover, US_TEMPSENS_ADDR);

Mix_Valve  mixValve[] = {
	{i2c_recover, ds_TempSens, ds_heatRelay, ds_coolRelay, eeprom(), 1}
	, {i2c_recover, us_TempSens, us_heatRelay, us_coolRelay, eeprom(), 1 + Mix_Valve::mixValve_all_register_size}
	//{i2c_recover, DS_TEMPSENS_ADDR, ds_heatRelay, ds_coolRelay, eeprom(), 1}
	//, {i2c_recover, US_TEMPSENS_ADDR, us_heatRelay, us_coolRelay, eeprom(), 1 + Mix_Valve::mixValve_all_register_size}
};

void setup() {
	logger() << F("Setup Started") << L_endl;
	//set_watchdog_timeout_mS(8000);
	//Serial.begin(SERIAL_RATE);
	//Serial.println("Start");
	mixValve[0].begin(55);
	mixValve[1].begin(55);
	psu_enable.begin(true);

	i2C().setAsMaster(MIX_VALVE_I2C_ADDR);
	i2C().setTimeouts(10000, I2C_Talk::WORKING_STOP_TIMEOUT);
	i2C().setMax_i2cFreq(100000);
	i2C().onReceive(mixV_register_set.receiveI2C);
	i2C().onRequest(mixV_register_set.requestI2C);
	i2C().begin();

	pinMode(e_Slave_Sense, INPUT);

	for (int i = 0; i < 10; ++i) { // signal a reset
		led_Status.set();
		delay(50);
		led_Status.clear();
		delay(50);
	}

	role = e_Slave;
	roleChanged(e_Master);
	logger() << F("Setup complete") << L_flush;
    logger() << F("NoFlush") << L_endl;
	delay(500);
	//psu_enable.clear();
}

void loop() {
	// It is up to any connected programmer to initiate sending a new temperature request
	// and to request register-data.
	static auto nextSecond = Timer_mS(1000);
	static Mix_Valve::Error err = Mix_Valve::e_OK;

	if (nextSecond) { // once per second
		nextSecond.repeat();
		reset_watchdog();
		
		const auto newRole = getRole();
		if (newRole != role) roleChanged(newRole); // Interrupt detection is not reliable!

		err = Mix_Valve::e_OK;
		if (mixValve[ds_mix].check_flow_temp() != Mix_Valve::e_OK) {
			err = Mix_Valve::e_DS_Temp_failed;
		}
		if (mixValve[us_mix].check_flow_temp() != Mix_Valve::e_OK) {
			err = static_cast<Mix_Valve::Error>(err | Mix_Valve::e_US_Temp_failed);
		}

	  logger() << F("DS req: ") << mixV_register_set.getRegister(1+ Mix_Valve::request_temp) << L_endl;
	  logger() << F("US req: ") << mixV_register_set.getRegister(1+ Mix_Valve::mixValve_all_register_size + Mix_Valve::request_temp) << L_endl;

		if (err) {
			logger() << showErr(err) << L_endl;
		}
	} else showErr(err);
}

void enableRelays(bool enable) { // called by MixValve.cpp
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
		while (i2C().write(PROGRAMMER_I2C_ADDR, slave_requesting_initialisation, 1) != _OK);
		logger() << F("Set to Slave") << L_endl;
	}
	else if (role != newRole) { // changed to Master
		logger() << F("Set to Master - trigger PSU-Restart") << L_endl;
		mixValve[ds_mix].setDefaultRequestTemp();
		mixValve[us_mix].setDefaultRequestTemp();
		psu_enable.clear();
	}
	role = newRole;
	logger() << F("DS Request: ") << mixValve[ds_mix].getReg(Mix_Valve::request_temp) / 256 << L_endl;
	logger() << F("US Request: ") << mixValve[us_mix].getReg(Mix_Valve::request_temp) / 256 << L_endl;
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