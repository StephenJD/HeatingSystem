// This is the Multi-Master Arduino Mini Controller
#ifndef __AVR__
	#define SIM_MIXV
#endif

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

extern bool receivingNewData;
extern const uint8_t version_month;
extern const uint8_t version_day;

constexpr int PSU_V_PIN = A3;
// Motor Functions

Motor::Motor(HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay) 
	: _heat_relay(&_heat_relay), _cool_relay(&_cool_relay) 
{
	_heat_relay.begin();
	_cool_relay.begin();
};

uint8_t Motor::pos(PowerSupply& pwr) {
	if (_motion != e_Stop) {
		auto moveTime = pwr.powerPeriod();
		if (moveTime) {
			_pos += moveTime * _motion;
			if (_pos < 0) {
				_pos = 0;
			}
			else if (_pos > 254) {
				_pos = 254;
			}
			logger() << (heatPort() == 11 ? F("US") : F("DS")) << L_tabs << _pos << F("Time:") << millis() << L_endl;
			if (pwr.requstToRelinquish()) 
				stop(pwr);
		}
	}
	return uint8_t(_pos);
}

bool Motor::moving(Direction direction, PowerSupply& pwr) {
	//if (psu.available(this)) {
	if (_motion == e_Stop) {
		if (direction != e_Stop) {
			if (psu.available(this)) {
				start(direction);
				return true;
			}
		}
		return false;
	} else { // motor moving
		if (direction == e_Stop) {
			stop(pwr);
			return false;
		} else if (direction != _motion) { // change of direction
			start(direction);
		}
		pos(pwr);
		return _motion != e_Stop;
	}
}

void Motor::start(Direction direction) {
	logger() << name() << L_tabs
		<< F("starting at") << millis() << L_endl;
	_motion = direction;
	if (_motion == e_Heating && _heat_relay->logicalState() == false) {
		_cool_relay->clear(); _heat_relay->set();
	}
	else if (_motion == e_Cooling && _cool_relay->logicalState() == false) {
		_heat_relay->clear(); _cool_relay->set();
	}
}

void Motor::stop(PowerSupply& pwr) {
	if (_motion != e_Stop) {
		pos(pwr);
		_cool_relay->clear(); _heat_relay->clear();
		_motion = e_Stop;
		pwr.relinquishPower();
		logger() << name() << L_tabs
			<< F("Stopping at") << millis() << L_endl;
	}
}

// Mix_Valve Functions

uint16_t Mix_Valve::update(int newPos) {
#ifdef SIM_MIXV
	addTempToDelayIntegral();
	setFlowTempReg();
	if (_motor.heatPort() == 11) {
		psu.moveOn1Sec();
	}
#endif
	return stateMachine(newPos);
}

// Loop Functions
uint16_t Mix_Valve::currReqTemp_16() const {
	if (registers().get(R_MODE) == e_FindingOff)
		return uint16_t(HardwareInterfaces::MIN_FLOW_TEMP) << 8;
	else 
		return uint16_t(_currReqTemp) << 8; 
}


