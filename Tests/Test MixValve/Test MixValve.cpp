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
#include <conio.h>

bool relaysOn();

char getKey() {
	char c = _getch();
	return c;
}

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
	float getFlowTemp(int mixInd) const { return mixValve[mixInd].flowTemp() / 256.f; }
	uint8_t reqTemp(int mixInd) const { return  mixValve[mixInd]._currReqTemp; }
	int vPos(int mixInd) const { return  mixValve[mixInd]._valvePos; }
	int mode(int mixInd) const { return  mixValve[mixInd].registers().get(Mix_Valve::R_MODE); }
	int direction(int mixInd) const {	return mixValve[mixInd]._motorDirection; }
	uint8_t status(int mixInd) const { return  mixValve[mixInd].registers().get(Mix_Valve::R_DEVICE_STATE); }
	uint16_t getTC(int mixInd) const {	return mixValve[mixInd].get_TC(); }
	int getDelay(int mixInd) const { return mixValve[mixInd].get_delay(); }
	int getMaxTemp(int mixInd) const { return mixValve[mixInd].get_maxTemp(); }
	// modifiers
	Mix_Valve& mv(int mixInd) { return mixValve[mixInd]; }
	void setMaxTemp(int mixInd, int max) { mixValve[mixInd].set_maxTemp(max); }
	void setVPos(int mixInd, int vpos) { mixValve[mixInd]._valvePos = vpos; }
	void setMode(int mixInd, Mix_Valve::Mode mode) { mixValve[mixInd].registers().set(Mix_Valve::R_MODE, mode); }
	void setReqTemp(int mixInd, int temp) { mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp); }
	void setIsTemp(int mixInd, int temp) { mixValve[mixInd].setIsTemp(temp); }
	void setDelay(int mixInd, int delay) { mixValve[mixInd].setDelay(delay); }
	void setTC(int mixInd, int tc) { mixValve[mixInd].setTC(tc); }
	auto update(int mixInd, int pos) { return mixValve[mixInd].update(pos); }
	void readyToMove(int mixInd) { setVPos(mixInd, 0); mixValve[mixInd].update(0); }
	void registerReqTemp(int mixInd, int temp) { 
		mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp);
		update(mixInd, vPos(mixInd));
		mixValve[mixInd].registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp);
		update(mixInd, vPos(mixInd));
	}
	void off(int index) { mixValve[index].turnValveOff(); }
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
	//Mix_Valve::motor_queued = false;
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
			requestTemp += step; // back to where it was
			requestTemp += step;
			if (requestTemp < 26) requestTemp = 26;
		}
		else if (requestTemp < 26) {
			step = -step;
			requestTemp += step; // back to where it was
			requestTemp += step;
			doneOneTempCycle = true;
		}
		return requestTemp;
	}
	void nextStep() {
		step = int(step * step_multiplier);
		doneOneStepCycle = false;
		if (abs(step) > 32) {
			step = step > 0 ? 32 : -32;
			step_multiplier = 0.5f;
		}		
		if (abs(step) < 1) {
			step = step > 0 ? 1 : -1;
			step_multiplier = 2.f;
			doneOneStepCycle = true;
		}
	}
	void restart_cycle() { doneOneTempCycle = false; doneOneStepCycle = false; }
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
	testMV.update(1, 0); // e_FindingOff -> e_ValveOff
	CHECK(testMV.mode(0) == Mix_Valve::e_WaitingToMove);
	CHECK(testMV.mode(1) == Mix_Valve::e_ValveOff);
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.vPos(1) == 0);
	CHECK(relaysOn() == false);
	// [0] Starts with Move
	testMV.update(0, 5); // -> e_Moving
	testMV.update(1, 35); // -> e_WaitingToMove
	CHECK(testMV.vPos(0) == 0);
	CHECK(testMV.vPos(1) == 0);
	CHECK(relaysOn() == true);
	do {
		testMV.update(0, 5);
		testMV.update(1, 35);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);

	CHECK(testMV.vPos(0) == 5);
	CHECK(testMV.vPos(1) == 0);
	CHECK(testMV.mode(0) == Mix_Valve::e_AtTargetPosition);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(relaysOn() == true);
	
	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);
	//testMV.update(0, 35);
	CHECK(testMV.vPos(0) == 5);
	CHECK(testMV.vPos(1) == 10);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_WaitingToMove);
	CHECK(relaysOn() == true);

	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 15);
	CHECK(testMV.vPos(1) == 11);
	CHECK(testMV.mode(0) == Mix_Valve::e_WaitingToMove);
	CHECK(testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(relaysOn() == true);

	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(1) == Mix_Valve::e_Moving);
	CHECK(testMV.vPos(0) == 15);
	CHECK(testMV.vPos(1) == 20);
	CHECK(testMV.mode(0) == Mix_Valve::e_Moving);
	CHECK(testMV.mode(1) == Mix_Valve::e_WaitingToMove);
	CHECK(relaysOn() == true);

	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(0) != Mix_Valve::e_AtTargetPosition);

	CHECK(testMV.vPos(0) == 35);
	CHECK(testMV.vPos(1) == 30);
	CHECK(relaysOn() == true);
	do {
		testMV.update(0, 35);
		testMV.update(1, 35);
	} while (testMV.mode(1) != Mix_Valve::e_AtTargetPosition);
	CHECK(testMV.vPos(0) == 35);
	CHECK(testMV.vPos(1) == 35);
	CHECK(relaysOn() == false);
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

