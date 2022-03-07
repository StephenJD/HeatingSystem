// This is the Multi-Master Arduino Mini Controller
#define TEST_MIXV

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
Mix_Valve * Mix_Valve::motor_mutex = 0;
uint16_t Mix_Valve::_motorsOffV = 1024;
bool Mix_Valve::motor_queued;

int Mix_Valve::measurePSUVoltage(int period_mS) {
	// 4v ripple. 3.3v when PSU off, 17-19v when motors off, 16v motor-on
	// Analogue val = 856-868 PSU on. 920-932 when off.
	// Detect peak voltage during cycle
	auto testComplete = Timer_mS(period_mS);
	auto psuMaxV = 0;
	do {
		auto thisVoltage = analogRead(PSU_V_PIN);
		if (thisVoltage > psuMaxV) psuMaxV = thisVoltage;
	} while (!testComplete);
#ifdef TEST_MIXV
	psuMaxV = _valvePos < 5 ? 700 : (_valvePos >= 100 ? 700 : 500);
#endif
	_psuV = psuMaxV/4;
	//logger() << name() << L_tabs << F("PSU_V:") << L_tabs << psuMaxV << L_endl;
	return psuMaxV > 100 ? psuMaxV : 0;
}

Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag & heatRelay, Pin_Wag & coolRelay, EEPROMClass & ep, int reg_offset)
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
		reg.set(R_FULL_TRAVERSE_TIME, 150);
		reg.set(R_SETTLE_TIME, 40);
		reg.set(R_DEFAULT_FLOW_TEMP, defaultFlowTemp);
		saveToEEPROM();
		logger() << F("Saved defaults") << F(" Write month: ") << version_month << F(" Read month: ") << _ep->read(_regOffset + R_VERSION_MONTH) << F(" Write Day: ") << version_day << F(" Read day: ") << _ep->read(_regOffset + R_VERSION_DAY) << L_endl;
	} else {
		loadFromEEPROM();
		logger() << F("Loaded from EEPROM") << L_endl;
	}
	reg.set(R_RATIO, 30);
	reg.set(R_FLOW_TEMP, e_MIN_FLOW_TEMP);

	auto speedTest = I2C_SpeedTest(_temp_sensr);
	speedTest.fastest();

	auto psuOffV = measurePSUVoltage(200);
	//if (psuOffV < _motorsOffV) _motorsOffV = psuOffV;
#ifdef TEST_MIXV
	_motorsOffV = 700;
#endif
	logger() << F("OffV: ") << _motorsOffV << L_endl;
	reg.set(R_MODE, e_FindOff);
	logger() << name() << F(" TS Speed:") << _temp_sensr.runSpeed() << L_endl;
}

const __FlashStringHelper* Mix_Valve::name() const {
	return (_heat_relay->port() == 11 ? F("US ") : F("DS "));
}

void Mix_Valve::saveToEEPROM() { // returns noOfBytes saved
	auto eepromRegister = _regOffset + R_TS_ADDRESS;
	auto reg = registers();
	_temp_sensr.setAddress(reg.get(R_TS_ADDRESS));
	_ep->update(  eepromRegister, reg.get(R_TS_ADDRESS));
	_ep->update(++eepromRegister, reg.get(R_FULL_TRAVERSE_TIME));
	_ep->update(++eepromRegister, reg.get(R_SETTLE_TIME));
	_ep->update(++eepromRegister, reg.get(R_DEFAULT_FLOW_TEMP));
	_ep->update(++eepromRegister, reg.get(R_VERSION_MONTH));
	_ep->update(++eepromRegister, reg.get(R_VERSION_DAY));
	logger() << F("MixValve saveToEEPROM at reg ") << _regOffset << F(" to ") << (int)eepromRegister << L_tabs << (int)reg.get(R_FULL_TRAVERSE_TIME) << (int)reg.get(R_SETTLE_TIME)
		<< (int)reg.get(R_DEFAULT_FLOW_TEMP) << (int)reg.get(R_VERSION_MONTH) << (int)reg.get(R_VERSION_DAY) << L_endl;
}