uint16_t Mix_Valve::stateMachine(int newPos) {
	// Gets new TempReq, finds OFF, stops at limit or moves towards requested posistion.
	// Lambda
	auto tryToMove = [this]() {
		if (_motor.moving(_journey, psu))
			return e_Moving;
		else return e_WaitingToMove;
	};

	//Algorithm
	auto reg = registers();
	auto sensorTemp = getSensorTemp_16() ;
	bool isTooCool = sensorTemp / 256.f < _currReqTemp;
	auto mode = reg.get(R_MODE);
	if (checkForNewReqTemp()) {
		if (_newReqTemp <= HardwareInterfaces::MIN_FLOW_TEMP) {
			mode = e_findOff;
		}
	}
	if (newPos > _endPos && mode == e_HotLimit) newPos = _endPos;
	bool newPosRequest = newPos != _endPos;
	do {
		switch (mode) {
		// Persistant Modes processed once each second
		case e_Moving:
			mode = tryToMove();
			if (mode == e_Moving && valveIsAtLimit()) {
				mode = e_reachedLimit;
			} else if (_motor.curr_pos() == _endPos) {
				stopMotor();
				mode = e_AtTargetPosition;
			}
			break;
		case e_WaitingToMove: // was blocked by Mutex
			mode = tryToMove();
			break;
		case e_ValveOff:
			if (isTooCool || _endPos != _motor.curr_pos()) 
				mode = tryToMove();
			break;
		case e_AtTargetPosition:
			break;
		case e_FindingOff:
			if (tryToMove() == e_Moving) {
				//logger() << "Finding off\n";
				if (valveIsAtLimit()) {
					mode = e_reachedLimit;
				}
			}
			break; 
		case e_HotLimit:
			if (!isTooCool) {
				I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL); // when flow temp increases due to gas boiler on.
				mode = tryToMove();
			}
			break;
		// transitional modes, immediatly processed
		case e_findOff:
			turnValveOff();
			tryToMove();
			mode = e_FindingOff;
			break;
		case e_reachedLimit:
			stopMotor();
			if (_motor.curr_pos() == 0) {
				mode = e_ValveOff;
				setDirection(); // ready to move to requested endpos.
			}
			else {
				_endPos = _motor.curr_pos();
				I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_STORE_TOO_COOL);
				mode = e_HotLimit;
			}
			break;
		}
	} while (mode >= e_findOff);
	if (newPosRequest) {
		_endPos = newPos;
		if (mode != e_FindingOff) {
			setDirection();
			if (mode >= e_ValveOff) /*2*/ {
				mode = tryToMove();;
			}
		}
	}	
	reg.set(R_MODE, mode);
	reg.set(R_MOTOR_ACTIVITY, _motor.direction()); // Motor activity: e_Cooling, e_Stop, e_Heating
	reg.set(R_VALVE_POS, _motor.curr_pos());

	//logger() << name() << L_tabs << F("Mode:") << mode << L_endl;
	return sensorTemp;
}

// Transition Functions

void Mix_Valve::stopMotor() {
	_motor.stop(psu);
	_journey = Motor::e_Stop;
	logger() << name() << L_tabs << "stopMotor\n";
	get_PSU_OffV();
}

void Mix_Valve::get_PSU_OffV() {
	psu.setOn(true);
	auto newOffV = measurePSUVoltage(NO_OF_EQUAL_PSU_CYCLES);
	if (newOffV) {
		_motorsOffV = newOffV;
		_motors_off_diff_V = uint16_t(_motorsOffV * ON_OFF_RATIO);
		auto reg = registers();
		reg.set(R_PSU_MIN_OFF_V, reg.get(R_PSU_MIN_V));
		reg.set(R_PSU_MAX_OFF_V, reg.get(R_PSU_MAX_V));
		logger() << name() << L_tabs << F("New OFF-V:") << _motorsOffV << L_endl;
	}
}

void Mix_Valve::turnValveOff() { // Move valve to cool to prevent gravity circulation
	_journey = Motor::e_Cooling;
	I2C_Flags_Ref(*registers().ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL);
	logger() << name() << L_tabs << F("Turn Valve OFF:") 
		<< _motor.curr_pos() 
		<< L_endl;
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
			return true;
		}
	}
	return false;
}

// Non-Algorithmic Functions

bool Mix_Valve::valveIsAtLimit() {
	if (_motor.direction() == Motor::e_Stop)
		return false;

	auto isOff = [this]() {
#ifndef SIM_MIXV
		if (digitalRead(LED_BUILTIN)) return false; // only measure when LED off.
#endif
		auto psuV = 0;
		int offCount = 5;
		do {
			psuV = measurePSUVoltage(NO_OF_EQUAL_PSU_CYCLES);
			if (psuV > _motorsOffV - _motors_off_diff_V) {
				--offCount;
				if (offCount > 1) delay_mS(100);
			}
			else offCount = 0;
		} while (offCount > 1);
		return offCount == 1;
	};

	if (isOff()) {
		if (_journey == Motor::e_Cooling) {
			_motor.setPosToZero();
		}
		else {
			if (_motor.curr_pos() > 140) {
				registers().set(R_HALF_TRAVERSE_TIME, uint8_t(_motor.curr_pos() / 2));
				_ep->update(_regOffset + R_HALF_TRAVERSE_TIME, uint8_t(_motor.curr_pos() / 2));
			}
		}
		return true;
	}
	return false;
}

