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
extern bool receivingNewData;
extern const uint8_t version_month;
extern const uint8_t version_day;

Mix_Valve * Mix_Valve::motor_mutex = 0;

Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag & heatRelay, Pin_Wag & coolRelay, EEPROMClass & ep, int reg_offset)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40),
	_heat_relay(&heatRelay),
	_cool_relay(&coolRelay),
	_ep(&ep),
	_regOffset(reg_offset)
	{}

void Mix_Valve::begin(int defaultFlowTemp) {
	logger() << F("MixValve begin") << L_endl;
	const bool writeDefaultsToEEPROM = _ep->read(_regOffset + R_VERSION_MONTH) != version_month || _ep->read(_regOffset + R_VERSION_DAY) != version_day;
	//const bool writeDefaultsToEEPROM = true;
	logger() << F("regOffset: ") << _regOffset << F(" Saved month: ") << _ep->read(_regOffset + R_VERSION_MONTH) << F(" Saved Day: ") << _ep->read(_regOffset + R_VERSION_DAY) << L_endl;
	auto reg = registers();
	if (writeDefaultsToEEPROM) {
		reg.set(R_VERSION_MONTH, version_month);
		reg.set(R_VERSION_DAY, version_day);
		reg.set(R_TS_ADDRESS, _temp_sensr.getAddress());
		reg.set(R_FULL_TRAVERSE_TIME, 255);
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
	reg.set(R_DEVICE_STATE, 0);
	_cool_relay->set(false);
	_heat_relay->set(false);
	enableRelays(false);

	auto speedTest = I2C_SpeedTest(_temp_sensr);
	speedTest.fastest();
	logger() << F("TS Speed:") << _temp_sensr.runSpeed() << L_endl;
}

const __FlashStringHelper* Mix_Valve::name() {
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

Mix_Valve::Mode Mix_Valve::algorithmMode(int call_flowDiff) const {
	auto reg = registers();
	bool dontWantHeat = _currReqTemp <= e_MIN_FLOW_TEMP;
	bool valveIsAtLimit = (_valvePos == 0 && dontWantHeat) || (_valvePos == reg.get(R_FULL_TRAVERSE_TIME) && call_flowDiff >= 0);

	if (I2C_Flags_Obj{ reg.get(R_DEVICE_STATE) }.is(F_NEW_TEMP_REQUEST)) return e_NewReq;
	else if (valveIsAtLimit) return e_AtLimit;
	else if (dontWantHeat) return e_DontWantHeat;
	else if (_onTime > 0) {
		if (motor_mutex && motor_mutex != this) return e_Mutex;
		else return e_Moving;
	} else if (_onTime < 0) return e_Wait;
	else return e_Checking;
}

bool Mix_Valve::doneI2C_Coms(I_I2Cdevice& programmer, bool newSecond) {
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

void Mix_Valve::check_flow_temp() { // Called once every second. maintains mix valve temp.
	auto reg = registers();
	auto sensorTemp = reg.get(R_FLOW_TEMP);
 
	checkForNewReqTemp();

	// lambdas
	auto turnValveOff = [&reg, sensorTemp, this]() { // Move valve to cool to prevent gravity circulation
		_onTime = reg.get(R_FULL_TRAVERSE_TIME);
		if (sensorTemp <= e_MIN_FLOW_TEMP) _onTime /= 2;
		_journey = e_Moving_Coolest;
		_motorDirection = e_Cooling;
		logger() << name() << L_tabs << F("Turn Valve OFF:") << _valvePos << L_endl;
	};

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
	int call_flowDiff = _currReqTemp - sensorTemp;
	serviceMotorRequest(); // -> wait if finished moving and -> check if finished waiting

	switch (algorithmMode(call_flowDiff)) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
	case e_NewReq:
		logger() << name() << L_tabs << F("New Temp Req:") << _currReqTemp <<  L_endl;
		if (_onTime >= 0 && _valvePos == 0 && sensorTemp >= _currReqTemp) {
			startWaiting(); // keep waiting whilst the pump is cooling the temp sensor
			break;
		//} else if (_journey == e_TempOK) { // picked up by e_Checking:
		} else /*if (call_flowDiff == 0)*/ { // happens to be off target, but at the new temp. 
			startWaiting();
		//} else if (_currReqTemp == _moveFrom_Temp) {// let it continue moving or waiting		
		} /*else if (_valvePos != reg.get(R_FULL_TRAVERSE_TIME)) {
			adjustValve(_currReqTemp - _moveFrom_Temp);
		}*/
		I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).clear(F_NEW_TEMP_REQUEST);
		break;
	case e_AtLimit: // false as soon as it overshoots 
		if (_currReqTemp > e_MIN_FLOW_TEMP && _onTime == 0) startWaiting();
		break;
	case e_DontWantHeat:
		if (_journey != e_Moving_Coolest) turnValveOff();
		break;
	case e_Moving: 
	case e_Mutex:
		if (call_flowDiff * _motorDirection <= 0) startWaiting();  // Got there early during a move
		break;
	case e_Wait:
		if (call_flowDiff * _journey < 0) { // Overshot during wait
			reg.set(R_RATIO, (reg.get(R_RATIO) * 2) / 3);
			adjustValve(call_flowDiff);
		} else if (call_flowDiff * _journey > 0 && (_flowTempAtStartOfWait - sensorTemp) * _journey > 0) { // Got worse (undershot) during wait
			//if ((_moveFrom_Temp - sensorTemp) * _journey > 0) { // worse than _moveFrom_Temp
			//	_moveFrom_Temp = sensorTemp;
			//	_moveFromPos = _valvePos;
			//}
			adjustValve(call_flowDiff);
			//_onTimeRatio *= 2; // ratio now reflects what has happened so far.
		} else if (_flowTempAtStartOfWait != sensorTemp) startWaiting(); // restart waiting if temp has changed.
		break;
	case e_Checking: 
		if (call_flowDiff == 0) {
			if (_journey != e_TempOK) {
				_journey = e_TempOK;
				calcRatio();
			}
		} else {
			if (_journey == e_TempOK) { // Previously OK temperature has drifted
				reg.set(R_FROM_TEMP, sensorTemp);
				adjustValve(call_flowDiff); // action becomes e_Moving
			}
			else { // Undershot after a wait - last adjustment was too small
				auto oldRatio = reg.get(R_RATIO);
				if ((reg.get(R_FROM_TEMP) - sensorTemp) * _journey < 0) { // got nearer during last move
					reg.set(R_RATIO, reg.get(R_RATIO) / 2);
				} else reg.set(R_FROM_TEMP, sensorTemp); // worse or no better
				adjustValve(call_flowDiff);
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

	int16_t onTime = curr_ratio * tempError;
	_onTime = reg.get(R_FULL_TRAVERSE_TIME) / 2;
	if (onTime < _onTime) _onTime = onTime;
}

bool Mix_Valve::activateMotor() {
	if (!motor_mutex || motor_mutex == this) { // we have control
		if (_motorDirection == e_Stop) motor_mutex = 0;
		else motor_mutex = this;
		_cool_relay->set(_motorDirection == e_Cooling); // turn Cool relay on/off
		_heat_relay->set(_motorDirection == e_Heating); // turn Heat relay on/off
		enableRelays(_motorDirection != e_Stop);
		return true;
	} else {
		return false;
	}
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
	if (_onTime > 0) {
		if (activateMotor()) {
			auto traverseTime = reg.get(R_FULL_TRAVERSE_TIME);
			logger() << name() << L_tabs << F("ValveMoving") << (_motorDirection == e_Heating ? "H" : (_motorDirection == e_Stop ? "Off" : "C")) << _onTime << F("ValvePos:") << _valvePos << F("Temp:") << reg.get(R_FLOW_TEMP) << F("Call") << _currReqTemp << L_endl;
			--_onTime;
			_valvePos += _motorDirection;
			if (_onTime == 0) {
				auto i2C_status = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
				if (_valvePos <= 0) {
					_journey = e_TempOK;
					_valvePos = 0;
					stopMotor();
				} else if (_valvePos >= traverseTime) {
					i2C_status.set(F_STORE_TOO_COOL);
					_valvePos = traverseTime;
					stopMotor();
				} else {  // Turn motor off and wait
					if (_journey == e_Moving_Coolest) _onTime = traverseTime / 2;
					else {
						startWaiting();
						i2C_status.clear(F_STORE_TOO_COOL);
					}
				}
			} else if (_onTime % 10 == 0) {
				motor_mutex = 0; // Give up ownership every 10 seconds.
				_cool_relay->set(false);
				_heat_relay->set(false);
				enableRelays(false);
			}
		}
	}
	else if (_onTime < 0)  {
		logger() << name() << L_tabs << F("WaitSettle:\t") << _onTime << F("\tPos:") << _valvePos << F("\tTemp:") << reg.get(R_FLOW_TEMP) << L_endl;
		++_onTime;
	}
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
	reg.set(R_MODE, mode); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
	reg.set(R_COUNT, abs(_onTime));
	reg.set(R_MOTOR_ACTIVITY, motorActivity()); // Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
	reg.set(R_VALVE_POS, (uint8_t)_valvePos);
	reg.set(R_FROM_POS, (uint8_t)_moveFromPos);
}

void Mix_Valve::checkForNewReqTemp() {
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	auto reg = registers();
	auto reqTempReg = reg.get(R_REQUEST_FLOW_TEMP);
	//logger() << F("R_REQUEST_FLOW_TEMP from: ") << _regOffset + R_REQUEST_FLOW_TEMP << F(" reg:") << reqTempReg << F(" Curr : ") << _currReqTemp << L_endl;
	if (reqTempReg != 0 && reqTempReg != _currReqTemp) {
		logger() << F("R_REQUEST_FLOW_TEMP HasBeenWrittenTo") << L_endl;
		const auto newReqTemp = reg.get(R_REQUEST_FLOW_TEMP);
		logger() << F("Reg:") << _regOffset + R_REQUEST_FLOW_TEMP << F(" newReq:") << newReqTemp << F(" lastReq:") << _newReqTemp << F(" Curr:") << _currReqTemp <<  L_endl;
		if (_newReqTemp != newReqTemp) {
			_newReqTemp = newReqTemp; // REQUEST TWICE TO REGISTER.
			reg.set(R_REQUEST_FLOW_TEMP, 0);
		} else if (_currReqTemp != newReqTemp) {
			_currReqTemp = newReqTemp;
			auto i2c_status = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
			if (newReqTemp > e_MIN_FLOW_TEMP) {
				i2c_status.set(F_NEW_TEMP_REQUEST);
			} else i2c_status.clear(F_NEW_TEMP_REQUEST);

			saveToEEPROM();
		}
	}
}