void Mix_Valve::loadFromEEPROM() { // returns noOfBytes saved
	auto eepromRegister = _regOffset + R_TS_ADDRESS;
	auto reg = registers();
#ifdef TEST_MIX_VALVE_CONTROLLER
	reg.set(R_FULL_TRAVERSE_TIME, 140);
	reg.set(R_SETTLE_TIME, 10);
	reg.set(R_DEFAULT_FLOW_TEMP, 55);
#else	
	_temp_sensr.setAddress(_ep->read(eepromRegister));
	reg.set(R_TS_ADDRESS, _temp_sensr.getAddress());
	reg.set(R_FULL_TRAVERSE_TIME, _ep->read(++eepromRegister));
	reg.set(R_SETTLE_TIME, _ep->read(++eepromRegister));
	reg.set(R_DEFAULT_FLOW_TEMP, _ep->read(++eepromRegister));
	if (reg.get(R_FULL_TRAVERSE_TIME) < 120) reg.set(R_FULL_TRAVERSE_TIME, 150);
#endif
	setDefaultRequestTemp();
}

void Mix_Valve::setDefaultRequestTemp() { // called by MixValve_Slave.ino when master/slave mode changes
	auto reg = registers();
	_currReqTemp = reg.get(R_DEFAULT_FLOW_TEMP);
	reg.set(R_REQUEST_FLOW_TEMP, _currReqTemp);
	I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_NO_PROGRAMMER);
}

