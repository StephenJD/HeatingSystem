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
	// Modifiers
	void setVPos(int mixInd, int vPos) { mixValve[mixInd]._valvePos = vPos; }
	void setMode(int mixInd, Mix_Valve::Mode mode) { mixValve[mixInd].registers().set(Mix_Valve::R_MODE, mode); }
	void setReqTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp); }
	void setIsTemp(int mixInd, double temp) { mixValve[mixInd].registers().set(Mix_Valve::R_FLOW_TEMP, int(temp)); mixValve[mixInd].registers().set(Mix_Valve::R_FLOW_TEMP_FRACT, 0);
	}
	void checkTemp(int mixInd) { mixValve[mixInd].check_flow_temp(); }
	void off(int index) { mixValve[index].turnValveOff(); }
	void setToCool_30Deg(int mixV, int pos, int onTime);
	void autoIsTemp();
	
	// Queries
	double getIsTemp(int mixInd) { return mixValve[mixInd].registers().get(Mix_Valve::R_FLOW_TEMP) + mixValve[mixInd].registers().get(Mix_Valve::R_FLOW_TEMP_FRACT) / 256.; }
	int reqTemp(int mixInd) { return  mixValve[mixInd]._currReqTemp; }
	int vPos(int mixInd) { return  mixValve[mixInd]._valvePos; }
	int mode(int mixInd) { return  mixValve[mixInd].registers().get(Mix_Valve::R_MODE); }
	int status(int mixInd) { return  mixValve[mixInd].registers().get(Mix_Valve::R_DEVICE_STATE); }
	int launchPos(int mixInd) { return  mixValve[mixInd].registers().get(Mix_Valve::R_LAUNCH_POS); }
	int journey(int mixInd) { return  mixValve[mixInd]._journey; }
	int correctPos();
	double correctRatio();
private:
	Mix_Valve mixValve[2];
	uint32_t _simulated_secs = 0;
	uint8_t _simMaxTemp = 55;
	double _sim_timeConst = 50;
	double _sim_flowTemp = 20;
	//int simFlowT(int mixV) { return tankTemp * vPos(mixV) / 150; }
	//int tankTemp;
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

void TestMixV::setToCool_30Deg(int mixV, int pos, int onTime) {
	setVPos(mixV, pos);
	setMode(mixV, Mix_Valve::e_Checking);
	setIsTemp(mixV, 35);
	setReqTemp(mixV, 30);
	checkTemp(mixV); // register req temp
	setReqTemp(mixV, 30);
	checkTemp(mixV); // accept new temp
	// Set to cool
	checkTemp(mixV);
	// Reset on-time
}

void TestMixV::autoIsTemp() {
	++_simulated_secs;
	auto roomTemp = 20;
	auto reg =  mixValve[0].registers();
	auto vRange = reg.get(Mix_Valve::R_HALF_TRAVERSE_TIME) * 2;
	double endTemp = roomTemp + (_simMaxTemp - roomTemp) * vPos(0) / vRange;
	double startTemp = reg.get(Mix_Valve::R_FLOW_TEMP);
	startTemp += reg.get(Mix_Valve::R_FLOW_TEMP_FRACT) / 256.;
	_sim_flowTemp = startTemp + (endTemp - startTemp) * (1 - exp(-(_simulated_secs / _sim_timeConst)));
	int intTemp = int(_sim_flowTemp);
	int fractTemp = int(_sim_flowTemp * 256.) & 0xF0;
	reg.set(Mix_Valve::R_FLOW_TEMP_FRACT, fractTemp);
	if (intTemp != int(startTemp)) {
		reg.set(Mix_Valve::R_FLOW_TEMP, intTemp);
		_simulated_secs = 0;
	}
}

int TestMixV::correctPos() {
	return int(0.5 + (reqTemp(0) - 20) * correctRatio());
}

double TestMixV::correctRatio() {
	auto reg =  mixValve[0].registers();
	auto vRange = reg.get(Mix_Valve::R_HALF_TRAVERSE_TIME) * 2;
	return vRange / (_simMaxTemp - 20.);
}

