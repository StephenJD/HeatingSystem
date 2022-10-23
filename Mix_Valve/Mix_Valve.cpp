// This is the Multi-Master Arduino Mini Controller
#define SIM_MIXV

#include <Mix_Valve.h>
#include <TempSensor.h>
#include "PinObject.h"
#include <I2C_Talk.h>
#include <I2C_SpeedTest.h>
#include <I2C_Recover.h>
#include <I2C_Registers.h>
#include <I2C_Talk_ErrorCodes.h>
#include <Logging.h>
#include <Timer_mS_uS.h>

using namespace HardwareInterfaces;
using namespace arduino_logger;
using namespace I2C_Recovery;
using namespace I2C_Talk_ErrorCodes;
using namespace flag_enum;

void enableRelays(bool enable); // this function must be defined elsewhere
bool relaysOn(); // this function must be defined elsewhere
extern bool receivingNewData;
extern const uint8_t version_month;
extern const uint8_t version_day;

constexpr int PSU_V_PIN = A3;
Mix_Valve* Mix_Valve::motor_mutex = 0;
int16_t Mix_Valve::_motorsOffV = 1024;
int16_t Mix_Valve::_motors_off_diff_V = int16_t(_motorsOffV * 0.03);

bool Mix_Valve::motor_queued;
// PID Functions
void Mix_Valve::requestNewPosition(int newPos) {
	_endPos = newPos;
	_motorDirection = static_cast<MotorDirection>(newPos - _valvePos);
}

int16_t Mix_Valve::update(int newPos) {
	if (newPos != _endPos) requestNewPosition(newPos);
	_valvePos += _motorDirection;
#ifdef SIM_MIXV
	addTempToDelayIntegral();
	setFlowTempReg();
#endif
	auto reg = registers();
	auto sensorTemp = reg.get(R_FLOW_TEMP) + reg.get(R_FLOW_TEMP_FRACT) / 256.f;
	return int16_t(sensorTemp);
}

// Loop Functions
void Mix_Valve::check_flow_temp() { // Called once every second. maintains mix valve temp.
	while (stateMachine());
	refreshRegisters();
	log();
}

bool Mix_Valve::stateMachine() { // returns true if in transitional-mode
	// Gets new TempReq, finds OFF, stops a limit or moves towards requested posistion.
	// Lambda
	auto moveMode = [this]() {
		if (motor_mutex == this) return e_Moving;
		else if (motor_mutex == 0) {
			motor_mutex = this;
			activateMotor_isMoving();
			return e_Moving;
		}
		else {
			logger() << name() << L_tabs << F("Mutex") << L_endl;
			motor_queued = true;
			_cool_relay->set(false);
			_heat_relay->set(false);
			return e_WaitingToMove;
		}
	};

	//Algorithm
	auto reg = registers();
	auto sensorTemp = reg.get(R_FLOW_TEMP) + reg.get(R_FLOW_TEMP_FRACT) / 256.;
	bool isTooCool = sensorTemp < _currReqTemp;
	auto mode = reg.get(R_MODE);
	if (mode != e_FindingOff && checkForNewReqTemp()) {
		if (_newReqTemp > HardwareInterfaces::MIN_FLOW_TEMP) {
			mode = e_newReq;
		}
		else {
			mode = e_findOff;
		}
	}
	switch (mode) {
		// Transitional-modes, immediatly processed
	case e_findOff:
		turnValveOff();
		mode = e_FindingOff;
		break;
	case e_newReq:
		logger() << name() << L_tabs << F("New Temp Req:") << _currReqTemp << L_endl;
		mode = newTempMode();
		break;
	case e_reachedLimit:
		_cool_relay->set(false);
		_heat_relay->set(false);
		if (_valvePos == 0) {
			mode = e_ValveOff;
		}
		else {
			I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_STORE_TOO_COOL);
			mode = e_HotLimit;
		}
		break;
	case e_swapMutex:
		motor_mutex = 0;
		mode = e_WaitingToMove;
		break;
		// Persistant Modes checked once each second
	case e_Moving:
		if (valveIsAtLimit()) {
			mode = e_reachedLimit;
		}
		else if (motor_queued && _valvePos % 10 == 0) {
			mode = e_swapMutex;
		}
		else if (!continueMove()) {
			mode = e_AtTargetPosition;
		}
		 break;
	case e_WaitingToMove: // try to move
		mode = moveMode();
		break;
	case e_AtTargetPosition:
		break;
	case e_HotLimit:
		if (!isTooCool) {
			I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL); // when flow temp increases due to gas boiler on.
			mode = e_WaitingToMove;
		}
		break;
	case e_WaitToCool:
	case e_ValveOff:
		if (isTooCool) mode = e_WaitingToMove;
		break;
	}
	reg.set(R_MODE, mode);
	return mode >= e_findOff;
	//logger() << F("Mode:") << mode << L_endl;
}

