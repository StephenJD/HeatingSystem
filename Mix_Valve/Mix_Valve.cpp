// This is the Multi-Master Arduino Mini Controller

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

Mix_Valve * Mix_Valve::motor_mutex = 0;
uint16_t Mix_Valve::_motorsOffV = 675;

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

	if (reg.get(R_FULL_TRAVERSE_TIME) < 50) reg.set(R_FULL_TRAVERSE_TIME, 50);
	auto speedTest = I2C_SpeedTest(_temp_sensr);
	speedTest.fastest();
	auto oneCycleComplete = Timer_mS(20);
	auto newVoltage = 0;
	auto badCount = 10;
	do {
		auto thisVoltage = analogRead(A1);
		if (thisVoltage < 675 && thisVoltage > newVoltage) newVoltage = thisVoltage;
		if (thisVoltage > 675) {
			oneCycleComplete.restart();
			logger() << "Bad AnalogueRead" << L_endl;
			badCount--;
		}
	} while (badCount && !oneCycleComplete);
	//_motorsOffV = newVoltage;
	//TODO: Fix unreliable analogue readings
	_motorsOffV = 630;
	logger() << F("OffV: ") << _motorsOffV << L_endl;
	turnValveOff();
	logger() << name() << F(" TS Speed:") << _temp_sensr.runSpeed() << L_endl;
	//showFindOffReg();
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
#endif
	setDefaultRequestTemp();
}

void Mix_Valve::setDefaultRequestTemp() { // called by MixValve_Slave.ino when master/slave mode changes
	auto reg = registers();
	_currReqTemp = reg.get(R_DEFAULT_FLOW_TEMP);
	reg.set(R_REQUEST_FLOW_TEMP, _currReqTemp);
	I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).set(F_NO_PROGRAMMER);
}

Mix_Valve::Mode Mix_Valve::algorithmMode(int needIncreaseBy_deg) const {
	//{e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating,
	auto reg = registers();
	bool dontWantHeat = _currReqTemp <= e_MIN_FLOW_TEMP;
	bool valveIsOff = (_valvePos == 0 && dontWantHeat);
	auto deviceState = I2C_Flags_Obj{ reg.get(R_DEVICE_STATE) };
	if (_findOffPos) {
		return e_StopHeating;
	}
	else if (deviceState.is(F_NEW_TEMP_REQUEST)) return e_NewReq;
	else if (valveIsOff) return e_ValveOff;
	else if (_valvePos == 0  && needIncreaseBy_deg < 0) return e_WaitToCool;
	else if (_valvePos >= reg.get(R_FULL_TRAVERSE_TIME) && needIncreaseBy_deg >= 0) return e_HotLimit;
	else if (dontWantHeat) return e_StopHeating;
	else if (_onTime > 0) {
		if (motor_mutex && motor_mutex != this) return e_Mutex;
		else return e_Moving;
	} else if (_onTime < 0) return e_Wait;
	else return e_Checking;
}

void Mix_Valve::turnValveOff() { // Move valve to cool to prevent gravity circulation
	_onTime = 160;
	if (_valvePos == 0) _valvePos = 20;
	_journey = e_Moving_Coolest;
	_motorDirection = e_Cooling;
	_findOffPos = true;
	logger() << name() << L_tabs << F("Turn Valve OFF:") << _valvePos << L_endl;
}

