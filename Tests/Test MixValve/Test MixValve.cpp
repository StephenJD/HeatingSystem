// Test MixValve.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define SIM_MIXV

#include <catch.hpp>
#include <Arduino.h>
#include <Mix_Valve.h>
#include <I2C_RecoverRetest.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <PinObject.h>
#include <PID_Controller.h>
#define  WITHOUT_NUMPY
#include "matplotlibcpp.h"


using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
using namespace flag_enum;
namespace plt = matplotlibcpp;

constexpr uint16_t TS_TIME_CONST = 10;
constexpr uint16_t TS_DELAY = 20;

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
	void setMaxTemp(int mixInd, int max) { mixValve[mixInd].set_maxTemp(max); }
	void setVPos(int mixInd, int vpos) { mixValve[mixInd]._valvePos = vpos; }
	void setMode(int mixInd, Mix_Valve::Mode mode) { mixValve[mixInd].registers().set(Mix_Valve::R_MODE, mode); }
	void setReqTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp); }
	void setIsTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_FLOW_TEMP, temp); }
	void update(int mixInd, int pos) { mixValve[mixInd].update(pos); }
	void off(int index) { mixValve[index].turnValveOff(); }
	int reqTemp(int mixInd) { return  mixValve[mixInd]._currReqTemp; }
	int vPos(int mixInd) { return  mixValve[mixInd]._valvePos; }
	int mode(int mixInd) { return  mixValve[mixInd].registers().get(Mix_Valve::R_MODE); }
	int direction(int mixInd) {	return mixValve[mixInd]._motorDirection; }
	uint8_t status(int mixInd) { return  mixValve[mixInd].registers().get(Mix_Valve::R_DEVICE_STATE); }
	//void simMV_temp(int mixInd) { mixValve[mixInd].simulateFlowTemp(); }
	//void logPID(int mixInd) { mixValve[mixInd].logPID(); }
	void setToCool_30Deg(int mixV, int pos, int onTime) {
		setVPos(mixV, pos);
		setIsTemp(mixV, 35);
		registerReqTemp(mixV, 30);
		// Set to cool
		//checkTemp(mixV);
	}
	void registerReqTemp(int mixV, int temp) {
		setMode(mixV, Mix_Valve::e_AtTargetPosition);
		setReqTemp(mixV, temp);
		//checkTemp(mixV); // register req temp
		setReqTemp(mixV, temp);
		//checkTemp(mixV); // register req temp
	}

private:
	Mix_Valve mixValve[2];
	//int simFlowT(int mixV) { return tankTemp * vPos(mixV) / 150; }
	//int tankTemp;
};

TestMixV::TestMixV() : mixValve{
		{i2c_recover, US_FLOW_TEMPSENS_ADDR, us_heatRelay, us_coolRelay, eeprom(), 0,TS_TIME_CONST, TS_DELAY, 70}
	  , {i2c_recover, DS_FLOW_TEMPSENS_ADDR, ds_heatRelay, ds_coolRelay, eeprom(), Mix_Valve::MV_ALL_REG_SIZE, TS_TIME_CONST, TS_DELAY, 40}
} {
	mixValve[0].begin(55); // does speed-test for TS
	mixValve[1].begin(55);
	Mix_Valve::motor_mutex = 0;
	Mix_Valve::motor_queued = false;
}

TEST_CASE("Plot", "[MixValve]") {
	logger() << "Plot..." << L_endl;
	//plt::plot({ 1,3,2,4 });
	//plt::show();
	//TestMixV testMV;
	//CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
}


TEST_CASE("Find Off", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0, 30);
	do {
		testMV.update(0,35);
	} while (testMV.mode(0) == Mix_Valve::e_FindingOff);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
}


TEST_CASE("MutexSwap", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0, 0);
	testMV.setVPos(1, 0);
	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(0) != Mix_Valve::e_Moving);
	// US Starts with Move
	CHECK(testMV.mode(1) == Mix_Valve::e_WaitingToMove);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.vPos(1) == 0);

	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.vPos(0) < 10);

	CHECK(testMV.vPos(1) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_WaitingToMove);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	
	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.vPos(1) < 10);

	CHECK(testMV.vPos(0) == 10);

	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(0) != Mix_Valve::e_AtTargetPosition);

	CHECK(testMV.vPos(0) == 35);
	CHECK(testMV.vPos(1) == 30);
	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(1) != Mix_Valve::e_AtTargetPosition);
	CHECK(testMV.vPos(0) == 35);
	CHECK(testMV.vPos(1) == 35);
}

TEST_CASE("RetainMutexAfterOtherValveStopped", "[MixValve]") {
	// e_AtTargetPosition, e_Moving, e_WaitingToMove, e_HotLimit, e_ValveOff, e_FindingOff, e_WaitToCool
	TestMixV testMV;
	// Move [0] -> 10
	testMV.setVPos(0, 0);
	testMV.setVPos(1, 0);
	do {
		testMV.update(0, 35);
		testMV.update(1, 5);
	} while (testMV.vPos(0) < 10);

	CHECK(testMV.vPos(1) == 0);

	// Move [1] -> 5
	do {
		testMV.update(0, 35);
		testMV.update(1, 5);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 10);
	CHECK(testMV.vPos(1) == 5);

	do {
		testMV.update(0, 35);
		testMV.update(1, 5);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 35);
	CHECK(testMV.vPos(1) == 5);
}