// Continuation Functions
bool Mix_Valve::continueMove() { // must be called just once per second
	_valvePos += _motorDirection;
	if (_valvePos < 0) {
		_valvePos = 0;
	}
	else if (_valvePos > 511) {
		_valvePos = 511;
	}
	return _valvePos != _endPos;
}

// Transition Functions

void Mix_Valve::stopMotor() {
	_motorDirection = e_Stop;
	motor_queued = false;
	motor_mutex = 0;
	activateMotor_isMoving();
}

bool Mix_Valve::activateMotor_isMoving() {
	// Lambda
	auto getPSU_Off_V = [this]() {
		enableRelays(true);
		auto newOffV = measurePSUVoltage(100);
		if (newOffV) {
			_motorsOffV = newOffV;
			_motors_off_diff_V = uint16_t(_motorsOffV * 0.03);
			//logger() << F("New OFF-V: ") << _motorsOffV << L_endl;
		}
	};
	// Algorithm
	auto isMoving = true;
	_cool_relay->set(_motorDirection == e_Cooling); // turn Cool relay on/off
	_heat_relay->set(_motorDirection == e_Heating); // turn Heat relay on/off
	if (_motorDirection == e_Stop) {
		getPSU_Off_V();
		enableRelays(motor_queued); // disable if no queued motor.
		isMoving = false;
	}
	else {
		enableRelays(true);
	}
	return isMoving;
}

void Mix_Valve::turnValveOff() { // Move valve to cool to prevent gravity circulation
	if (_valvePos == 0) _valvePos = 2;
	_motorDirection = e_Cooling;
	I2C_Flags_Ref(*registers().ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL);
	logger() << name() << L_tabs << F("Turn Valve OFF:") << _valvePos << L_endl;
}

// New Temp Request Functions
bool Mix_Valve::checkForNewReqTemp() { // called every second
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	auto reg = registers();
	const auto thisReqTemp = reg.get(R_REQUEST_FLOW_TEMP);
	//logger() << millis() << L_tabs << name() << F("This-req:") << thisReqTemp << F("Curr:") << _currReqTemp << L_endl;
	if (thisReqTemp != 0 && thisReqTemp != _currReqTemp) {
		logger() << name() << L_tabs;
		if (_newReqTemp != thisReqTemp) {
			logger() << F("NewReq:") << thisReqTemp << L_endl;
			_newReqTemp = thisReqTemp; // REQUEST TWICE TO REGISTER.
			reg.set(R_REQUEST_FLOW_TEMP, 0);
		}
		else {
			logger() << F("Confirmed:") << _newReqTemp << F("Curr:") << _currReqTemp << L_endl;
			_currReqTemp = _newReqTemp;
			saveToEEPROM();
			_newRequest = true;
			return true;
		}
	}
	return false;
}

Mix_Valve::Mode Mix_Valve::newTempMode() {
	//auto sensorTemp = reg.get(R_FLOW_TEMP) + reg.get(R_FLOW_TEMP_FRACT) / 256.;
	//bool isTooWarm = sensorTemp > _currReqTemp;
	//bool isTooCool = sensorTemp < _currReqTemp;
	auto mode = e_WaitingToMove;
	//if (_valvePos == 0 && isTooWarm) {
	//	startWaiting(); // Keep waiting whilst the pump cools the temp sensor
	//	return e_WaitToCool;
	//}
	//else if (_onTime != 0) { // Re-start waiting and see what happens. 
	//	startWaiting();
	//	mode = e_WaitingToMove;
	//} // is now waiting or checking
	//if (_journey != e_TempOK) {
	//	if (isTooCool) { // allow wait to be interrupted
	//		_journey = e_GoingWarm;
	//	}
	//	else if (isTooWarm) { // allow wait to be interrupted
	//		_journey = e_GoingCool;
	//	}
	//	else
			//_journey = e_NewReq;

	//}
	return mode;
}