//	enum Mode {e_newReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff, e_Error };
TEST_CASE("Find Off", "[MixValve][Baics]") {
	TestMixV testMV;
	testMV.setVPos(0, 30);
	do {
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
}

TEST_CASE("MutexSwap", "[MixValve][Baics]") {
	TestMixV testMV;
	testMV.setVPos(0, 31);
	testMV.setVPos(1, 31);
	testMV.checkTemp(0);
	testMV.checkTemp(1);
	// US Starts with Move
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.vPos(0) == 31);
	CHECK(testMV.vPos(1) == 31);

	// DS gets move at US 30
	testMV.checkTemp(0);
	CHECK(testMV.vPos(0) == 30);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	testMV.checkTemp(1);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);

	testMV.checkTemp(0);
	testMV.checkTemp(1);
	CHECK(testMV.vPos(0) == 30);
	CHECK(testMV.vPos(1) == 30);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	
	// US gets move at DS 30
	testMV.checkTemp(0);
	testMV.checkTemp(1);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.vPos(0) == 30);
	CHECK(testMV.vPos(1) == 30);

	testMV.checkTemp(0); 
	testMV.checkTemp(1);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.vPos(0) == 29);
	CHECK(testMV.vPos(1) == 30);

	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.vPos(0) == 20);
	CHECK(testMV.vPos(1) == 30);
	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	
	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);

	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);
	CHECK(testMV.vPos(0) == 19);
	CHECK(testMV.vPos(1) == 19);

	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 9);
	CHECK(testMV.vPos(1) == 19);
}

TEST_CASE("RetainMutexAfterOtherValveStopped", "[MixValve][Baics]") {
//	enum Mode {e_newReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff, e_Error };
	TestMixV testMV;
	testMV.setToCool_30Deg(0,60,40);
	testMV.setToCool_30Deg(1,60,10);

	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_Mutex);

	// Move [0] -10
	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.mode(0) == Mix_Valve::e_Mutex);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 50);
	CHECK(testMV.vPos(1) == 60);
	
	// Move [1] -10
	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);

	CHECK(testMV.vPos(0) == 50);
	CHECK(testMV.vPos(1) == 50);

	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);

	do {
		testMV.checkTemp(0);
		testMV.checkTemp(1);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.vPos(0) ==	20);
	CHECK(testMV.vPos(1) == 50);
}

