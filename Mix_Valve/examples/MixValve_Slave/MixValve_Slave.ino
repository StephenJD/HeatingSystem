// This is the Multi-Master Arduino Mini Controller

//#include <MemoryFree.h>
#include <Mix_Valve.h>
//#include <Mix_Valve\src\PSU\PSU.h>
#include <PSU.h>
#include <PID_Controller.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_Device.h>
#include <Flag_Enum.h>
//#include <I2C_Recover.h>
#include <I2C_RecoverRetest.h>
#include <TempSensor.h>
#include <PinObject.h>
#include <Logging.h>
#include <EEPROM_RE.h>
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

// #define DO_PID_TEST

using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
using namespace flag_enum;

constexpr uint32_t SERIAL_RATE = 115200;
constexpr auto R_SLAVE_REQUESTING_INITIALISATION = 0;
constexpr auto MV_REQUESTING_INI = 1;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_allwaysFlush);
		return _log;
	}
}
using namespace arduino_logger;

EEPROMClassRE& eeprom() {
  static EEPROMClassRE _eeprom;
  return _eeprom;
}

extern const uint8_t version_month;
extern const uint8_t version_day;
const uint8_t version_month = 11; // change to force re-application of defaults incl temp-sensor addresses
const uint8_t version_day = 24;

enum { e_PSU = 6, e_Slave_Sense = 7 };

enum { e_US_Heat = 11, e_US_Cool = 10, e_DS_Heat = 12, e_DS_Cool = A0, e_Status = 13 };

enum Role { e_Master, e_Slave }; // controls PSU enable
Role role = e_Slave; // trigger role-change

I2C_Talk& i2C() {
  static I2C_Talk _i2C{};
  return _i2C;
}

I2C_Recover_Retest i2c_recover(i2C());
//I2C_Recover i2c_recover(i2C());
I_I2Cdevice_Recovery programmer = {i2c_recover, PROGRAMMER_I2C_ADDR };

void roleChanged(Role newRole);
Role getRole();
const __FlashStringHelper * showErr(Mix_Valve::I2C_Flags_Obj status);

auto us_heatRelay = Pin_Wag(e_US_Heat, HIGH);
auto us_coolRelay = Pin_Wag(e_US_Cool, HIGH);
auto ds_heatRelay = Pin_Wag(e_DS_Heat, HIGH);
auto ds_coolRelay = Pin_Wag(e_DS_Cool, HIGH);

auto led_Status = Pin_Wag(e_Status, HIGH);
auto led_US_Heat = Pin_Wag(e_US_Heat, HIGH);
auto led_US_Cool = Pin_Wag(e_US_Cool, HIGH);
auto led_DS_Heat = Pin_Wag(e_DS_Heat, HIGH);
auto led_DS_Cool = Pin_Wag(e_DS_Cool, HIGH);


// All I2C transfers are initiated by Master
auto mixV_register_set = i2c_registers::Registers<Mix_Valve::MV_ALL_REG_SIZE * NO_OF_MIXERS>{i2C()};
i2c_registers::I_Registers& mixV_registers = mixV_register_set;
PowerSupply psu;
Mix_Valve  mixValve_US{ i2c_recover, US_FLOW_TEMPSENS_ADDR, us_heatRelay, us_coolRelay, eeprom(), 0 };
Mix_Valve  mixValve_DS{ i2c_recover, DS_FLOW_TEMPSENS_ADDR, ds_heatRelay, ds_coolRelay, eeprom(), Mix_Valve::MV_ALL_REG_SIZE };
PID_Controller pid_US{ 0,150, 25 * 256, 256 / 2,256 };
PID_Controller pid_DS{ 0,140, 25 * 256, 256 / 2,256 };

auto nextSecond = Timer_mS(1000);

void setup() {
	set_watchdog_timeout_mS(8000);
	logger().begin(SERIAL_RATE);
	logger() << F("MV\tReq\tIs\tState\tReqPos\tIsPos\tPID\tOut\tp\ti\td\tSetT\tErrT\tKp/deg\tKd") << L_flush;
	psu.begin();
	role = e_Master; // prevent PSU-Turn-off

	i2C().setAsMaster(MIX_VALVE_I2C_ADDR);
	i2C().setTimeouts(WORKING_SLAVE_BYTE_PROCESS_TIMOUT_uS, I2C_Talk::WORKING_STOP_TIMEOUT, 10000);
	i2C().setMax_i2cFreq(100000);
	i2C().begin();

	pinMode(e_Slave_Sense, INPUT);

	for (int i = 0; i < 10; ++i) { // signal a reset
		led_Status.set();
		delay(50);
		led_Status.clear();
		delay(50);
	}

	mixValve_US.begin(55); // does speed-test for TS
	mixValve_DS.begin(55);
	auto speedTest = I2C_SpeedTest{ programmer };
	speedTest.fastest();
	i2C().setTimeouts(WORKING_SLAVE_BYTE_PROCESS_TIMOUT_uS, I2C_Talk::WORKING_STOP_TIMEOUT, 10000);
	delay(500);
	roleChanged(getRole());
	i2C().onReceive(mixV_register_set.receiveI2C);
	i2C().onRequest(mixV_register_set.requestI2C);


	logger() << F("Setup complete.") << L_flush;
}

