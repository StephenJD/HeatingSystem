// Test MixValve.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <catch.hpp>
#include <Arduino.h>
#include <Mix_Valve.h>
#include <I2C_RecoverRetest.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <PinObject.h>

using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
using namespace flag_enum;

I2C_Talk& i2C();

I2C_Recover_Retest i2c_recover(i2C());
I_I2Cdevice_Recovery programmer = { i2c_recover, PROGRAMMER_I2C_ADDR };

enum { e_US_Heat = 11, e_US_Cool = 10, e_DS_Heat = 12, e_DS_Cool = A0, e_Status = 13 };
extern const uint8_t version_month;
extern const uint8_t version_day;
const uint8_t version_month = 2; // change to force re-application of defaults incl temp-sensor addresses
const uint8_t version_day = 9;


auto mixV_register_set = i2c_registers::Registers<Mix_Valve::MV_ALL_REG_SIZE* NO_OF_MIXERS, MIX_VALVE_I2C_ADDR>{ i2C() };
i2c_registers::I_Registers& mixV_registers = mixV_register_set;

auto us_heatRelay = Pin_Wag(e_US_Heat, HIGH);
auto us_coolRelay = Pin_Wag(e_US_Cool, HIGH);
auto ds_heatRelay = Pin_Wag(e_DS_Heat, HIGH);
auto ds_coolRelay = Pin_Wag(e_DS_Cool, HIGH);

class TestMixV {
public:
	TestMixV();
	void setOnTime(int mixInd, int ontime) { mixValve[mixInd]._onTime = ontime; }
	void setVPos(int mixInd, int vpos) { mixValve[mixInd]._valvePos = vpos; }
	void setMode(int mixInd, Mix_Valve::Mode mode) { mixValve[mixInd].registers().set(Mix_Valve::R_MODE, mode); }
	void setReqTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp); }
	void setIsTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_FLOW_TEMP, temp); }
	void checkTemp(int mixInd) { mixValve[mixInd].check_flow_temp(); }
	int reqTemp(int mixInd) { return  mixValve[mixInd]._currReqTemp; }
	int onTime(int mixInd) { return  mixValve[mixInd]._onTime; }
	int vPos(int mixInd) { return  mixValve[mixInd]._valvePos; }
	int mode(int mixInd) { return  mixValve[mixInd].registers().get(Mix_Valve::R_MODE); }
	void off(int index) { mixValve[index].turnValveOff(); }

	void setToCool(int mixV, int pos, int onTime) {
		setVPos(mixV, pos);
		setMode(mixV, Mix_Valve::e_Checking);
		setIsTemp(mixV, 35);
		setReqTemp(mixV, 30);
		checkTemp(mixV); // register req temp
		setReqTemp(mixV, 30);
		setOnTime(mixV, 0);
		checkTemp(mixV); // accept new temp
		// Set to cool
		checkTemp(mixV);
		// Reset on-time
		setOnTime(mixV, onTime);
	}

private:
	Mix_Valve mixValve[2]; 
};

TestMixV::TestMixV() : mixValve{
		{i2c_recover, US_FLOW_TEMPSENS_ADDR, us_heatRelay, us_coolRelay, eeprom(), 0}
	  , {i2c_recover, DS_FLOW_TEMPSENS_ADDR, ds_heatRelay, ds_coolRelay, eeprom(), Mix_Valve::MV_ALL_REG_SIZE}
} {
	mixValve[0].begin(55); // does speed-test for TS
	mixValve[1].begin(55);
	Mix_Valve::motor_mutex = 0;
	Mix_Valve::motor_queued = false;
}
//	enum Mode {e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff, e_Error };