// Non-Algorithmic Functions

bool Mix_Valve::valveIsAtLimit() {
	if (!_heat_relay->logicalState() && !_cool_relay->logicalState())
		return false;

	auto isOff = [this]() {
		auto psuV = 0;
		int offCount = 5;
		do {
			psuV = measurePSUVoltage();
			if (psuV > _motorsOffV - _motors_off_diff_V) {
				--offCount;
				if (offCount > 1) delay_mS(100);
			}
			else offCount = 0;
		} while (offCount > 1);
		return offCount == 1;
	};

	if (isOff()) {
		if (_motorDirection == e_Cooling) {
			_valvePos = 0;
		}
		else {
			if (_valvePos > 100) {
				registers().set(R_HALF_TRAVERSE_TIME, uint8_t(_valvePos / 2));
				_ep->update(_regOffset + R_HALF_TRAVERSE_TIME, uint8_t(_valvePos / 2));
			}
			else return false;
		}
		return true;
	}
	return false;
}

void Mix_Valve::refreshRegisters() {
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	//lambda
	auto reg = registers();
	reg.set(R_MOTOR_ACTIVITY, _motorDirection); // Motor activity: e_Cooling, e_Stop, e_Heating
	reg.set(R_VALVE_POS, (uint8_t)_valvePos / 2);
}

bool Mix_Valve::doneI2C_Coms(I_I2Cdevice& programmer, bool newSecond) { // called every loop()
	auto reg = registers();
	auto device_State = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
	uint8_t ts_status = device_State.is(F_DS_TS_FAILED);
	if (device_State.is(F_NO_PROGRAMMER) && newSecond) device_State.set(F_I2C_NOW);

	if (device_State.is(F_I2C_NOW)) {
		ts_status = _temp_sensr.readTemperature();
		if (ts_status == _disabledDevice) {
			_temp_sensr.reset();
		}

		if (ts_status) logger() << F("TSAddr:0x") << L_hex << _temp_sensr.getAddress()
			<< F(" Err:") << I2C_Talk::getStatusMsg(_temp_sensr.lastError())
			<< F(" Status:") << I2C_Talk::getStatusMsg(ts_status) << L_endl;

		reg.set(R_FLOW_TEMP, _temp_sensr.get_fractional_temp() >> 8);
		reg.set(R_FLOW_TEMP_FRACT, _temp_sensr.get_fractional_temp() & 0xFF);
		device_State.clear(F_I2C_NOW);
		device_State.set(F_DS_TS_FAILED, ts_status);
		uint8_t clearState = 0;
		programmer.write(R_DEVICE_STATE, 1, &clearState);
	}
	return ts_status == _OK;
}

int Mix_Valve::measurePSUVoltage(int period_mS) {
	// 4v ripple. 3.3v when PSU off, 17-19v when motors off, 16v motor-on
	// Analogue val = 856-868 PSU on. 920-932 when off.
	// Detect peak voltage during cycle
	auto psuMaxV = 0;
#ifdef ZPSIM
	psuMaxV = _valvePos < 5 && _motorDirection == e_Cooling ? 980 : (_valvePos >= VALVE_TRANSIT_TIME && _motorDirection == e_Heating ? 980 : _motorsOffV - 2 * _motors_off_diff_V);
	//psuMaxV = 80;
#else
	auto testComplete = Timer_mS(period_mS);
	do {
		auto thisVoltage = analogRead(PSU_V_PIN);
		if (thisVoltage < 1024 && thisVoltage > psuMaxV) psuMaxV = thisVoltage;
	} while (!testComplete);
	registers().set(R_PSU_V, psuMaxV / PSUV_DIVISOR);
#endif
	//logger() << name() << L_tabs << F("PSU_V:") << L_tabs << psuMaxV << L_endl;
	return psuMaxV > 500 ? psuMaxV : 0;
}

Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag& heatRelay, Pin_Wag& coolRelay, EEPROMClass& ep, int reg_offset)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40),
	_heat_relay(&heatRelay),
	_cool_relay(&coolRelay),
	_ep(&ep),
	_regOffset(reg_offset)
{}