void appendData(Mix_Valve& mv, PID_Controller& pid) {
	reqPos.push_back(pid.currOut());
	isPos.push_back(mv._valvePos);
	reqTemp.push_back(mv._currReqTemp);
	isTemp.push_back(mv.flowTemp() / 256.f);
	i.push_back(pid.integralPart());
	Kd.push_back(pid.diffConst() * 10.f);
	Kp.push_back(pid.propConst() * 10000.f);
	overshootRatio.push_back(pid.overshoot_ratio() * 10);
}

void printDataHeader() {
	cout
		<< "TC:"
		<< "\tDelay:"
		<< "\tMaxT:"
		<< "\tStep:"
		<< "\tRatio:"
		<< "\tKd:"
		<< "\tKp:"
		<< "\tdFact:"
		<< "\td"
		<< endl;
}
void printData(TestTemps& testTemps, Mix_Valve& mv, PID_Controller& pid) {
	cout
		<< mv.get_TC()
		<< "\t" << mv.get_delay()
		<< "\t" << mv.get_maxTemp()
		<< "\t" << abs(testTemps.step)
		<< "\t" << pid.overshoot_ratio()
		<< "\t" << pid.diffConst()
		<< "\t" << pid.propConst()
		<< endl;
}

bool changeParams(TestTemps& testTemps, TestMixV& testMV) {
	cout << "q(uit), r(epeat), C/c(TC), D/d(delay), S/s(step), T/t (Temp), M/m(MaxMinT)" << endl;
	while (true) {
		switch (getKey()) {
		case 'q': return false;
		case 'r': return true;
		case 'C':
			testMV.setTC(0,testMV.getTC(0) * 2);
			cout << "TC: " << testMV.getTC(0) << endl;
			return true;
		case 'c':
			testMV.setTC(0,testMV.getTC(0) / 2);
			if (testMV.getTC(0) < 1) testMV.setTC(0, 1);
			cout << "TC: " << testMV.getTC(0) << endl;
			return true;
		case 'D':
			testMV.setDelay(0,testMV.getDelay(0) * 2);
			cout << "Delay: " << testMV.getDelay(0) << endl;
			return true;
		case 'd':
			testMV.setDelay(0,testMV.getDelay(0) / 2);
			if (testMV.getDelay(0) < 1) testMV.setDelay(0, 1);
			cout << "Delay: " << testMV.getDelay(0) << endl;
			return true;
		case 'S':
			testTemps.step = testTemps.step * 2;
			cout << "Step: " << testTemps.step << endl;
			return true;
		case 's':
			testTemps.step = testTemps.step / 2;
			if (abs(testTemps.step) < 1) testTemps.step = 1;
			cout << "Step: " << testTemps.step << endl;
			return true;
		case 'T':
			testTemps.requestTemp = 85;
			cout << "Temp: " << testTemps.requestTemp << endl;
			return true;
		case 't':
			testTemps.requestTemp = 25;
			cout << "Temp: " << testTemps.requestTemp << endl;
			return true;
		case 'M':
			testMV.setMaxTemp(0,testMV.getMaxTemp(0) + 2);
			cout << "MaxTemp: " << testMV.getMaxTemp(0) << endl;
			return true;
		case 'm':
			testMV.setMaxTemp(0, testMV.getMaxTemp(0) - 2);
			cout << "MaxTemp: " << testMV.getMaxTemp(0) << endl;
			return true;
		default:
			;
		}
	}
}