TEST_CASE("FindOff", "[MixValve][Baics]") {
	TestMixV testMV;
	testMV.setVPos(0, 60);
	testMV.checkTemp(0);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 60);
	do {
		testMV.setReqTemp(0,40); // try interrupting findOff
		testMV.checkTemp(0);
	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);

	// Register new temp
	CHECK(testMV.reqTemp(0) == 0);
	testMV.checkTemp(0);
	CHECK(testMV.reqTemp(0) == 0);
	testMV.setReqTemp(0,40);
	testMV.checkTemp(0);
	CHECK(testMV.reqTemp(0) == 40);
}
//
//TEST_CASE("Move-Wait-Check-Move-Off", "[MixValve][Basic_Sequence]") {
//	TestMixV testMV;
//	testMV.setToCool_30Deg(0, 60, 20);
//
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) == Mix_Valve::e_Moving);
//	CHECK(testMV.vPos(0) == 40);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Wait);
//	
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) == Mix_Valve::e_Wait);
//	CHECK(testMV.vPos(0) == 40);
//	CHECK(testMV.onTime(0) == 0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
//	testMV.checkTemp(0);
//	CHECK(testMV.onTime(0) == 75);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) == Mix_Valve::e_Moving);
//	CHECK(testMV.vPos(0) == 0);
//	CHECK(testMV.onTime(0) == 0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
//}
//
//TEST_CASE("Limit on Heat then OK then too hot", "[MixValve][Basic_Sequence]") {
//	TestMixV testMV;
//	testMV.setToCool_30Deg(0, 60, 20);
//	testMV.setReqTemp(0, 40);
//	testMV.checkTemp(0);
//	testMV.setReqTemp(0, 40);
//	testMV.checkTemp(0);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
//	CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
//	CHECK(testMV.vPos(0) == VALVE_TRANSIT_TIME);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
//	}
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == true);
//
//	testMV.setIsTemp(0, 40);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
//	}
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
//
//	testMV.setIsTemp(0, 45);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_Checking);
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
//
//
//}
//
//TEST_CASE("Limit on Heat then ReduceRequest", "[MixValve][Basic_Sequence]") {
//	TestMixV testMV;
//	testMV.setToCool_30Deg(0, 60, 20);
//	testMV.setReqTemp(0, 40);
//	testMV.checkTemp(0);
//	testMV.setReqTemp(0, 40);
//	testMV.checkTemp(0);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
//	CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
//	CHECK(testMV.vPos(0) == VALVE_TRANSIT_TIME);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
//	}
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == true);
//	testMV.setReqTemp(0, 29);
//	testMV.checkTemp(0);
//	testMV.setReqTemp(0, 29);
//	testMV.checkTemp(0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
//	testMV.checkTemp(0);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
//	}
//	testMV.setIsTemp(0, 29);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) == Mix_Valve::e_Wait);
//	testMV.checkTemp(0);
//	CHECK(testMV.journey(0) == Mix_Valve::e_TempOK);
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
//}
//
//TEST_CASE("Limit on Heat then Off", "[MixValve][Basic_Sequence]") {
//	TestMixV testMV;
//	testMV.setToCool_30Deg(0, 60, 20);
//	testMV.setReqTemp(0, 40);
//	testMV.checkTemp(0);
//	testMV.setReqTemp(0, 40);
//	testMV.checkTemp(0);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
//	CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
//	CHECK(testMV.vPos(0) == VALVE_TRANSIT_TIME);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_HotLimit);
//	}
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == true);
//	testMV.setReqTemp(0, 20);
//	testMV.checkTemp(0);
//	testMV.setReqTemp(0, 20);
//	testMV.checkTemp(0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
//	testMV.checkTemp(0);
//	CHECK(testMV.journey(0) == Mix_Valve::e_TempOK);
//	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
//	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
//}
//
//TEST_CASE("Off on Cool and Recover", "[MixValve][Basic_Sequence]") {
//	TestMixV testMV;
//	testMV.setToCool_30Deg(0, 60, 20);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_ValveOff);
//	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
//	CHECK(testMV.vPos(0) == 0);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
//	}
//	testMV.setIsTemp(0, 30);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
//	}
//	testMV.setIsTemp(0, 29);
//	testMV.checkTemp(0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_Checking);
//	testMV.checkTemp(0);
//	do {
//		testMV.checkTemp(0);
//	} while (testMV.mode(0) != Mix_Valve::e_Checking);
//	testMV.setIsTemp(0, 30);
//	for (int i = 0; i < 10; ++i) {
//		testMV.checkTemp(0);
//		CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
//	}
//}
//
//TEST_CASE("Cool-Heat-Cool", "[MixValve][Basic_Sequence]") {
//	TestMixV testMV;
//	testMV.setToCool_30Deg(0, 60, 20);
//	auto flowtemp = 32.0;
//	testMV.setIsTemp(0, flowtemp);
//	testMV.checkTemp(0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
//	CHECK(testMV.journey(0) == Mix_Valve::e_GoingCool);
//	do {
//		flowtemp -= 0.1;
//		testMV.setIsTemp(0, flowtemp);
//		testMV.checkTemp(0);
//	} while (testMV.journey(0) == Mix_Valve::e_GoingCool);
//	CHECK(testMV.journey(0) == Mix_Valve::e_GoingWarm);
//	
//	flowtemp = 27.0;
//	testMV.setIsTemp(0, flowtemp);
//	testMV.checkTemp(0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
//	do {
//		flowtemp += 0.1;
//		testMV.setIsTemp(0, flowtemp);
//		testMV.checkTemp(0);
//	} while (testMV.journey(0) == Mix_Valve::e_GoingWarm);
//	CHECK(testMV.journey(0) == Mix_Valve::e_GoingCool);
//
//	flowtemp = 32.0;
//	testMV.setIsTemp(0, flowtemp);
//	testMV.checkTemp(0);
//	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
//	CHECK(testMV.journey(0) == Mix_Valve::e_GoingCool);
//	do {
//		flowtemp -= 0.1;
//		testMV.setIsTemp(0, flowtemp);
//		testMV.checkTemp(0);
//	} while (testMV.journey(0) == Mix_Valve::e_GoingCool);
//	CHECK(testMV.journey(0) == Mix_Valve::e_GoingWarm);
//}