void Mix_Valve::stateMachine() {
	/* As thermal store cools or heats, the temp will drift off target.
	* Assuming the store is hot enough for the requested temp, the correction for the drift gives the current ratio.
	* Likewise the move required for a new temp request gives the current ratio.
	* Changes & overshoots in the ratio whilst adjusting temp do not matter; it is overshoots in valve-pos that are relevant.
	* After settling on correct temp, ratio calc should predict next adjustment required.
	* Ratio should not be changed if valve goes to limit or returns from limit.
	* 1 degree overshoots are good for speed of response.
	*/
	// Lambda
	auto moveMode = [this]() {
		if (motor_mutex == this) return e_Moving;
		else if (motor_mutex == 0) {
			motor_mutex = this;
			return e_Moving;
		} else {
			logger() << name() << L_tabs << F("Mutex") << L_endl;
			motor_queued = true;
			_cool_relay->set(false);
			_heat_relay->set(false);
			return e_Mutex;
		}
	};

	//Algorithm
	auto reg = registers();
	auto sensorTemp = registers().get(R_FLOW_TEMP);
	bool isTooCool = sensorTemp < _currReqTemp;
	auto needIncreaseBy_deg = int(_currReqTemp) - int(sensorTemp);
	auto mode = reg.get(R_MODE);
	if (_journey != e_Moving_Coolest && checkForNewReqTemp()) {
		if (_newReqTemp > e_MIN_FLOW_TEMP) {
			mode = e_NewReq;
		}
		else {
			mode = e_FindOff;
		}
	}
	switch (mode) {
	case e_NewReq:
		logger() << name() << L_tabs << F("New Temp Req:") << _currReqTemp << L_endl;
		mode = newTempMode();
		break;
	case e_Moving: // if not (_journey == e_Moving_Coolest) can interrupt by under/overshoot
		logger() << name() << L_tabs << F("ValveMoving") << (_motorDirection == e_Heating ? "H" : (_motorDirection == e_Stop ? "Off" : "C")) << _onTime << F("ValvePos:") << _valvePos << F("Temp:") << reg.get(R_FLOW_TEMP) << F("Call") << _currReqTemp << L_endl;
		if (_journey != e_Moving_Coolest && needIncreaseBy_deg * _motorDirection <= 0 || !continueMove()) {
			startWaiting();
			mode = e_Wait;
		}
		else {
			if (valveIsAtLimit()) {
				_onTime = 0;
				stopMotor();
				motor_mutex = 0; 
				if (_valvePos == 0) {
					mode = e_ValveOff;
					_journey = e_TempOK;
				} else {
					I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_STORE_TOO_COOL);
					mode = e_HotLimit;
				}
			}
			else if (motor_queued && _onTime % 10 == 0) {
				mode = e_Mutex;
				motor_mutex = 0;
			} else if (!activateMotor_isMoving()) motor_mutex = 0;
		}
		
		break;
	case e_Wait: // can interrupt by under/overshoot
		logger() << name() << L_tabs << F("WaitSettle:\t") << _onTime << F("\tPos:") << _valvePos << F("\tTemp:") << reg.get(R_FLOW_TEMP) << L_endl;
		if (needIncreaseBy_deg * _journey < 0) { // Overshot during wait
			reg.set(R_RATIO, (reg.get(R_RATIO) * 2) / 3);
			adjustValve(needIncreaseBy_deg);
			mode = e_Mutex;
		} else if (needIncreaseBy_deg * _journey > 0 && (_flowTempAtStartOfWait - sensorTemp) * _journey > 0) { // Got worse (undershot) during wait
			//if ((_moveFrom_Temp - sensorTemp) * _journey > 0) { // worse than _moveFrom_Temp
			//	_moveFrom_Temp = sensorTemp;
			//	_moveFromPos = _valvePos;
			//}
			adjustValve(needIncreaseBy_deg);
			mode = e_Mutex;
			//_onTimeRatio *= 2; // ratio now reflects what has happened so far.
		}
		else if (_flowTempAtStartOfWait != sensorTemp) startWaiting(); // restart waiting if temp has changed.
		else if (!continueWait()) mode = e_Checking;
		break;
	case e_Mutex: // try to move
		mode = moveMode();
		break;
	case e_Checking:
		if (!continueChecking()) mode = moveMode();
		break;
	case e_HotLimit:
		if (!isTooCool) {
			I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL);
			mode = e_Checking;
		}
		break;
	case e_WaitToCool:
	case e_ValveOff:
		if (isTooCool) mode = e_Checking;
		break;
	case e_FindOff: // Momentary mode
		if (_journey != e_Moving_Coolest) turnValveOff();
		mode = moveMode();
	}
	reg.set(R_MODE, mode);
	//logger() << F("Mode:") << mode << L_endl;
}

Mix_Valve::Mode Mix_Valve::newTempMode() {
	auto sensorTemp = registers().get(R_FLOW_TEMP);
	bool isTooWarm = sensorTemp > _currReqTemp;
	bool isTooCool = sensorTemp < _currReqTemp;
	auto mode = e_Checking;
	if (_valvePos == 0 && isTooWarm) {
		startWaiting(); // Keep waiting whilst the pump cools the temp sensor
		return e_WaitToCool;
	}
	else if (_onTime != 0) { // Re-start waiting and see what happens. 
		startWaiting();
		mode = e_Wait;
	} // is now waiting or checking
	if (isTooCool) { // allow wait to be interrupted
		_journey = e_WarmSouth;
	}
	else if (isTooWarm) { // allow wait to be interrupted
		_journey = e_CoolNorth;
	}
	else _journey = e_TempOK;
	_flowTempAtStartOfWait = sensorTemp;
	return mode;
}

void Mix_Valve::check_flow_temp() { // Called once every second. maintains mix valve temp.
	stateMachine();
	refreshRegisters();
}