void runOneTemp(TestTemps& testTemps, Mix_Valve& mv, PID_Controller& pid) {
	auto flowTemp = mv.update(pid.currOut());
	pid.adjust(flowTemp);
	//testMV.setVPos(0, newPos); // instant move!
	// record results
	appendData(mv, pid);
	step.push_back(abs(testTemps.step));
}

void run_n_Temps(int noToRun, TestTemps& testTemps, Mix_Valve& mv, PID_Controller& pid) {
	auto newPos = 0;
	auto currTempReq = testTemps.requestTemp;
	testTemps.restart_cycle();
	do {
		mv.registerReqTemp(currTempReq);
		pid.changeSetpoint(currTempReq * 256);
		mv.setTestTime();
		do {
			runOneTemp(testTemps, mv, pid);
		} while (mv.waitToSettle());
		printData(testTemps, mv, pid);
		currTempReq = testTemps.nextTempReq();
	} while (--noToRun);
}

void runOneCycle(TestTemps& testTemps, Mix_Valve& mv, PID_Controller& pid) {
	auto newPos = 0;
	auto currTempReq = testTemps.requestTemp;
	testTemps.restart_cycle();
	do {
		mv.registerReqTemp(currTempReq);
		pid.changeSetpoint(mv._currReqTemp * 256);
		runOneTemp(testTemps, mv, pid);
		printData(testTemps, mv, pid);
		currTempReq = testTemps.nextTempReq();
		//cout << currTempReq << endl;
	} while (!testTemps.doneOneTempCycle);
}

void changingStepCycle(TestTemps& testTemps, Mix_Valve& mv, PID_Controller& pid) {
	testTemps.step = 1;
	auto newPos = 0;
	auto currTempReq = testTemps.requestTemp;
	testTemps.restart_cycle();
	do {
		mv.registerReqTemp(currTempReq);
		pid.changeSetpoint(mv._currReqTemp * 256);
		runOneTemp(testTemps, mv, pid);
		printData(testTemps, mv, pid);
		testTemps.nextStep();
		currTempReq = testTemps.nextTempReq();
	} while (!testTemps.doneOneStepCycle);
}

void find_Kd_per_step(TestTemps& testTemps, Mix_Valve& mv, PID_Controller& pid) {
	testTemps.step = 1;
	auto newPos = 0;
	auto currTempReq = testTemps.requestTemp;
	testTemps.restart_cycle();
	do {
		auto noOfTrys = 20;
		do {
			mv.registerReqTemp(currTempReq);
			pid.changeSetpoint(mv._currReqTemp * 256);
			runOneTemp(testTemps, mv, pid);
			currTempReq = testTemps.nextTempReq();
			//printData(testTemps, testMV, pid);
		} while (--noOfTrys /*&& (pid.overshoot_ratio() < 1.07 || pid.overshoot_ratio() > 1.15)*/);
		printData(testTemps, mv, pid);
		testTemps.nextStep();
	} while (!testTemps.doneOneStepCycle);
}

