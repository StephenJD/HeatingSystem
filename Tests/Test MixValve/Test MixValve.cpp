// Test MixValve.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define SIM_MIXV

#include <catch.hpp>
#include <Arduino.h>
#include <Mix_Valve.h>
#include <PID_Controller.h>
#include <I2C_RecoverRetest.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <PinObject.h>
#include <PID_Controller.h>
#define  WITHOUT_NUMPY
#include "matplotlibcpp.h"
#include <vector>

using namespace I2C_Recovery;
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;
using namespace flag_enum;
using namespace std;
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
	//uint8_t calcEndPos(int mixInd) { return mixValve[mixInd].calculatedEndPos(); }
	float getFlowTemp(int mixInd) { return mixValve[mixInd].flowTemp() / 256.f; }
	void setMaxTemp(int mixInd, int max) { mixValve[mixInd].set_maxTemp(max); }
	void setVPos(int mixInd, int vpos) { mixValve[mixInd]._valvePos = vpos; }
	void setMode(int mixInd, Mix_Valve::Mode mode) { mixValve[mixInd].registers().set(Mix_Valve::R_MODE, mode); }
	void setReqTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp); }
	void setIsTemp(int mixInd, int temp) { mixValve[mixInd].setIsTemp(temp); }
	void setDelay(int mixInd, int delay) { mixValve[mixInd].setDelay(delay); }
	auto update(int mixInd, int pos) { return mixValve[mixInd].update(pos); }
	void readyToMove(int mixInd) { setVPos(mixInd, 0); mixValve[mixInd].update(0); }
	void registerReqTemp(int mixInd, int temp) { 
		mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp);
		update(mixInd, vPos(mixInd));
		mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp);
		update(mixInd, vPos(mixInd));
	}
	void off(int index) { mixValve[index].turnValveOff(); }
	uint8_t reqTemp(int mixInd) { return  mixValve[mixInd]._currReqTemp; }
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

private:
	Mix_Valve mixValve[2];
	//int simFlowT(int mixV) { return tankTemp * vPos(mixV) / 150; }
	//int tankTemp;
};

TestMixV::TestMixV() : mixValve{
		{i2c_recover, US_FLOW_TEMPSENS_ADDR, us_heatRelay, us_coolRelay, eeprom(), 0,TS_TIME_CONST, TS_DELAY, 70}
	  , {i2c_recover, DS_FLOW_TEMPSENS_ADDR, ds_heatRelay, ds_coolRelay, eeprom(), Mix_Valve::MV_ALL_REG_SIZE, TS_TIME_CONST, TS_DELAY, 40}
} {
	Mix_Valve::motor_mutex = 0;
	Mix_Valve::motor_queued = false;
	mixValve[0].begin(55); // does speed-test for TS
	mixValve[1].begin(55);
}

class TestTemps {
public:
	int nextTempReq() {
		requestTemp += step;
		doneOneTempCycle = false;
		if (requestTemp > 60) {
			step = -step;
			requestTemp = 60;
		}
		else if (requestTemp < 26) {
			step = -step;
			requestTemp = 26;
			doneOneTempCycle = true;
		}
		return requestTemp;
	}
	void nextStep() {
		step *= step_multiplier;
		doneOneStepCycle = false;
		if (step > 16) {
			step = 16;
			step_multiplier = 0.5f;
		}		
		if (step < 1) {
			step = 1;
			step_multiplier = 2.f;
			doneOneStepCycle = true;
		}
	}
	bool doneOneTempCycle = false;
	bool doneOneStepCycle = false;
	int step = 1;
	int requestTemp = 26;
	float step_multiplier = 2.f;
};

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
	testMV.setVPos(1, 30);
	do {
		testMV.update(0,35);
		testMV.update(1,35);
	} while (testMV.mode(0) == Mix_Valve::e_FindingOff);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(1) == Mix_Valve::e_FindingOff);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.vPos(1) == 0);
}

TEST_CASE("ChangeDirrection", "[MixValve]") {
	TestMixV testMV;
	testMV.readyToMove(0);
	do {
		testMV.update(0, 10); // start moving
	} while (testMV.vPos(0) != 8);
	testMV.update(0, 3); // change direction
	CHECK(testMV.vPos(0) == 9); // registered final move
	testMV.update(0, 3);
	CHECK(testMV.vPos(0) == 8);
	do {
		testMV.update(0, 3); // start moving
	} while (testMV.vPos(0) != 3);
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
}