void Mix_Valve::check_flow_temp() { // Called once every second. maintains mix valve temp.
	auto reg = registers();
	if (_findOffPos) {
		logger() << name() << F(" FindOff") << L_endl;
		serviceMotorRequest();
		refreshRegisters();
		return;
	}
	auto sensorTemp = reg.get(R_FLOW_TEMP);
	checkForNewReqTemp();

	// lambdas
	auto calcRatio = [&reg,sensorTemp, this]() {
		auto tempChange = sensorTemp - reg.get(R_FROM_TEMP);
		auto posChange = _valvePos - _moveFromPos;
		bool inMidRange = _valvePos > reg.get(R_FULL_TRAVERSE_TIME) / 4 || _valvePos < int(reg.get(R_FULL_TRAVERSE_TIME)) * 3 / 4;
		if (inMidRange && posChange != 0 && tempChange != 0) {
			auto newRatio = posChange / tempChange;
			if (newRatio > e_MIN_RATIO) reg.set(R_RATIO, newRatio);
		}
		reg.set(R_FROM_TEMP, sensorTemp);
		_moveFromPos = _valvePos;
	};

	// Algorithm
	/* As thermal store cools or heats, the temp will drift off target.
	* Assuming the store is hot enough for the requested temp, the correction for the drift gives the current ratio.
	* Likewise the move required for a new temp request gives the current ratio.
	* Changes & overshoots in the ratio whilst adjusting temp do not matter; it is overshoots in valve-pos that are relevant.
	* After settling on correct temp, ratio calc should predict next adjustment required.
	* Ratio should not be changed if valve goes to limit or returns from limit.
	* 1 degree overshoots are good for speed of response.
	*/
	auto needIncreaseBy_deg = int(_currReqTemp) - int(sensorTemp);
	if (_onTime != 0) serviceMotorRequest(); // wait or move
	//{e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating,
	auto mode = algorithmMode(needIncreaseBy_deg);
	//logger() << name() << F("Need increseBy: ") << needIncreaseBy_deg << F(" Mode: ") << mode << L_endl;
	switch (mode) {
	case e_NewReq:
		logger() << name() << L_tabs << F("New Temp Req:") << _currReqTemp <<  L_endl;
		I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).clear(F_NEW_TEMP_REQUEST);
		if (_valvePos == 0 && sensorTemp >= _currReqTemp) {
			startWaiting(); // Keep waiting whilst the pump cools the temp sensor
			break;
		} else if (_onTime != 0) { // Re-start waiting and see what happens. 
			startWaiting();
		}
		if (needIncreaseBy_deg > 0) { // allow wait to be interrupted
			_journey = e_WarmSouth;
		} else if (needIncreaseBy_deg < 0) { // allow wait to be interrupted
			_journey = e_CoolNorth;
		}
		_flowTempAtStartOfWait = sensorTemp;
		break;
	case e_ValveOff:
	case e_HotLimit:
	case e_WaitToCool:  // Want heat, but is too warm, and at cool limit.
		break;
	case e_StopHeating:
		if (_journey != e_Moving_Coolest) turnValveOff();
		break;
	case e_Moving: 
	case e_Mutex:
		if (needIncreaseBy_deg * _motorDirection <= 0) startWaiting();  // Got there early during a move
		break;
	case e_Wait:
		if (needIncreaseBy_deg * _journey < 0) { // Overshot during wait
			reg.set(R_RATIO, (reg.get(R_RATIO) * 2) / 3);
			adjustValve(needIncreaseBy_deg);
		} else if (needIncreaseBy_deg * _journey > 0 && (_flowTempAtStartOfWait - sensorTemp) * _journey > 0) { // Got worse (undershot) during wait
			//if ((_moveFrom_Temp - sensorTemp) * _journey > 0) { // worse than _moveFrom_Temp
			//	_moveFrom_Temp = sensorTemp;
			//	_moveFromPos = _valvePos;
			//}
			adjustValve(needIncreaseBy_deg);
			//_onTimeRatio *= 2; // ratio now reflects what has happened so far.
		} else if (_flowTempAtStartOfWait != sensorTemp) startWaiting(); // restart waiting if temp has changed.
		break;
	case e_Checking: 
		if (needIncreaseBy_deg == 0) {
			if (_journey != e_TempOK) {
				_journey = e_TempOK;
				calcRatio();
			}
		} else {
			if (_journey == e_TempOK) { // Previously OK temperature has drifted
				reg.set(R_FROM_TEMP, sensorTemp);
				adjustValve(needIncreaseBy_deg); // action becomes e_Moving
			}
			else { // Undershot after a wait - last adjustment was too small
				auto oldRatio = reg.get(R_RATIO);
				if ((reg.get(R_FROM_TEMP) - sensorTemp) * _journey < 0) { // got nearer during last move
					reg.set(R_RATIO, reg.get(R_RATIO) / 2);
				} else reg.set(R_FROM_TEMP, sensorTemp); // worse or no better
				adjustValve(needIncreaseBy_deg);
				reg.add(R_RATIO, oldRatio);
			} 
		}
		break;
	}
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
	int16_t maxOnTime = reg.get(R_FULL_TRAVERSE_TIME) / 2;
	if (_onTime > maxOnTime) _onTime = maxOnTime;
}

bool Mix_Valve::activateMotor() {
	static auto motorQueued = false;
	auto isMoving = false;
	if (_motorDirection == e_Stop) { 
		if (motor_mutex == this) { // If other motor has mutex, this will already be stopped.
			_cool_relay->set(false);
			_heat_relay->set(false);
			enableRelays(motorQueued); // disable if no queued motor.
			motor_mutex = 0;
		}
		motorQueued = false;
	}
	else { // wanting to move
		if (motor_mutex == this && motorQueued && _onTime % 10 == 0) { // Give up ownership every 10 seconds.
			motor_mutex = 0;
			_cool_relay->set(false);
			_heat_relay->set(false);
		}
		else { 
			if (motor_mutex == 0 ) motor_mutex = this; // try to acquire ownership
			if (motor_mutex == this) { // we have control
				_cool_relay->set(_motorDirection == e_Cooling); // turn Cool relay on/off
				_heat_relay->set(_motorDirection == e_Heating); // turn Heat relay on/off
				enableRelays(true);
				isMoving = true;
			}
			else {
				motorQueued = true;
				logger() << name() << L_tabs << F("Mutex") << L_endl;
			}
		}
	}
	return isMoving;
}

void Mix_Valve::stopMotor() {
	_motorDirection = e_Stop;
	activateMotor();
}

void Mix_Valve::startWaiting() {
	auto reg = registers();
	_onTime = -reg.get(R_SETTLE_TIME);
	_flowTempAtStartOfWait = reg.get(R_FLOW_TEMP);
	stopMotor();
}