TEST_CASE("Find_50", "[MixValve][Dynamic]") {
	TestMixV testMV;
	testMV.setMode(0, Mix_Valve::e_Checking);
	testMV.setVPos(0, 10);
	testMV.setReqTemp(0, 50);
	testMV.checkTemp(0);
	testMV.setReqTemp(0, 50);
	testMV.checkTemp(0);
	auto correctPos = testMV.correctPos();
	logger() << "Correct Ratio is " << testMV.correctRatio() << " Correct Pos is " << correctPos << L_endl;
	
	logger() << "Move->Overshoot\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0); 
	} while (testMV.mode(0) != Mix_Valve::e_HotLimit);
	
	logger() << "Now wait for temp to reach required\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0); 
	} while (testMV.getIsTemp(0) != 50);
	
	logger() << "Now return to target-pos; abort when temp = 49\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0); 
	} while (testMV.getIsTemp(0) == 50);
	CHECK(testMV.mode(0) == Mix_Valve::e_Checking);
	testMV.checkTemp(0); 
	
	logger() << "Now launch and return to target\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0); 
	} while (testMV.mode(0) != Mix_Valve::e_Checking);
	double diff = testMV.launchPos(0) - testMV.vPos(0);
	auto ratio = (testMV.launchPos(0) - correctPos) / diff;
	logger() << "From: " << testMV.launchPos(0) << " to " << testMV.vPos(0) << " Ratio is " << ratio << L_endl;
	
	logger() << "1 Wait for drift\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Checking);
	
	logger() << "1 Next launch\n";
	auto journey = testMV.journey(0);
	auto launchPos = testMV.vPos(0);
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.journey(0) == journey && testMV.vPos(0) < 149);
	diff = launchPos - testMV.vPos(0);
	ratio = (launchPos - correctPos) / diff;
	logger() << "From: " << launchPos << " to " << testMV.vPos(0) << " Ratio is " << ratio << L_endl;

	logger() << "1 ...return to target\n";
	journey = testMV.journey(0);
	launchPos = testMV.vPos(0);
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.journey(0) == journey);

	logger() << "2 Wait for drift\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Checking);
	
	logger() << "2 Next launch\n";
	journey = testMV.journey(0);
	launchPos = testMV.vPos(0);
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.journey(0) == journey);
	diff = launchPos - testMV.vPos(0);
	ratio = (launchPos - correctPos) / diff;
	logger() << "From: " << launchPos << " to " << testMV.vPos(0) << " Ratio is " << ratio << L_endl;

	logger() << "2 ...return to target\n";
	journey = testMV.journey(0);
	launchPos = testMV.vPos(0);
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.journey(0) == journey);

	logger() << "3 Wait for drift\n";
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.mode(0) == Mix_Valve::e_Checking);
	
	logger() << "3 Next launch\n";
	journey = testMV.journey(0);
	launchPos = testMV.vPos(0);
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.journey(0) == journey);
	diff = launchPos - testMV.vPos(0);
	ratio = (launchPos - correctPos) / diff;
	logger() << "From: " << launchPos << " to " << testMV.vPos(0) << " Ratio is " << ratio << L_endl;

	logger() << "3 ...return to target\n";
	journey = testMV.journey(0);
	launchPos = testMV.vPos(0);
	do {
		testMV.autoIsTemp();
		testMV.checkTemp(0);
	} while (testMV.journey(0) == journey);

	logger() << "Now stay on target\n";
	for (int i = 0; i < 100; ++i) {
		testMV.autoIsTemp();
		testMV.checkTemp(0); 
	}
}