TEST_CASE("Move1", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0, 0);
	testMV.setVPos(1, 0);
	testMV.update(0, 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);
	CHECK(testMV.mode(1) == Mix_Valve::e_FindingOff);
	testMV.update(1, 0);
	CHECK(testMV.mode(1) == Mix_Valve::e_ValveOff);
	// Change vpos request
	testMV.update(0, 1); // start moving
	testMV.update(0, 1); // get there
	testMV.update(1, 1); // obtain mutex / start moving
	testMV.update(1, 1); // get there
	CHECK(testMV.vPos(0) == 1);
	CHECK(testMV.vPos(1) == 1);
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	CHECK(testMV.mode(1) == Mix_Valve::e_AtTargetPosition);
	testMV.update(0, 2); // start moving
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	testMV.update(1, 2); // waiting
	CHECK(testMV.mode(1) == Mix_Valve::e_WaitingToMove);
	testMV.update(0, 2); // get there
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	testMV.update(1, 2); // obtain mutex / start moving
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	testMV.update(1, 2); // get there
	CHECK(testMV.vPos(0) == 2);
	CHECK(testMV.vPos(1) == 2);
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	CHECK(testMV.mode(1) == Mix_Valve::e_AtTargetPosition);
}

TEST_CASE("MutexSwap", "[MixValve]") {
	TestMixV testMV;
	testMV.setVPos(0, 0);
	testMV.setVPos(1, 0);
	testMV.update(0, 35); // e_FindingOff -> e_WaitingToMove
	testMV.update(1, 35); // e_FindingOff -> e_WaitingToMove
	CHECK(testMV.mode(0) == Mix_Valve::e_WaitingToMove);
	CHECK(testMV.mode(1) == Mix_Valve::e_WaitingToMove);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.vPos(1) == 0);
	// [0] Starts with Move
	testMV.update(0, 35); // -> e_Moving
	testMV.update(1, 35); // -> e_WaitingToMove
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

TEST_CASE("Limit on Heat then OK", "[MixValve]") {
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
	while (testMV.mode(0) == Mix_Valve::e_HotLimit) {
		testMV.update(0, 150);
	}
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);

	do {
		testMV.update(0, 100);
	} while (testMV.mode(0) != Mix_Valve::e_AtTargetPosition);
	CHECK(Mix_Valve::I2C_Flags_Obj(testMV.status(0)).is(Mix_Valve::F_STORE_TOO_COOL) == false);
}
/*
TEST_CASE("testTemps", "[MixValve]") {
	TestTemps testTemps;
	testTemps.step = 10;
	vector<int> reqTemp;
	auto currTempReq = testTemps.requestTemp;
	do {
		int timeAtOneTemp = 10;
		do {
			reqTemp.push_back(currTempReq);
		} while (--timeAtOneTemp);
		currTempReq = testTemps.nextTempReq();
	} while (!testTemps.doneOneTempCycle);
	plt::named_plot("ReqTemp", reqTemp);
	plt::legend();
	plt::show();
}

TEST_CASE("testTempSim", "[MixValve]") {
	TestMixV testMV{};
	testMV.readyToMove(0);
	testMV.setIsTemp(0, 25);
	CHECK(testMV.mode(0) == Mix_Valve::e_ValveOff);

	vector<float> isTemp;
	auto currTemp = testMV.update(0,50);
	int timeAtOneTemp = 200;
	do {
		isTemp.push_back(currTemp/256.f);
		currTemp = testMV.update(0, 50);
	} while (--timeAtOneTemp);
	plt::named_plot("ReqTemp", isTemp);
	plt::legend();
	plt::show();
}
*/
vector<int> reqPos;
vector<int> isPos;
vector<int> reqTemp;
vector<float> isTemp;
vector<int> i;
vector<float> Kd;
vector<float> Kp;
vector<int> step;
vector<float> overshootRatio;
vector<int> step1;
vector<float> overshootRatio1;

void clearVectors() {
	reqPos.clear();
	isPos.clear();
	reqTemp.clear();
	isTemp.clear();
	i.clear();
	Kd.clear();
	Kp.clear();
	step.clear();
	overshootRatio.clear();
	step1.clear();
	overshootRatio1.clear();
}

void appendData(TestMixV& testMV, PID_Controller& pid) {
	reqPos.push_back(pid.currOut());
	isPos.push_back(testMV.vPos(0));
	reqTemp.push_back(testMV.reqTemp(0));
	isTemp.push_back(testMV.getFlowTemp(0));
	i.push_back(pid.integralPart());
	Kd.push_back(pid.diffConst() * 100.f);
	Kp.push_back(pid.propConst() * 10000.f);
	overshootRatio.push_back(pid.overshoot_ratio() * 10);
}

