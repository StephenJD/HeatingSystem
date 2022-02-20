// Test MixValve.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Mix_Valve.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <I2C_Device.h>
#include <Flag_Enum.h>
#include <I2C_RecoverRetest.h>
#include <TempSensor.h>
#include <PinObject.h>
#include <Logging.h>
#include <EEPROM_RE.h>
#include <Watchdog_Timer.h>
#include <Timer_mS_uS.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>

using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
using namespace flag_enum;
constexpr uint32_t SERIAL_RATE = 115200;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_allwaysFlush);
		return _log;
	}
}
using namespace arduino_logger;

I2C_Talk& i2C() {
    static I2C_Talk _i2C{};
    return _i2C;
}

EEPROMClassRE& eeprom() {
	static EEPROMClassRE _eeprom(EEPROM_I2C_ADDR);
	return _eeprom;
}

I2C_Recover_Retest i2c_recover(i2C());
I_I2Cdevice_Recovery programmer = { i2c_recover, PROGRAMMER_I2C_ADDR };

enum { e_US_Heat = 11, e_US_Cool = 10, e_DS_Heat = 12, e_DS_Cool = A0, e_Status = 13 };
enum { e_PSU = 6, e_Slave_Sense = 7 };
extern const uint8_t version_month;
extern const uint8_t version_day;
const uint8_t version_month = 2; // change to force re-application of defaults incl temp-sensor addresses
const uint8_t version_day = 9;

auto us_heatRelay = Pin_Wag(e_US_Heat, HIGH);
auto us_coolRelay = Pin_Wag(e_US_Cool, HIGH);
auto psu_enable = Pin_Wag(e_PSU, LOW, true);

Mix_Valve  mixValve = { i2c_recover, US_FLOW_TEMPSENS_ADDR, us_heatRelay, us_coolRelay, eeprom(), 0 };
auto mixV_register_set = i2c_registers::Registers<Mix_Valve::MV_ALL_REG_SIZE* NO_OF_MIXERS, MIX_VALVE_I2C_ADDR>{ i2C() };
i2c_registers::I_Registers& mixV_registers = mixV_register_set;

int main() {
	logger().begin(SERIAL_RATE);
	logger() << F("Setup Started") << L_flush;
	psu_enable.begin(true);

	i2C().setAsMaster(MIX_VALVE_I2C_ADDR);
	i2C().begin();
	i2C().onReceive(mixV_register_set.receiveI2C);
	i2C().onRequest(mixV_register_set.requestI2C);

	mixValve.begin(55); // does speed-test for TS
	mixValve.doneI2C_Coms(programmer, true);

	mixValve.check_flow_temp();
}

bool relaysOn() {
	return psu_enable.logicalState();
}

void enableRelays(bool enable) { // called by MixValve.cpp
		psu_enable.set(enable);
}