//
//TEST_CASE("PID_Step_v_delay", "[MixValve]") {
//	TestMixV test2MV{};
//  auto& testMV = test2MV.mv(0);
//	PID_Controller pid{0,150,256/16,256/8};
//	TestTemps testTemps;
//	clearVectors();
//	// ini-MV
//	test2MV.setReqTemp(0, Mix_Valve::ROOM_TEMP);
//	test2MV.readyToMove(0);
//	test2MV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
//	test2MV.setMaxTemp(0, 70);
//
//	// Ini PID
//	pid.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
//	pid.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
//	cout << "PID_Step_v_delay" << endl;
//	printDataHeader();
//	// Go...
//	test2MV.setDelay(0, 1);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	test2MV.setDelay(0, 10);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	test2MV.setDelay(0, 30);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	test2MV.setDelay(0, 100);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	test2MV.setDelay(0, 200);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	plt::figure_size(1400, 700);
//	plt::named_plot("ReqTemp", reqTemp);
//	plt::named_plot("Temp", isTemp);
//	plt::named_plot("Step", step);
//	plt::named_plot("OverShRatio * 10", overshootRatio);
//	plt::named_plot("Pos", isPos);
//	//plt::named_plot("i", i);
//	plt::named_plot("Kd*100", Kd);
//	plt::named_plot("Kp*10000", Kp);
//	plt::legend();
//	plt::show();
//}
//
//TEST_CASE("PID_Step_v_tc", "[MixValve]") {
//	TestMixV testMV{};
//	PID_Controller pid{0,150,256/16,256/8};
//	TestTemps testTemps;
//	clearVectors();
//	// ini-MV
//	testMV.setReqTemp(0, Mix_Valve::ROOM_TEMP);
//	testMV.readyToMove(0);
//	testMV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
//	testMV.setMaxTemp(0, 70);
//
//	// Ini PID
//	pid.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
//	pid.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
//	testMV.setDelay(0, 30);
// 	cout << "PID_Step_v_tc" << endl;
//
//	printDataHeader();
//	// Go...
//	testMV.setTC(0,1);
//	find_Kd_per_step(testTemps, testMV, pid);
//	getKey();
//	testMV.setTC(0, 10);
//	find_Kd_per_step(testTemps, testMV, pid);
//	getKey();
//
//	testMV.setTC(0, 30);
//	find_Kd_per_step(testTemps, testMV, pid);
//	getKey();
//
//	testMV.setTC(0, 100);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	testMV.setTC(0, 200);
//	find_Kd_per_step(testTemps, testMV, pid);
//
//	plt::figure_size(1400, 700);
//	plt::named_plot("ReqTemp", reqTemp);
//	plt::named_plot("Temp", isTemp);
//	plt::named_plot("Step", step);
//	plt::named_plot("OverShRatio * 10", overshootRatio);
//	plt::named_plot("Pos", isPos);
//	//plt::named_plot("i", i);
//	plt::named_plot("Kd*100", Kd);
//	plt::named_plot("Kp*10000", Kp);
//	plt::legend();
//	plt::show();
//}

//TEST_CASE("PID_change_params", "[MixValve]") {
//	TestMixV testMV{};
//	PID_Controller pid{0,150, 25*256, 256/16,256};
//	TestTemps testTemps;
//	clearVectors();
//	// ini-MV
//	testMV.setReqTemp(0, Mix_Valve::ROOM_TEMP);
//	testMV.readyToMove(0);
//	testMV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
//	testMV.setMaxTemp(0, 70);
//
//	// Ini PID
//	pid.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
//	pid.set_Kd(.3f);
//	pid.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
//	testMV.setDelay(0, 30);
//	cout << "PID_change_params" << endl;
//	
//	printDataHeader();
//	// Go...
//	testMV.setTC(0,1);
//	auto hWnd = GetConsoleWindow();
//	do {
//		clearVectors();
//		printDataHeader();
//		run_n_Temps(4,testTemps, testMV, pid);
//#ifndef _DEBUG
//		plt::close();
//		plt::figure_size(1400, 600);
//		//plt::get_current_fig_manager().window.setGeometry(0,0, 1400, 600);
//		plt::named_plot("ReqTemp D:" + to_string(testMV.getDelay(0)), reqTemp);
//		plt::named_plot("Temp TC: " + to_string(testMV.getTC(0)), isTemp);
//		plt::named_plot("Step: " + to_string(testTemps.step), step);
//		plt::named_plot("OverSh*10: " + to_string(pid.overshoot_ratio()), overshootRatio);
//		plt::named_plot("Pos. MaxT: " + to_string(testMV.getMaxTemp(0)), isPos);
//		//plt::named_plot("i", i);
//		plt::named_plot("Kd*10: " + to_string(pid.diffConst()), Kd);
//		//plt::named_plot("Kp*10000", Kp);
//		plt::legend();
//		plt::show(false);
//		plt::pause(.1);
//#endif
//		SetForegroundWindow(hWnd);
//		SetFocus(hWnd);
//	} while (changeParams(testTemps, testMV));
//}