uint16_t Mix_Valve::getSensorTemp_16() {
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	auto reg = registers();
	return reg.get(R_FLOW_TEMP) * 256 + reg.get(R_FLOW_TEMP_FRACT);
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

int Mix_Valve::measurePSUVoltage(int noOfCycles) {
	// 4v ripple. 3.3v when PSU off, 17-19v when motors off, 16v motor-on
	// Analogue val = 856-868 PSU on. 920-932 when off.
	// Detect peak voltage during cycle - unreliable detection
	// Detect min voltage during cycle
	auto psuMaxV = 0;
	auto psuMinV = 1024;
#ifdef ZPSIM
	if (_motor.curr_pos() == VALVE_TRANSIT_TIME)
		bool debug = false;
	psuMaxV = _motor.curr_pos() < 5 && _journey == Motor::e_Cooling ? 980 : (_motor.curr_pos() >= VALVE_TRANSIT_TIME && _journey == Motor::e_Heating ? 980 : _motorsOffV - 2 * _motors_off_diff_V);
	//psuMaxV = 80;
#else
	auto checkAgain = noOfCycles;
	auto timeout = Timer_mS(200);

	do {
		auto testComplete = Timer_mS(20);
		auto thisMaxV = 0;
		auto thisMinV = 1024;
		do {
			auto thisVoltage = analogRead(PSU_V_PIN);
			if (thisVoltage < 1024 && thisVoltage > thisMaxV) thisMaxV = thisVoltage;
			if (thisVoltage > 600 && thisVoltage < thisMinV) thisMinV = thisVoltage;
		} while (!testComplete);
		if (thisMaxV == psuMaxV) --checkAgain;
		else {
			psuMaxV = thisMaxV;
			psuMinV = thisMinV;
			checkAgain = noOfCycles;
		}
	} while (!timeout && checkAgain > 0);

	registers().set(R_PSU_MIN_V, psuMinV /4);
	registers().set(R_PSU_MAX_V, psuMaxV - 800);
#endif
	//logger() << name() << L_tabs << F("PSU_V:") << L_tabs << psuMaxV << L_endl;
	//return psuMaxV > 800 ? psuMaxV : 0;
	return psuMaxV > 700 ? psuMaxV : 0;
}

Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag& heatRelay, Pin_Wag& coolRelay, EEPROMClass& ep, int reg_offset)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40),
	_motor{ heatRelay,coolRelay },
	_ep(&ep),
	_regOffset(reg_offset)
{
}

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
		reg.set(R_DEFAULT_FLOW_TEMP, defaultFlowTemp);
		saveToEEPROM();
		logger() << F("Saved defaults") << F(" Write month: ") << version_month << F(" Read month: ") << _ep->read(_regOffset + R_VERSION_MONTH) << F(" Write Day: ") << version_day << F(" Read day: ") << _ep->read(_regOffset + R_VERSION_DAY) << L_endl;
	}
	else {
		loadFromEEPROM();
		logger() << F("Loaded from EEPROM") << L_endl;
	}
	reg.set(R_FLOW_TEMP, HardwareInterfaces::MIN_FLOW_TEMP);
	reg.set(R_FLOW_TEMP_FRACT, 0);
	_currReqTemp = defaultFlowTemp;
	auto speedTest = I2C_SpeedTest(_temp_sensr);
	speedTest.fastest();

	auto psuOffV = measurePSUVoltage(NO_OF_EQUAL_PSU_CYCLES);
	if (psuOffV) {
		_motorsOffV = psuOffV;
		_motors_off_diff_V = int16_t(psuOffV * ON_OFF_RATIO);
	}
	logger() << name() << L_tabs << F("OffV:") << _motorsOffV << L_endl;
	reg.set(R_MODE, e_findOff);
	stateMachine(0);
	logger() << name() << F(" TS Speed:") << _temp_sensr.runSpeed() << L_endl;
}

const __FlashStringHelper* Mix_Valve::name() const {
	return (_motor.heatPort() == 11 ? F("US ") : F("DS "));
}