void Mix_Valve::begin(int defaultFlowTemp) {
	logger() << name() << F("MixValve begin") << L_endl;
	const bool writeDefaultsToEEPROM = _ep->read(_regOffset + R_VERSION_MONTH) != version_month || _ep->read(_regOffset + R_VERSION_DAY) != version_day;
	//const bool writeDefaultsToEEPROM = true;
	logger() << F("regOffset: ") << _regOffset << F(" Saved month: ") << _ep->read(_regOffset + R_VERSION_MONTH) << F(" Saved Day: ") << _ep->read(_regOffset + R_VERSION_DAY) << L_endl;
	auto reg = registers();
	if (writeDefaultsToEEPROM) {
		reg.set(R_VERSION_MONTH, version_month);
		reg.set(R_VERSION_DAY, version_day);
		reg.set(R_TS_ADDRESS, _temp_sensr.getAddress());
		reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME / 2);
		reg.set(R_SETTLE_TIME, 40);
		reg.set(R_DEFAULT_FLOW_TEMP, defaultFlowTemp);
		saveToEEPROM();
		logger() << F("Saved defaults") << F(" Write month: ") << version_month << F(" Read month: ") << _ep->read(_regOffset + R_VERSION_MONTH) << F(" Write Day: ") << version_day << F(" Read day: ") << _ep->read(_regOffset + R_VERSION_DAY) << L_endl;
	}
	else {
		loadFromEEPROM();
		logger() << F("Loaded from EEPROM") << L_endl;
	}
	reg.set(R_RATIO, 30);
	reg.set(R_FLOW_TEMP, HardwareInterfaces::MIN_FLOW_TEMP);
	reg.set(R_FLOW_TEMP_FRACT, 0);

	auto speedTest = I2C_SpeedTest(_temp_sensr);
	speedTest.fastest();

	auto psuOffV = measurePSUVoltage(200);
	if (psuOffV) _motorsOffV = psuOffV;

	logger() << F("OffV: ") << _motorsOffV << L_endl;
	reg.set(R_MODE, e_findOff);
	logger() << name() << F(" TS Speed:") << _temp_sensr.runSpeed() << L_endl;
}

const __FlashStringHelper* Mix_Valve::name() const {
	return (_heat_relay->port() == 11 ? F("US ") : F("DS "));
}

void Mix_Valve::saveToEEPROM() { // returns noOfBytes saved
	auto eepromRegister = _regOffset + R_TS_ADDRESS;
	auto reg = registers();
	_temp_sensr.setAddress(reg.get(R_TS_ADDRESS));
	_ep->update(eepromRegister, reg.get(R_TS_ADDRESS));
	_ep->update(++eepromRegister, reg.get(R_HALF_TRAVERSE_TIME));
	_ep->update(++eepromRegister, reg.get(R_SETTLE_TIME));
	_ep->update(++eepromRegister, reg.get(R_DEFAULT_FLOW_TEMP));
	_ep->update(++eepromRegister, reg.get(R_VERSION_MONTH));
	_ep->update(++eepromRegister, reg.get(R_VERSION_DAY));
	logger() << F("MixValve saveToEEPROM at reg ") << _regOffset << F(" to ") << (int)eepromRegister << L_tabs << (int)reg.get(R_HALF_TRAVERSE_TIME) << (int)reg.get(R_SETTLE_TIME)
		<< (int)reg.get(R_DEFAULT_FLOW_TEMP) << (int)reg.get(R_VERSION_MONTH) << (int)reg.get(R_VERSION_DAY) << L_endl;
}