void Mix_Valve::serviceMotorRequest() {
	auto reg = registers();
	if (_onTime < 0) { // wait
		logger() << name() << L_tabs << F("WaitSettle:\t") << _onTime << F("\tPos:") << _valvePos << F("\tTemp:") << reg.get(R_FLOW_TEMP) << L_endl;
		++_onTime;
	} else if (_onTime > 0 && activateMotor()) { // this motor is moving
		logger() << name() << L_tabs << F("ValveMoving") << (_motorDirection == e_Heating ? "H" : (_motorDirection == e_Stop ? "Off" : "C")) << _onTime << F("ValvePos:") << _valvePos << F("Temp:") << reg.get(R_FLOW_TEMP) << F("Call") << _currReqTemp << L_endl;
		--_onTime;
		_valvePos += _motorDirection;
		if (_valvePos < 0) {
			_valvePos = 0;
		}
		else if (_valvePos > 255) {
			_valvePos = 255;
		}
		auto i2C_status = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
		auto motorIsAtLimit = valveIsAtLimit();
		if (motorIsAtLimit) {
			if (_motorDirection == e_Cooling) {
				_journey = e_TempOK;
			}
			else {
				i2C_status.set(F_STORE_TOO_COOL);
			}
			_onTime = 0;
			stopMotor();
		} else if (_onTime == 0) { // Newly zero. 
			if (_journey == e_Moving_Coolest) _onTime = 255;
			else { // Turn motor off and wait
				startWaiting();
				i2C_status.clear(F_STORE_TOO_COOL);
			}
		}
	}
}

bool Mix_Valve::valveIsAtLimit() {
	/*
	* If _findOffPos true, keep going until time run out.
	Return true if valvePos is 0 or limit
	*/
	if (_findOffPos) {
		if (_onTime == 0) {
			_findOffPos = false;
			_valvePos = 0;
			return true;
		} 
		return false;
	}
	else if (_motorDirection == e_Cooling) {
		if (_valvePos > registers().get(R_FULL_TRAVERSE_TIME)) _valvePos = registers().get(R_FULL_TRAVERSE_TIME);
		return _valvePos == 0;
	}
	else return _valvePos > registers().get(R_FULL_TRAVERSE_TIME) + 20;
	// Mini at 3.3 still uses 5v as the analogue ref. So max analogue reading is 675.
	//if (!relaysOn() || (!_cool_relay->logicalState() && !_heat_relay->logicalState())) {
	//	logger() << name() << F("Lim? AllOff") << L_endl;
	//	return false;
	//}
	//auto oneCycleComplete = Timer_mS(20);
	//auto newVoltage = 0;
	//do {
	//	auto thisVoltage = analogRead(A1);
	//	if (thisVoltage < 675 && thisVoltage > newVoltage) newVoltage = thisVoltage;
	//} while (!oneCycleComplete);
	//bool isOff = newVoltage > _motorsOffV - _MOTORS_ON_DIFF_V;
	//if (_findOffPos) {
	//	logger() << name() << L_tabs << newVoltage << F("OffV:") << _motorsOffV - _MOTORS_ON_DIFF_V << F("IsOff:") << isOff << L_endl;
	//	if (isOff && _valvePos == 0) {
	//		_findOffPos = false;
	//		logger() << name() << L_tabs << F("ClearFindOff") << L_endl;
	//	}
	//	else isOff = false;
	//} else if (_motorDirection == e_Heating) {
	// 		if( _valvePos < 50) isOff = false;
//			else {
//			reg.set(R_FULL_TRAVERSE_TIME, uint8_t(_valvePos));
//			_ep->update(_regOffset + R_FULL_TRAVERSE_TIME, uint8_t(_valvePos));
//				}
//}
	//return isOff;
}

void Mix_Valve::checkPumpIsOn() {
}

void Mix_Valve::refreshRegisters() {
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	//lambda
	auto motorActivity = [this]() -> uint8_t {if (_motorDirection == e_Cooling && _journey == e_Moving_Coolest) return e_Moving_Coolest; else return _motorDirection; };
	auto reg = registers();
	auto mode = algorithmMode(_currReqTemp - reg.get(R_FLOW_TEMP));
	//logger() << F("Mode:") << mode << L_endl;
	reg.set(R_MODE, mode); // e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_Error
	reg.set(R_COUNT, abs(_onTime));
	reg.set(R_MOTOR_ACTIVITY, motorActivity()); // Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
	reg.set(R_VALVE_POS, (uint8_t)_valvePos);
	reg.set(R_FROM_POS, (uint8_t)_moveFromPos);
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

void Mix_Valve::checkForNewReqTemp() { // called every second
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
			if (_newReqTemp > e_MIN_FLOW_TEMP) {
				i2c_status.set(F_NEW_TEMP_REQUEST);
			}
			else {
				i2c_status.clear(F_NEW_TEMP_REQUEST);
				//if (_valvePos == 0) _valvePos = 1; // make sure valve really is off
			}
			saveToEEPROM();
		}
	}
}