TEST_CASE("MutexSwap", "[MixValve]") {
	TestMixV testMV;
	testMV.setOnTime(0, 20);
	testMV.setOnTime(1, 20);
	testMV.setVPos(0, 30);
	testMV.setVPos(1, 30);
	testMV.checkTemp(0);
	testMV.checkTemp(1);
	// US Starts with Move
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.onTime(0) == 511);
	CHECK(testMV.onTime(1) == 511);

	// DS gets move at US 510
	testMV.checkTemp(0);
	CHECK(testMV.onTime(0) == 510);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	testMV.checkTemp(1);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);

	testMV.checkTemp(0);
	testMV.checkTemp(1);
	CHECK(testMV.onTime(0) == 510);
	CHECK(testMV.onTime(1) == 510);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	
	// US gets move at DS 510
	testMV.checkTemp(0);
	testMV.checkTemp(1);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.onTime(0) == 510);
	CHECK(testMV.onTime(1) == 510);

	testMV.checkTemp(0); 
	testMV.checkTemp(1);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.onTime(0) == 509);
	CHECK(testMV.onTime(1) == 510);

	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.onTime(0) == 500);
	CHECK(testMV.onTime(1) == 510);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	
	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);

	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.onTime(0) == 500);
	CHECK(testMV.onTime(1) == 500);
	CHECK(testMV.vPos(0) == 19);
	CHECK(testMV.vPos(1) == 19);

	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.onTime(0) == 490);
	CHECK(testMV.onTime(1) == 500);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 9);
	CHECK(testMV.vPos(1) == 19);
}

TEST_CASE("RetainMutexAfterOtherValveStopped", "[MixValve]") {
//	enum Mode {e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff, e_Error };
	TestMixV testMV;
	testMV.setToCool(0,60,40);
	testMV.setToCool(1,60,10);

	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);

	// Move [0] -10
	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(testMV.onTime(0) == 30);
	CHECK(testMV.onTime(1) == 10);
	CHECK(testMV.vPos(0) == 50);
	CHECK(testMV.vPos(1) == 60);
	
	// Move [1] -10
	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);

	CHECK(testMV.onTime(0) == 30);
	CHECK(testMV.onTime(1) == -40);
	CHECK(testMV.vPos(0) == 50);
	CHECK(testMV.vPos(1) == 50);

	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Wait);

	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.onTime(0) == -40);
	CHECK(testMV.onTime(1) == -10);
	CHECK(testMV.mode(0) == Mix_Valve::e_Wait);
	CHECK(testMV.mode(1) == Mix_Valve::e_Wait);
	CHECK(testMV.vPos(0) ==	20);
	CHECK(testMV.vPos(1) == 50);
}

TEST_CASE("FindOff", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0, 60);
	CHECK(testMV.mode(0) == Mix_Valve::e_FindOff);
	testMV.checkTemp(0);
	CHECK(testMV.onTime(0) == 511);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 60);
	do {
		testMV.setReqTemp(0,40); // try interrupting findOff
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.onTime(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);

	// Register new temp
	CHECK(testMV.reqTemp(0) == 0);
	testMV.checkTemp(0);
	CHECK(testMV.reqTemp(0) == 0);
	testMV.setReqTemp(0,40);
	testMV.checkTemp(0);
	CHECK(testMV.reqTemp(0) == 40);
}

TEST_CASE("Move-Wait-Check-Move-Off", "[MixValve]") {
	TestMixV testMV;
	testMV.setToCool(0, 60, 20);

	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 40);
	CHECK(testMV.onTime(0) == -40);
	CHECK(testMV.mode(0) == Mix_Valve::e_Wait);
	
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Wait);
	CHECK(testMV.vPos(0) == 40);
	CHECK(testMV.onTime(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
	testMV.checkTemp(0);
	CHECK(testMV.onTime(0) == 150);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.onTime(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
}

TEST_CASE("Limit on Heat and Recover", "[MixValve]") {

}

TEST_CASE("Off on Cool and Recover", "[MixValve]") {

}

TEST_CASE("Cool-OK-Heat", "[MixValve]") {

}

TEST_CASE("Heat-OK-Cool", "[MixValve]") {

}


