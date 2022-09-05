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

constexpr int PSU_V_PIN = A3;
Mix_Valve * Mix_Valve::motor_mutex = 0;
int16_t Mix_Valve::_motorsOffV = 1024;
int16_t Mix_Valve::_motors_off_diff_V = int16_t(_motorsOffV * 0.03);

bool Mix_Valve::motor_queued;

int Mix_Valve::measurePSUVoltage(int period_mS) {
	// 4v ripple. 3.3v when PSU off, 17-19v when motors off, 16v motor-on
	// Analogue val = 856-868 PSU on. 920-932 when off.
	// Detect peak voltage during cycle
	auto testComplete = Timer_mS(period_mS);
	auto psuMaxV = 0;
	do {
		auto thisVoltage = analogRead(PSU_V_PIN);
		if (thisVoltage < 1024 && thisVoltage > psuMaxV) psuMaxV = thisVoltage;
	} while (!testComplete);
#ifdef ZPSIM
	if (_motorDirection == e_Stop || _valvePos < 5 || _valvePos >= VALVE_TRANSIT_TIME) psuMaxV = 980;
	else psuMaxV =  860;
	//psuMaxV = 80;
#endif
	registers().set(R_PSU_V, psuMaxV / PSUV_DIVISOR);

	//logger() << name() << L_tabs << F("PSU_V:") << L_tabs << psuMaxV << L_endl;
	return psuMaxV > 500 ? psuMaxV : 0;
}

Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag & heatRelay, Pin_Wag & coolRelay, EEPROMClass & ep, int reg_offset)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40),
	_heat_relay(&heatRelay),
	_cool_relay(&coolRelay),
	_ep(&ep),
	_regOffset(reg_offset)
	{}

#ifdef SIM_MIXV
Mix_Valve::Mix_Valve(I2C_Recover& i2C_recover, uint8_t defaultTSaddr, Pin_Wag & heatRelay, Pin_Wag & coolRelay, EEPROMClass & ep, int reg_offset
	, uint16_t timeConst, uint16_t delay, uint8_t maxTemp)
	: _temp_sensr(i2C_recover, defaultTSaddr, 40)
	, _heat_relay(&heatRelay)
	, _cool_relay(&coolRelay)
	, _ep(&ep)
	, _regOffset(reg_offset)
	, _timeConst(timeConst)
	, _delayLine(delay)
	, _maxTemp(maxTemp)
{
	_delayLine.prime(40 * 16);
	_reportedTemp = _delayLine.getAverage() / 16;
};
#endif	

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
		reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME/2);
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
	if (psuOffV) _motorsOffV = psuOffV;

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
	reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME /2);
	reg.set(R_SETTLE_TIME, 10);
	reg.set(R_DEFAULT_FLOW_TEMP, 55);