void Mix_Valve::loadFromEEPROM() { // returns noOfBytes saved
	auto eepromRegister = _regOffset + R_TS_ADDRESS;
	auto reg = registers();
#ifdef TEST_MIX_VALVE_CONTROLLER
	reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME / 2);
	reg.set(R_SETTLE_TIME, 10);
	reg.set(R_DEFAULT_FLOW_TEMP, 55);
#else	
	_temp_sensr.setAddress(_ep->read(eepromRegister));
	reg.set(R_TS_ADDRESS, _temp_sensr.getAddress());
	reg.set(R_HALF_TRAVERSE_TIME, _ep->read(++eepromRegister));
	reg.set(R_SETTLE_TIME, _ep->read(++eepromRegister));
	reg.set(R_DEFAULT_FLOW_TEMP, _ep->read(++eepromRegister));
	if (reg.get(R_HALF_TRAVERSE_TIME) < (VALVE_TRANSIT_TIME / 2) - 20) reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME / 2);
#endif
	setDefaultRequestTemp();
}

void Mix_Valve::setDefaultRequestTemp() { // called by MixValve_Slave.ino when master/slave mode changes
	auto reg = registers();
	_currReqTemp = reg.get(R_DEFAULT_FLOW_TEMP);
	reg.set(R_REQUEST_FLOW_TEMP, _currReqTemp);
	I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_NO_PROGRAMMER);
}

void Mix_Valve::log() {
	auto reg = registers();

	auto showState = [this]() {
		auto reg = registers();
		switch (reg.get(Mix_Valve::R_MODE)) {
		case Mix_Valve::e_swapMutex: return F("SwM");
		case Mix_Valve::e_AtTargetPosition: return F("Ok");
		case Mix_Valve::e_WaitingToMove: return F("Mx");
		case Mix_Valve::e_newReq: return F("Req");
		case Mix_Valve::e_WaitToCool: return F("WtC");
		case Mix_Valve::e_ValveOff: return F("Off");
		case Mix_Valve::e_reachedLimit: return F("RLm");
		case Mix_Valve::e_HotLimit: return F("Lim");
		case Mix_Valve::e_completedMove: return F("Mok");
		case Mix_Valve::e_findOff:
		case Mix_Valve::e_Moving:
			switch (int8_t(reg.get(Mix_Valve::R_MOTOR_ACTIVITY))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
			case Mix_Valve::e_Cooling: return F("Cl");
			case Mix_Valve::e_Heating: return F("Ht");
			default: return F("Stp"); // Now stopped!
			}
		default: return F("SEr");
		}
	};

	logger() << name() << L_tabs << _currReqTemp << reg.get(R_FLOW_TEMP) + reg.get(R_FLOW_TEMP_FRACT) / 256.
		<< showState() << _valvePos << L_endl;
}

#ifdef SIM_MIXV
Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag& heatRelay, Pin_Wag& coolRelay, EEPROMClass& ep, int reg_offset
	, uint16_t timeConst, uint16_t delay, uint8_t maxTemp)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40)
	, _heat_relay(&heatRelay)
	, _cool_relay(&coolRelay)
	, _ep(&ep)
	, _regOffset(reg_offset)
	, _timeConst(timeConst)
	, _delay(delay)
	, _maxTemp(maxTemp)
{
	_delayLine.prime_n(40 * 16, _delay);
	_reportedTemp = getAverage(_delayLine) / 16;
};

void Mix_Valve::setFlowTempReg() { // must be called only once per second
	float delayTemp = getAverage(_delayLine) / 16.;
	auto reg = registers();
	_reportedTemp += (delayTemp - _reportedTemp) * (1.f - exp(-(1.f / _timeConst)));
	auto reported_16t = uint16_t((_reportedTemp + (1.f / 32.f)) * 256.f);
	reg.set(R_FLOW_TEMP, uint8_t(reported_16t >> 8));
	reg.set(R_FLOW_TEMP_FRACT, uint8_t(reported_16t & 0xF0));
}

void Mix_Valve::addTempToDelayIntegral() {
	_delayLine.push(finalTempForPosition_16ths());
	if (_delayLine.size() > _delay) _delayLine.popFirst();
}

#endif	