void runOneCycle(TestTemps& testTemps, TestMixV& testMV, PID_Controller& pid) {
	auto newPos = 0;
	auto currTempReq = testTemps.requestTemp;
	do {
		testMV.registerReqTemp(0, currTempReq);
		pid.changeSetpoint(testMV.reqTemp(0) * 256);
		int timeAtOneTemp = 500;
		do {
			auto flowTemp = testMV.update(0, newPos);
			newPos = pid.adjust(flowTemp);
			//testMV.setVPos(0, newPos); // instant move!
			// record results
			appendData(testMV,pid);
			step.push_back(abs(testTemps.step));
		} while (--timeAtOneTemp);
		cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
		currTempReq = testTemps.nextTempReq();
		//cout << currTempReq << endl;
	} while (!testTemps.doneOneTempCycle);
}

void changingStepCycle(TestTemps& testTemps, TestMixV& testMV, PID_Controller& pid) {
	testTemps.step = 1;
	auto newPos = 0;
	auto currTempReq = testTemps.requestTemp;
	do {
		testMV.registerReqTemp(0, currTempReq);
		pid.changeSetpoint(testMV.reqTemp(0) * 256);
		int timeAtOneTemp = 200;
		do {
			auto flowTemp = testMV.update(0, newPos);
			newPos = pid.adjust(flowTemp);
			//testMV.setVPos(0, newPos); // instant move!
			// record results
			appendData(testMV, pid);
			step.push_back(abs(testTemps.step));
		} while (--timeAtOneTemp);
		step1.push_back(abs(testTemps.step));
		overshootRatio1.push_back(pid.overshoot_ratio());
		cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << endl;
		++testTemps.step;
		currTempReq = testTemps.nextTempReq();
	} while (abs(testTemps.step) > 1);
}
/*
TEST_CASE("PID_ChangeStep", "[MixValve]") {
	TestMixV testMV{};
	PID_Controller pid{0,150,256/16,256/8};
	TestTemps testTemps;
	clearVectors();
	// ini-MV
	testMV.setReqTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.readyToMove(0);
	testMV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.setMaxTemp(0, 70);

	// Ini PID
	pid.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
	pid.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
	// Go...
	changingStepCycle(testTemps, testMV, pid);
	//changingStepCycle(testTemps, testMV, pid);
	//plt::clf();

	plt::figure_size(1400, 700);
	plt::named_plot("ReqTemp", reqTemp);
	plt::named_plot("Temp", isTemp);
	plt::named_plot("Step", step);
	plt::named_plot("OverShRatio * 10", overshootRatio);
	plt::named_plot("Pos", isPos);
	plt::legend();
	plt::show();

	plt::named_plot("Overshoot v Step", step1, overshootRatio1);
	plt::legend();
	plt::show();


}
*/

TEST_CASE("PID", "[MixValve]") {
	TestMixV testMV{};
	PID_Controller pid{0,150,256/16,256/2};
	TestTemps testTemps;
	clearVectors();
	// ini-MV
	testMV.setReqTemp(0,Mix_Valve::ROOM_TEMP);
	testMV.readyToMove(0);
	testMV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.setMaxTemp(0, 90);
	testMV.setDelay(0, 50);
	// Set req-temp
	testTemps.step = 1;
	// Ini PID
	pid.set_Kp(MAX_VALVE_TIME/50.f/256.f);
	pid.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
	testTemps.step = 1;
	runOneCycle(testTemps, testMV, pid);
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio()  << "\tKd:\t" << pid.diffConst() << endl;

	testTemps.step = 2;
	runOneCycle(testTemps, testMV, pid);
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio()  << "\tKd:\t" << pid.diffConst() << endl;

	testTemps.doneOneTempCycle = false;
	testTemps.step = 3;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.doneOneTempCycle = false;
	testTemps.step = 4;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.doneOneTempCycle = false;
	testTemps.step = 5;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.doneOneTempCycle = false;
	testTemps.step = 6;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.doneOneTempCycle = false;
	testTemps.step = 10;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;

	testTemps.doneOneTempCycle = false;
	testTemps.step = 20;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.doneOneTempCycle = false;
	testTemps.step = 30;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	//testTemps.doneOneTempCycle = false;
	//runOneCycle(testTemps, testMV, pid);
	//plt::clf();
	plt::figure_size(1400, 700);
	plt::named_plot("ReqTemp", reqTemp);
	//plt::named_plot("ReqPos", reqPos);
	plt::named_plot("Pos", isPos);
	plt::named_plot("Temp", isTemp);
	plt::named_plot("i", i);
	plt::named_plot("Kd*100", Kd);
	//plt::named_plot("Kp*10000", Kp);
	//plt::named_plot("OverShRatio * 10", overshootRatio);

	plt::legend();
	plt::show();

	//plt::show(false);
	//plt::pause(0.1);
	//auto again = string{};
	//cin >> again;
}