TEST_CASE("FindOff", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0, 10);
	testMV.update(0, 35);
	CHECK(testMV.mode(0) == Mix_Valve::e_FindingOff);

	do {
		testMV.setReqTemp(0,40); // try interrupting findOff
		testMV.update(0, 35);
	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
	CHECK(testMV.vPos(0) == 0);

	// Register new temp
	CHECK(testMV.reqTemp(0) == 40);
	// Interrupt move with low-temp request
	do {
		testMV.update(0, 35);
		if (testMV.vPos(0) > 10) testMV.setReqTemp(0, 25); // move off
	} while (testMV.mode(0) != Mix_Valve::e_FindingOff);
	CHECK(testMV.vPos(0) == 12);
}

TEST_CASE("Limit on Heat then OK then too hot", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0,0);
	testMV.setIsTemp(0, 40);
	testMV.setMaxTemp(0, 50);
	do {
		testMV.setReqTemp(0, 55);
		testMV.update(0, 150);
	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
	CHECK(testMV.vPos(0) == VALVE_TRANSIT_TIME);
	
	for (int i = 0; i < 10; ++i) {
		testMV.update(0, 150);
		CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
	}
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == true);

	testMV.setMaxTemp(0, 70);
	for (int i = 0; i < 10; ++i) {
		testMV.update(0, 150);
		CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	}
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);

	do {
		testMV.update(0, 100);
	} while (testMV.mode(0) != Mix_Valve::e_AtTargetPosition);
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
}

/*
TEST_CASE("Limit on Heat then ReduceRequest", "[MixValve]") {
	TestMixV testMV;
	testMV.setToCool_30Deg(0, 60, 20);
	testMV.setReqTemp(0, 40);
	testMV.checkTemp(0);
	testMV.setReqTemp(0, 40);
	testMV.checkTemp(0);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
	CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
	CHECK(testMV.vPos(0) == VALVE_TRANSIT_TIME);
	for (int i = 0; i < 10; ++i) {
		testMV.checkTemp(0);
		CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
	}
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == true);
	testMV.setReqTemp(0, 29);
	testMV.checkTemp(0);
	testMV.setReqTemp(0, 29);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	testMV.checkTemp(0);
	for (int i = 0; i < 10; ++i) {
		testMV.checkTemp(0);
		CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	}
	testMV.setIsTemp(0, 29);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_WaitingToMove);
	testMV.checkTemp(0);
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
}


TEST_CASE("Limit on Heat then Off", "[MixValve]") {
	TestMixV testMV;
	testMV.setToCool_30Deg(0, 60, 20);
	testMV.setReqTemp(0, 40);
	testMV.checkTemp(0);
	testMV.setReqTemp(0, 40);
	testMV.checkTemp(0);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
	CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
	CHECK(testMV.vPos(0) == VALVE_TRANSIT_TIME);
	for (int i = 0; i < 10; ++i) {
		testMV.checkTemp(0);
		CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
	}
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == true);
	testMV.setReqTemp(0, 20);
	testMV.checkTemp(0);
	testMV.setReqTemp(0, 20);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
}


TEST_CASE("Off on Cool and Recover", "[MixValve]") {
	TestMixV testMV;
	testMV.setToCool_30Deg(0, 60, 20);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
	CHECK(testMV.vPos(0) == 0);
	for (int i = 0; i < 10; ++i) {
		testMV.checkTemp(0);
		CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
	}
	testMV.setIsTemp(0, 30);
	for (int i = 0; i < 10; ++i) {
		testMV.checkTemp(0);
		CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
	}
	testMV.setIsTemp(0, 29);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_AtTargetPosition);
	testMV.checkTemp(0);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_AtTargetPosition);
	testMV.setIsTemp(0, 30);
	for (int i = 0; i < 10; ++i) {
		testMV.checkTemp(0);
		CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	}
}


TEST_CASE("Cool-Heat-Cool", "[MixValve]") {
	TestMixV testMV;
	testMV.setToCool_30Deg(0, 60, 20);
	auto flowtemp = 32.0;
	testMV.setIsTemp(0, flowtemp);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.journey(0) == Mix_Valve::e_CoolNorth);
	do {
		flowtemp -= 0.1;
		testMV.setIsTemp(0, flowtemp);
		testMV.checkTemp(0);
	} while (testMV.journey(0) == Mix_Valve::e_CoolNorth);
	CHECK(testMV.journey(0) == Mix_Valve::e_WarmSouth);
	
	flowtemp = 27.0;
	testMV.setIsTemp(0, flowtemp);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	do {
		flowtemp += 0.1;
		testMV.setIsTemp(0, flowtemp);
		testMV.checkTemp(0);
	} while (testMV.journey(0) == Mix_Valve::e_WarmSouth);
	CHECK(testMV.journey(0) == Mix_Valve::e_CoolNorth);

	flowtemp = 32.0;
	testMV.setIsTemp(0, flowtemp);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.journey(0) == Mix_Valve::e_CoolNorth);
	do {
		flowtemp -= 0.1;
		testMV.setIsTemp(0, flowtemp);
		testMV.checkTemp(0);
	} while (testMV.journey(0) == Mix_Valve::e_CoolNorth);
	CHECK(testMV.journey(0) == Mix_Valve::e_WarmSouth);
}
*/