void Mix_Valve::adjustValve(int tempDiff) {
	// Get required direction.
	if (tempDiff < 0) _motorDirection = e_Cooling;  // cool valve 
	else _motorDirection = e_Heating;  // heat valve
	
	_journey = Journey(_motorDirection);
	auto reg = registers();
	const auto curr_ratio = reg.get(R_RATIO);
	if (curr_ratio < e_MIN_RATIO) reg.set(R_RATIO, e_MIN_RATIO);
	else if (curr_ratio > e_MAX_RATIO) reg.set(R_RATIO, e_MAX_RATIO);
	logger() << name() << L_tabs << F("Move") << tempDiff << F("\tRatio:") << (int)curr_ratio << L_endl;
	auto tempError = abs(tempDiff);

	_onTime = curr_ratio * tempError;
	int16_t maxOnTime = reg.get(R_FULL_TRAVERSE_TIME) /* / 2*/;
	if (_onTime > maxOnTime) _onTime = maxOnTime;
}

bool Mix_Valve::activateMotor_isMoving() {
	auto isMoving = true;
	_cool_relay->set(_motorDirection == e_Cooling); // turn Cool relay on/off
	_heat_relay->set(_motorDirection == e_Heating); // turn Heat relay on/off
	if (_motorDirection == e_Stop) { 
		enableRelays(motor_queued); // disable if no queued motor.
		isMoving = false;
	} else {
		enableRelays(true);
	}
	return isMoving;
}

void Mix_Valve::turnValveOff() { // Move valve to cool to prevent gravity circulation
	_onTime = 511;
	if (_valvePos == 0) _valvePos = 2;
	_journey = e_Moving_Coolest;
	_motorDirection = e_Cooling;
	logger() << name() << L_tabs << F("Turn Valve OFF:") << _valvePos << L_endl;
}

void Mix_Valve::stopMotor() {
	_motorDirection = e_Stop;
	motor_queued = false;
	motor_mutex = 0;
	activateMotor_isMoving();
}

void Mix_Valve::startWaiting() {
	auto reg = registers();
	_onTime = -reg.get(R_SETTLE_TIME);
	_flowTempAtStartOfWait = reg.get(R_FLOW_TEMP);
	stopMotor();
}

bool Mix_Valve::continueMove() {
	--_onTime;
	_valvePos += _motorDirection;
	if (_valvePos < 0) {
		_valvePos = 0;
	}
	else if (_valvePos > 511) {
		_valvePos = 511;
	}
	return _onTime > 0;
}

bool Mix_Valve::continueWait() {
	++_onTime;
	if (_onTime < 0) return true;  else return false;
}

bool Mix_Valve::continueChecking() {
	auto reg = registers();
	auto sensorTemp = reg.get(R_FLOW_TEMP);
	// lambdas
	auto calcRatio = [&reg, sensorTemp, this]() {
		auto tempChange = sensorTemp - reg.get(R_FROM_TEMP);
		auto posChange = _valvePos - _moveFromPos;
		bool inMidRange = _valvePos > reg.get(R_FULL_TRAVERSE_TIME) / 2 || _valvePos < int(reg.get(R_FULL_TRAVERSE_TIME)) * 3 / 2;
		if (inMidRange && posChange != 0 && tempChange != 0) {
			auto newRatio = posChange / tempChange;
			if (newRatio > e_MIN_RATIO) reg.set(R_RATIO, newRatio);
		}
		reg.set(R_FROM_TEMP, sensorTemp);
		_moveFromPos = _valvePos;
	};

	// Algorithm
	auto needIncreaseBy_deg = int(_currReqTemp) - int(sensorTemp);
	if (needIncreaseBy_deg == 0) {
		if (_journey != e_TempOK) {
			_journey = e_TempOK;
			calcRatio();
		}
		return true;
	}
	else {
		if (_journey == e_TempOK) { // Previously OK temperature has drifted
			reg.set(R_FROM_TEMP, sensorTemp);
			adjustValve(needIncreaseBy_deg); // action becomes e_Moving
		}
		else { // Undershot after a wait - last adjustment was too small
			auto oldRatio = reg.get(R_RATIO);
			if ((reg.get(R_FROM_TEMP) - sensorTemp) * _journey < 0) { // got nearer during last move
				reg.set(R_RATIO, reg.get(R_RATIO) / 2);
			}
			else reg.set(R_FROM_TEMP, sensorTemp); // worse or no better
			adjustValve(needIncreaseBy_deg);
			reg.add(R_RATIO, oldRatio);
		}
		return false;
	}
}