void loop() {
	// All I2C transfers are initiated by Master
	static auto err = Mix_Valve::I2C_Flags_Obj{};
	static bool multimaster_mode;
	err.set(Mix_Valve::F_US_TS_FAILED, !mixValve_US.doneI2C_Coms(programmer, nextSecond));
	err.set(Mix_Valve::F_DS_TS_FAILED, !mixValve_DS.doneI2C_Coms(programmer, nextSecond));
	
	auto flowTemp_US = mixValve_US.update(pid_US.currOut());
	auto flowTemp_DS = mixValve_DS.update(pid_DS.currOut());

	if (nextSecond) { // once per second
		nextSecond.repeat();
		reset_watchdog();
		const auto newRole = getRole();
		if (newRole != role) roleChanged(newRole); // Interrupt detection is not reliable!
		
		pid_US.checkSetpoint(mixValve_US.currReqTemp_16());
		pid_US.adjust(flowTemp_US, mixValve_US.atTarget());
		mixValve_US.setRegister(Mix_Valve::R_PID_MODE, pid_US.mode());
		mixValve_US.setRegister(Mix_Valve::R_TS_ERR, pid_US.currSetPoint() / 256);
		mixValve_US.log();
		pid_US.log();
		logger() << L_endl;


		//if (mixValve_DS.mode() != Mix_Valve::e_WaitingToMove) {
			pid_DS.checkSetpoint(mixValve_DS.currReqTemp_16());
			pid_DS.adjust(flowTemp_DS, mixValve_DS.atTarget());
			mixValve_DS.setRegister(Mix_Valve::R_PID_MODE, pid_DS.mode());
			mixValve_DS.setRegister(Mix_Valve::R_TS_ERR, pid_DS.currSetPoint()/256);
			mixValve_DS.log();
			pid_DS.log();
			logger() << L_endl;
		//}

		if (err.flags()) {
			logger() << showErr(err) << L_endl;
		}
	} else showErr(err);
}

//bool relaysOn() {
//	return psu_enable.logicalState();
//}

//void enableRelays(bool enable) { // called by MixValve.cpp
//	if (role == e_Slave) {
//		psu_enable.set(enable);
//	}
//}

Role getRole() {
	return digitalRead(e_Slave_Sense) ? e_Slave : e_Master;
}

void roleChanged(Role newRole) {
	if (e_Slave == newRole) {
		psu.keepPSU_on(false);
		for (int i = 0; i < 10; ++i) {
			if (programmer.write(R_SLAVE_REQUESTING_INITIALISATION, MV_REQUESTING_INI) == _OK) break;
		} 
		logger() << F("Set to Slave") << L_endl;
	}
	else { // changed to Master
		psu.keepPSU_on(true);
		logger() << F("Set to Master - trigger PSU-Restart") << L_endl;
		mixValve_US.setDefaultRequestTemp();
		mixValve_DS.setDefaultRequestTemp();
	}
	role = newRole;
}

const __FlashStringHelper * showErr(Mix_Valve::I2C_Flags_Obj status) { // 1-4 errors, 5-9 status
	// 5 short flashes == start-up.
	// 1s on / 1s off == Programmer Action
	// 3s on / 1s off == Manual Action
	// 1 flash / 1s off == DS TS failed
	// 2 flash / 1s off == US TS failed
	// 3 flash / 1s off == Both TS failed
	static int lastStatus;
	static int flashCount = 0;
	auto err = status.is(Mix_Valve::F_DS_TS_FAILED) + 2 * status.is(Mix_Valve::F_US_TS_FAILED);

	unsigned long statusLed = millis() / 250UL; // quarter-second count
	int statusOn = statusLed % 2; // changes every 1/4 second
	if (lastStatus != statusOn) {
		lastStatus = statusOn;

		if (!err) {
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
	case 1:
		ds_heatRelay.clear();
		ds_coolRelay.clear();
		return F("DS TS Failed");
	case 2:
		us_heatRelay.clear();
		us_coolRelay.clear();
		return F("US TS Failed");	
	case 3:
		us_heatRelay.clear();
		us_coolRelay.clear();		
		ds_heatRelay.clear();
		ds_coolRelay.clear();
		return F("US/DS TS Failed");
	default:
		return F("");
	}
}