void Mix_Valve::saveToEEPROM() { // returns noOfBytes saved
	auto eepromRegister = _regOffset + R_TS_ADDRESS;
	auto reg = registers();
	_temp_sensr.setAddress(reg.get(R_TS_ADDRESS));
	_ep->update(eepromRegister, reg.get(R_TS_ADDRESS));
	_ep->update(++eepromRegister, reg.get(R_HALF_TRAVERSE_TIME));
	_ep->update(++eepromRegister, reg.get(R_DEFAULT_FLOW_TEMP));
	_ep->update(++eepromRegister, reg.get(R_VERSION_MONTH));
	_ep->update(++eepromRegister, reg.get(R_VERSION_DAY));
	logger() << F("MixValve saveToEEPROM at reg ") << _regOffset << F(" to ") << (int)eepromRegister << L_tabs << (int)reg.get(R_HALF_TRAVERSE_TIME) 
		<< (int)reg.get(R_DEFAULT_FLOW_TEMP) << (int)reg.get(R_VERSION_MONTH) << (int)reg.get(R_VERSION_DAY) << L_endl;
}

void Mix_Valve::loadFromEEPROM() { // returns noOfBytes saved
	auto eepromRegister = _regOffset + R_TS_ADDRESS;
	auto reg = registers();
#ifdef TEST_MIX_VALVE_CONTROLLER
	reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME / 2);
	reg.set(R_DEFAULT_FLOW_TEMP, 55);
#else	
	_temp_sensr.setAddress(_ep->read(eepromRegister));
	reg.set(R_TS_ADDRESS, _temp_sensr.getAddress());
	reg.set(R_HALF_TRAVERSE_TIME, _ep->read(++eepromRegister));
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

void Mix_Valve::log() const {
		//e_Moving, e_WaitingToMove, e_ValveOff, e_AtTargetPosition, e_FindingOff, e_HotLimit
		///*These are temporary triggers */, e_findOff, e_reachedLimit, e_Error

	auto reg = registers();

	auto showState = [this]() {
		auto reg = registers();
		switch (reg.get(Mix_Valve::R_MODE)) {
		case Mix_Valve::e_Moving:
			switch (int8_t(reg.get(Mix_Valve::R_MOTOR_ACTIVITY))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
			case Motor::e_Cooling: return F("Cl");
			case Motor::e_Heating: return F("Ht");
			default: return F("Stp"); // Now stopped!
			}
		case Mix_Valve::e_WaitingToMove: return F("Mx");
		case Mix_Valve::e_ValveOff: return F("Off");
		case Mix_Valve::e_AtTargetPosition: return F("Ok");
		case Mix_Valve::e_FindingOff:
		case Mix_Valve::e_HotLimit: return F("Lim");
		default: return F("SEr");
		}
	};

	logger() << name() << L_tabs << F("Req:") << _currReqTemp << F("Is:") << reg.get(R_FLOW_TEMP) + reg.get(R_FLOW_TEMP_FRACT) / 256.
		<< showState() << F(" ReqPos:") << _endPos << F(" IsPos:") << _motor.curr_pos();
}

#ifdef SIM_MIXV
Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag& heatRelay, Pin_Wag& coolRelay, EEPROMClass& ep, int reg_offset
	, uint16_t timeConst, uint8_t delay, uint8_t maxTemp)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40)
	, _motor {heatRelay, coolRelay}
	, _ep(&ep)
	, _regOffset(reg_offset)
	, _timeConst(timeConst)
	, _delay(delay)
	, _maxTemp(maxTemp)
{
	_delayLine.prime_n(40 * 256, _delay);
	_reportedTemp = getAverage(_delayLine) / 256.f;
};

void Mix_Valve::setFlowTempReg() { // must be called only once per second
	float delayTemp = getAverage(_delayLine) / 256.f;
	auto reg = registers();
	_reportedTemp += (delayTemp - _reportedTemp) * (1.f - exp(-(1.f / _timeConst)));
	auto reported_16t = uint16_t((_reportedTemp + (1.f / 32.f)) * 16) * 16;
	reg.set(R_FLOW_TEMP, uint8_t(reported_16t >> 8));
	reg.set(R_FLOW_TEMP_FRACT, uint8_t(reported_16t & 0xF0));
}

void Mix_Valve::addTempToDelayIntegral() {
	_delayLine.push(finalTempForPosition());
	if (_delayLine.size() > _delay) _delayLine.popFirst();
	if (_timeAtOneTemp > 0) --_timeAtOneTemp;
}

void Mix_Valve::setIsTemp(uint8_t temp) {
	_delayLine.prime_n(temp * 256, _delay);
	_reportedTemp = getAverage(_delayLine) / 256.f;
}
#endif	