TEST_CASE("DualMV_PID_change_params", "[MixValve]") {
	TestMixV testMV{};
	auto& mv0 = testMV.mv(0);
	auto& mv1 = testMV.mv(1);
	PID_Controller pid0{0,140, 25*256, 256/16,256};
	PID_Controller pid1{0,140, 25*256, 256/16,256};
	TestTemps testTemps0;
	TestTemps testTemps1;
	clearVectors();
	// ini-MV
	testMV.setReqTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.readyToMove(0);
	testMV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.setMaxTemp(0, 70);

	testMV.setReqTemp(1, Mix_Valve::ROOM_TEMP);
	testMV.readyToMove(1);
	testMV.setIsTemp(1, Mix_Valve::ROOM_TEMP);
	testMV.setMaxTemp(1, 70);

	// Ini PID
	pid0.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
	pid0.set_Kd(.3f);
	pid0.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
	testMV.setDelay(0, 30);
	pid1.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
	pid1.set_Kd(.3f);
	pid1.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
	testMV.setDelay(1, 30);
	cout << "DualMV_PID_change_params" << endl;
	
	printDataHeader();
	// Go...
	testMV.setTC(0,10);
	testMV.setTC(1,10);
	auto hWnd = GetConsoleWindow();
	do {
		clearVectors();
		printDataHeader();
		run_n_Temps(4,testTemps0, mv0, pid0);
#ifndef _DEBUG
		plt::close();
		plt::figure_size(1400, 600);
		//plt::get_current_fig_manager().window.setGeometry(0,0, 1400, 600);
		plt::named_plot("ReqTemp D:" + to_string(testMV.getDelay(0)), reqTemp);
		plt::named_plot("Temp TC: " + to_string(testMV.getTC(0)), isTemp);
		plt::named_plot("Step: " + to_string(testTemps0.step), step);
		plt::named_plot("OverSh*10: " + to_string(pid0.overshoot_ratio()), overshootRatio);
		plt::named_plot("Pos. MaxT: " + to_string(testMV.getMaxTemp(0)), isPos);
		//plt::named_plot("i", i);
		plt::named_plot("Kd*10: " + to_string(pid0.diffConst()), Kd);
		//plt::named_plot("Kp*10000", Kp);
		plt::legend();
		plt::show(false);
		plt::pause(.1);
#endif
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);
	} while (changeParams(testTemps0, testMV));
}

/*
TEST_CASE("PID_ChangeStep", "[MixValve]") {
	TestMixV testMV{};
	PID_Controller pid{0,150,256/16,256/2};
	TestTemps testTemps;
	// ini-MV
	testMV.setReqTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.readyToMove(0);
	testMV.setIsTemp(0, Mix_Valve::ROOM_TEMP);
	testMV.setMaxTemp(0, 90);
	testMV.setDelay(0, 50);

	// Ini PID
	pid.set_Kp(MAX_VALVE_TIME / 50.f / 256.f);
	pid.changeSetpoint(Mix_Valve::ROOM_TEMP * 256);
	// Go...
	changingStepCycle(testTemps, testMV, pid);
	changingStepCycle(testTemps, testMV, pid);
	//plt::clf();

	plt::figure_size(1400, 700);
	plt::named_plot("ReqTemp", reqTemp);
	plt::named_plot("Temp", isTemp);
	plt::named_plot("Step", step);
	plt::named_plot("OverShRatio * 10", overshootRatio);
	plt::named_plot("Pos", isPos);
	plt::named_plot("i", i);
	plt::named_plot("Kd*100", Kd);
	plt::named_plot("Kp*10000", Kp);
	plt::legend();
	plt::show();

	plt::named_plot("Overshoot v Step", step1, overshootRatio1);
	plt::legend();
	plt::show();
}


TEST_CASE("PID_temp_cycle", "[MixValve]") {
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
	
	testTemps.step = 3;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;	
	
	testTemps.step = 4;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;	
	
	testTemps.step = 5;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;	
	
	testTemps.step = 6;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.step = 10;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;

	testTemps.step = 20;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	testTemps.step = 30;
	runOneCycle(testTemps, testMV, pid);	
	cout << "Step:\t" << abs(testTemps.step) << "\tRatio:\t" << pid.overshoot_ratio() << "\tKd:\t" << pid.diffConst() << endl;
	
	//runOneCycle(testTemps, testMV, pid);
	//plt::clf();
	plt::figure_size(1400, 700);
	plt::named_plot("ReqTemp", reqTemp);
	//plt::named_plot("ReqPos", reqPos);
	plt::named_plot("Pos", isPos);
	plt::named_plot("Temp", isTemp);
	plt::named_plot("i", i);
	plt::named_plot("Kd*100", Kd);
	plt::named_plot("Kp*10000", Kp);
	//plt::named_plot("OverShRatio * 10", overshootRatio);

	plt::legend();
	plt::show();

	//plt::show(false);
	//plt::pause(0.1);
	//auto again = string{};
	//cin >> again;
}
*/