#else	
	_temp_sensr.setAddress(_ep->read(eepromRegister));
	reg.set(R_TS_ADDRESS, _temp_sensr.getAddress());
	reg.set(R_HALF_TRAVERSE_TIME, _ep->read(++eepromRegister));
	reg.set(R_SETTLE_TIME, _ep->read(++eepromRegister));
	reg.set(R_DEFAULT_FLOW_TEMP, _ep->read(++eepromRegister));
	if (reg.get(R_HALF_TRAVERSE_TIME) < (VALVE_TRANSIT_TIME /2) - 20) reg.set(R_HALF_TRAVERSE_TIME, VALVE_TRANSIT_TIME/2);
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
			activateMotor_isMoving();
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
		logger() << name() << L_tabs << F("ValveMoving") << (_motorDirection == e_Heating ? "H" : (_motorDirection == e_Stop ? "Off" : "C")) << _onTime << F("ValvePos:") << _valvePos << F("Temp:") << reg.get(R_FLOW_TEMP) << F("Call") << _currReqTemp /*<< F("V:") << measurePSUVoltage()*/ << L_endl;
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
			if (reg.get(R_ADJUST_MODE) != A_UNDERSHOT) {
				reg.set(R_RATIO, (reg.get(R_RATIO) * 2) / 3);
				reg.set(R_ADJUST_MODE, A_OVERSHOT);
			}
			adjustValve(needIncreaseBy_deg);
			mode = e_Mutex;
		} else if (needIncreaseBy_deg * _journey > 0 && (_flowTempAtStartOfWait - sensorTemp) * _journey > 0) { // Got worse (undershot) during wait
			reg.set(R_ADJUST_MODE, A_UNDERSHOT);
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
			I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL); // when flow temp increases due to gas boiler on.
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
	if (tempDiff < 0) {
		_motorDirection = e_Cooling;  // cool valve
		I2C_Flags_Ref(*registers().ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL);
	}
	else _motorDirection = e_Heating;  // heat valve
	
	_journey = Journey(_motorDirection);
	auto reg = registers();
	const auto curr_ratio = reg.get(R_RATIO);
	if (curr_ratio < e_MIN_RATIO) reg.set(R_RATIO, e_MIN_RATIO);
	else if (curr_ratio > e_MAX_RATIO) reg.set(R_RATIO, e_MAX_RATIO);
	auto tempError = abs(tempDiff);

	_onTime = curr_ratio * tempError;
	int16_t maxOnTime = reg.get(R_HALF_TRAVERSE_TIME) /* / 2*/;
	if (_onTime > maxOnTime) _onTime = maxOnTime;
	logger() << name() << L_tabs << F("Move(deg)") << tempDiff << F("\tRatio:") << (int)curr_ratio << F("Time:") << _onTime << L_endl;
}

bool Mix_Valve::activateMotor_isMoving() {
	auto getPSU_Off_V = [this]() {		
		enableRelays(true);
		auto newOffV = measurePSUVoltage(100);
		if (newOffV) {
			_motorsOffV = newOffV;
			_motors_off_diff_V = uint16_t(_motorsOffV * 0.03);
			//logger() << F("New OFF-V: ") << _motorsOffV << L_endl;
		}
	};

	auto isMoving = true;
	_cool_relay->set(_motorDirection == e_Cooling); // turn Cool relay on/off
	_heat_relay->set(_motorDirection == e_Heating); // turn Heat relay on/off
	if (_motorDirection == e_Stop) {
		getPSU_Off_V();
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
	I2C_Flags_Ref(*registers().ptr(R_DEVICE_STATE)).clear(F_STORE_TOO_COOL);
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
	if (abs(_flowTempAtStartOfWait - _currReqTemp) < 2) _onTime *= 2;
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

	auto needIncreaseBy_deg = int(_currReqTemp) - int(sensorTemp);
	if (needIncreaseBy_deg == 0) {
		if (_journey != e_TempOK) {
			_journey = e_TempOK;
			reg.set(R_ADJUST_MODE, A_GOOD_RATIO);
		}
		logger() << name() << L_tabs << F("OK:") << sensorTemp << L_endl;
		return true;
	}
	else {
		if (_journey == e_TempOK) { // Previously OK temperature has drifted
			_flowTempAtStartOfWait = sensorTemp;
			adjustValve(needIncreaseBy_deg); // action becomes e_Moving
		}
		else { // Undershot after a wait - last adjustment was too small
			auto oldRatio = reg.get(R_RATIO);
			reg.set(R_ADJUST_MODE, A_UNDERSHOT);
			if ((_flowTempAtStartOfWait - sensorTemp) * _journey < 0) { // got nearer during last move
				reg.set(R_RATIO, reg.get(R_RATIO) / 2);
			}
			else {
				_flowTempAtStartOfWait = sensorTemp;
			}
			adjustValve(needIncreaseBy_deg);
			reg.add(R_RATIO, oldRatio);
		}
		return false;
	}
}

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
				if (offCount > 1) delay(100);
			}
			else offCount = 0;
		} while (offCount > 1);
		return offCount == 1;
	};
	
	////////// Non-Measure PSU Version ////////////////
	//if (_journey == e_Moving_Coolest) {
	//	if (_onTime == 0) {
	//		_valvePos = 0;
	//		logger() << name() << L_tabs << F("ClearFindOff") << L_endl;
	//		return true;
	//	}
	//	return false;
	//}
	//else if (_motorDirection == e_Cooling) {
	//	if (_valvePos > registers().get(R_HALF_TRAVERSE_TIME) * 2) _valvePos = registers().get(R_HALF_TRAVERSE_TIME) * 2;
	//	return _valvePos == 0;
	//}
	//else return _valvePos > (registers().get(R_HALF_TRAVERSE_TIME) * 2) + 20;

	if (isOff()) {
		if (_motorDirection == e_Cooling) {
			_valvePos = 0;
		} else {
			if (_valvePos > 100) {
				registers().set(R_HALF_TRAVERSE_TIME, uint8_t(_valvePos / 2));
				_ep->update(_regOffset + R_HALF_TRAVERSE_TIME, uint8_t(_valvePos / 2));
			} else return false;
		}
		return true;
	}
	return false;
}