bool Mix_Valve::valveIsAtLimit() {
	int isOff = 3;
	for (; isOff > 1;) {
		if (measurePSUVoltage() > _motorsOffV - _MOTORS_ON_DIFF_V) {
			if (--isOff > 1) delay(100);
		}
		else isOff = 0;
	}
	////////// Non-Measure PSU Version ////////////////
	if (_journey == e_Moving_Coolest) {
		if (_onTime == 0) {
			_valvePos = 0;
			logger() << name() << L_tabs << F("ClearFindOff") << L_endl;
			return true;
		}
		return false;
	}
	else if (_motorDirection == e_Cooling) {
		if (_valvePos > registers().get(R_FULL_TRAVERSE_TIME) * 2) _valvePos = registers().get(R_FULL_TRAVERSE_TIME) * 2;
		return _valvePos == 0;
	}
	else return _valvePos > (registers().get(R_FULL_TRAVERSE_TIME) * 2) + 20;

	if (isOff) {
		if (_motorDirection == e_Cooling) {
			_valvePos = 0;
			//_motorsOffV = psuV;
		} else {
			if (_valvePos > 100) {
				registers().set(R_FULL_TRAVERSE_TIME, uint8_t(_valvePos / 2));
				_ep->update(_regOffset + R_FULL_TRAVERSE_TIME, uint8_t(_valvePos / 2));
			} else isOff = 0;
		}
	}
	return isOff;
}

void Mix_Valve::refreshRegisters() {
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	//lambda
	auto motorActivity = [this]() -> uint8_t {if (_motorDirection == e_Cooling && _journey == e_Moving_Coolest) return e_Moving_Coolest; else return _motorDirection; };
	auto reg = registers();
	//reg.set(R_COUNT, abs(_onTime));
	reg.set(R_COUNT, _motorsOffV/4);
	reg.set(R_MOTOR_ACTIVITY, motorActivity()); // Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
	reg.set(R_VALVE_POS, (uint8_t)_valvePos/2);
	//.set(R_FROM_POS, (uint8_t)_moveFromPos);
	reg.set(R_FROM_POS, _psuV);
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

		reg.set(R_FLOW_TEMP, _temp_sensr.get_temp());
		device_State.clear(F_I2C_NOW);
		device_State.set(F_DS_TS_FAILED, ts_status);
		uint8_t clearState = 0;
		programmer.write(R_DEVICE_STATE, 1, &clearState);
	}
	return ts_status == _OK;
}

bool Mix_Valve::checkForNewReqTemp() { // called every second
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	auto reg = registers();
	const auto thisReqTemp = reg.get(R_REQUEST_FLOW_TEMP);
	//logger() << millis() << L_tabs << name() << F("This-req:") << thisReqTemp << F("Curr:") << _currReqTemp << L_endl;
	auto i2c_status = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
	if (thisReqTemp != 0 && thisReqTemp != _currReqTemp) {
		logger() << name() << L_tabs;
		if (_newReqTemp != thisReqTemp) {
			logger() << F("NewReq:") << thisReqTemp << L_endl;
			_newReqTemp = thisReqTemp; // REQUEST TWICE TO REGISTER.
			reg.set(R_REQUEST_FLOW_TEMP, 0);
		} else {
			logger() << F("Confirmed:") << _newReqTemp << F("Curr:") << _currReqTemp << L_endl;
			_currReqTemp = _newReqTemp;
			saveToEEPROM();
			return true;
		}
	}
	return false;
}