void Mix_Valve::refreshRegisters() {
	// All I2C transfers are initiated by Programmer: Reading status and temps, sending new requests.
	//lambda
	auto motorActivity = [this]() -> uint8_t {if (_motorDirection == e_Cooling && _journey == e_Moving_Coolest) return e_Moving_Coolest; else return _motorDirection; };
	auto reg = registers();
	reg.set(R_COUNT, abs(_onTime));
	reg.set(R_MOTOR_ACTIVITY, motorActivity()); // Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
	reg.set(R_VALVE_POS, (uint8_t)_valvePos);
}

bool Mix_Valve::doneI2C_Coms(I_I2Cdevice& programmer, bool newSecond) { // called every loop()
	auto reg = registers();
	auto device_State = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
	uint8_t ts_status = device_State.is(F_DS_TS_FAILED);
	if (device_State.is(F_NO_PROGRAMMER) && newSecond) device_State.set(F_I2C_NOW);

	if (device_State.is(F_I2C_NOW)) {
		_temp_sensr.setHighRes();
		ts_status = _temp_sensr.readTemperature();
		if (ts_status == _disabledDevice) {
			_temp_sensr.reset();
		}

		if (ts_status) logger() << F("TSAddr:0x") << L_hex << _temp_sensr.getAddress()
			<< F(" Err:") << I2C_Talk::getStatusMsg(_temp_sensr.lastError())
			<< F(" Status:") << I2C_Talk::getStatusMsg(ts_status) << L_endl;
		auto temp = _temp_sensr.get_fractional_temp();
		reg.set(R_FLOW_TEMP, temp >> 8);
		_flowTempFract = temp & 0x00FF;
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
	//auto i2c_status = I2C_Flags_Ref(*reg.ptr(R_DEVICE_STATE));
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

void Mix_Valve::moveValveTo(int pos) {
	_onTime = abs(pos - _valvePos);
	_motorDirection = pos > _valvePos ? e_Heating : (pos < _valvePos ? e_Cooling : e_Stop);
}

#ifdef SIM_MIXV // must be called only once per second
void Mix_Valve::simulateFlowTemp() {
	_actualFlowTemp_16ths = 16 * (25 + (_maxTemp - 25) * _valvePos  / 140);
	_delayLine.addValue(_actualFlowTemp_16ths);
	float delayTemp = _delayLine.getAverage() / 16.;
	auto reg = registers();
	_reportedTemp += (delayTemp - _reportedTemp) * (1.f - exp(-(1.f / _timeConst)));
	uint16_t reported_16t = (_reportedTemp + (1 / 32.)) * 256;
	reg.set(R_FLOW_TEMP, uint8_t(reported_16t >> 8));
	_flowTempFract = uint8_t(reported_16t & 0xF0);
}
#endif

void Mix_Valve::logPID() {
	//enum Tune { init, findOff, waitForCool, riseToSetpoint, findMax, fallToSetPoint, findMin, lastRise, calcPID, turnOff, restart };

	switch (_pidState) {
	case init:
		logger() << F("Ini"); break;
	case restart:
		logger() << F("S"); break;
	case findOff:
		logger() << F("O"); break;
	case waitForCool:
		logger() << F("C"); break;
	case riseToSetpoint:
		logger() << F("^"); break;
	case findMax:
		logger() << F("^^"); break;
	case fallToSetPoint:
		logger() << F("v"); break;
	case findMin:
		logger() << F("vv"); break;
	case lastRise:
		logger() << F(".^"); break;
	case calcPID:
		logger() << F("?"); break;
	case turnOff:
		logger() << F("Z"); break;
	}
	float flowTemp = registers().get(R_FLOW_TEMP) + _flowTempFract / 256.f;
	logger() << L_tabs << _valvePos << flowTemp << _min_temp_16ths << _max_temp_16ths << _half_period << _period << registers().get(R_PSU_V) <<L_endl;
}

uint8_t Mix_Valve::getPIDconstants() {// Called once every second. maintains mix valve temp.
	runPIDstate();
	logPID();
	return _pidState;
}

void Mix_Valve::runPIDstate() {
	constexpr uint32_t MAX_CHANGE_PERIOD_S = 5;
	static int16_t lastSensTemp;
	static float fastestTempChange;
	auto reg = registers();

	// Lambda
	auto setFractReg = [&reg](uint8_t value) {reg.set(R_PSU_V, value); };

	auto valveHasReachedWarmPos = [this]() {
		return (_motorDirection == e_Heating && _onTime == 0) || _motorDirection != e_Heating;
	};	
	
	auto valveHasReachedCoolPos = [this]() {
		return (_motorDirection == e_Cooling && _onTime == 0) || _motorDirection != e_Cooling;
	};

	auto changeRate = [=](int16_t sensorTemp_16ths) {
		logger() << F("Change") << L_tabs << sensorTemp_16ths/16. << (sensorTemp_16ths - lastSensTemp) ;
		_integrator.addValue(sensorTemp_16ths - lastSensTemp);
		lastSensTemp = sensorTemp_16ths;
		auto aveChangeRate = _integrator.getAverage();
		if (aveChangeRate > fastestTempChange) 
			fastestTempChange = aveChangeRate;
		logger() << F("Fastest:") << fastestTempChange << F("Ave:") << aveChangeRate << L_endl;;
		return aveChangeRate;
	};

	auto isRising = [=](int16_t sensorTemp_16ths) {
		auto change_rate = changeRate(sensorTemp_16ths);
		setFractReg(uint8_t(change_rate));
		if (fastestTempChange < 2) {
			if (valveHasReachedWarmPos()) {
				_currReqTemp = uint8_t((sensorTemp_16ths + 8)/16);
			}
			else {
				change_rate = fastestTempChange; // temp static, still moving to pos.
			}
		}
		return change_rate;
	};

	auto isRisingFast = [isRising](int16_t sensorTemp_16ths) {
		auto debug = fastestTempChange;
		auto debug2 = isRising(sensorTemp_16ths);
		return (debug2 >= fastestTempChange - 0.15);
	};

	auto isCooling = [=](int16_t sensorTemp_16ths) {
		auto change_rate = changeRate(sensorTemp_16ths);
		setFractReg(uint8_t(change_rate));
		if (sensorTemp_16ths < lastSensTemp) {
		}
		else if (valveHasReachedCoolPos() && change_rate >= 0) {
			return false;
		}
		return true;
	};

	// Algorithm
	int16_t sensorTemp_16ths = reg.get(R_FLOW_TEMP) * 16 + _flowTempFract / 16;
	int16_t needIncreaseBy_16ths = (_currReqTemp*16) - sensorTemp_16ths;

	if (activateMotor_isMoving()) {
		if (!continueMove()) stopMotor(); // turns PSU off if stopped.
	}
	reg.set(R_ADJUST_MODE, PID_CHECK);
	switch (_pidState) {
	case init:
		_integrator.setNoOfValues(10);
		_integrator.clear();
		[[fallthrough]];
	case restart:
		logger() << F("Mode") << L_tabs << F("Pos") << F("flowT") << F("min") << F("max") << F("halfP") << F("P") << F("Change") << L_endl;
		_min_temp_16ths = 0; _max_temp_16ths = 0; _half_period = 0; _period = 0; lastSensTemp = sensorTemp_16ths;
		moveValveTo(0);
		_pidState = findOff;
		break;
	case findOff:
		changeRate(sensorTemp_16ths);
		if (valveIsAtLimit()) {
			stopMotor();
			_pidState = waitForCool;
		}
		break;
	case waitForCool:
		changeRate(sensorTemp_16ths);
		if (sensorTemp_16ths < 30 * 16) {
			moveValveTo(int(reg.get(R_HALF_TRAVERSE_TIME) * 1.5)); // 112
			fastestTempChange = .2f;
			_pidState = riseToSetpoint;	
		}
		break;
	case riseToSetpoint:
		if (!isRisingFast(sensorTemp_16ths)) {
			moveValveTo(int(reg.get(R_HALF_TRAVERSE_TIME) * .5)); // 37
			_currReqTemp = uint8_t((sensorTemp_16ths + 0.5f)/16);
			if (_currReqTemp < 40) _pidState = calcPID;
			else _pidState = findMax;
			logger() << F("Mode") << L_tabs << F("Pos") << F("flowT") << F("min") << F("max") << F("halfP") << F("P") << F("Change") << L_endl;
		}
		break;
	case findMax:
		++_period;
		if (isRising(sensorTemp_16ths) > 0.f) {
			_max_temp_16ths = -needIncreaseBy_16ths;
			reg.set(R_RATIO, uint8_t(_max_temp_16ths/16));
			reg.set(R_PSU_V, uint8_t(_max_temp_16ths % 16));
		}
		else {
			_pidState = fallToSetPoint;
		}
		break;
	case fallToSetPoint:
		++_period;
		if (!isCooling(sensorTemp_16ths)) {
			moveValveTo(0);
			_pidState = turnOff; // abort
		} else if (needIncreaseBy_16ths <= 0) {
			_half_period = _period;
		} else {
			moveValveTo(int(reg.get(R_HALF_TRAVERSE_TIME) * 1.5));
			_pidState = findMin;
		}
		break;
	case findMin:
		++_period;
		if (isCooling(sensorTemp_16ths)) {
			_min_temp_16ths = -needIncreaseBy_16ths;
			reg.set(R_RATIO, uint8_t(-_min_temp_16ths / 16));
			reg.set(R_PSU_V, uint8_t(-_min_temp_16ths % 16));
		}
		else {
			_pidState = lastRise;
		}
		break;
	case lastRise:
		++_period;
		if (needIncreaseBy_16ths <= 0) _pidState = calcPID;
		else if (isRising(sensorTemp_16ths) <= 0.f) {
			moveValveTo(0);
			_pidState = turnOff; // abort
		}
		break;
	case calcPID:
		logger() << F("Mode") << L_tabs << F("Pos") << F("flowT") << F("min") << F("max") << F("halfP") << F("P") << F("Change") << L_endl;
		{
			moveValveTo(0);
			_pidState = turnOff;
		}
		break;
	case turnOff:
		if (_valvePos < 5) {
			stopMotor();
			_pidState = init;
			_period = 0;
			reg.set(R_ADJUST_MODE, A_GOOD_RATIO);
		}
		break;
	}
	reg.set(R_MODE, _pidState);
	reg.set(R_VALVE_POS, (uint8_t)_valvePos);
	reg.set(R_COUNT, (uint8_t)_period);
}