// This is the Arduino Mini Controller

#include <Mix_Valve.h>
#include <TempSensor.h>
#include "PinObject.h"
#include <EEPROM.h>
#include <I2C_Talk.h>
#include <Logging.h>
#include <Timer_mS_uS.h>

using namespace HardwareInterfaces;

void enableRelays(bool enable); // this function must be defined elsewhere
extern bool receivingNewData;
extern uint8_t version_a;
extern uint8_t version_b;

const uint8_t I2C_MASTER_ADDR = 0x11;

Mix_Valve * Mix_Valve::motor_mutex = 0;

Mix_Valve::Mix_Valve(TempSensor & temp_sensr, Pin_Wag & heat_relay, Pin_Wag & cool_relay, EEPROMClass & ep, int eepromAddr, int defaultMaxTemp)
	: temp_sensr(&temp_sensr),
	heat_relay(&heat_relay),
	cool_relay(&cool_relay),
	_ep(&ep),
	_eepromAddr(eepromAddr)
{
	if (_ep->read(eeprom_OK1 + _eepromAddr) != version_a || _ep->read(eeprom_OK2 + _eepromAddr) != version_b) {
		_ep->update(eeprom_OK1 + _eepromAddr, version_a);
		_ep->update(eeprom_OK2 + _eepromAddr, version_b);
		_max_on_time = 140;
		_valve_wait_time = 40;
		_onTimeRatio = 30;
		_max_flowTemp = defaultMaxTemp;
		saveToEEPROM();
		logger() << F("Saved defaults") << L_endl;
	} else {
		loadFromEEPROM();
		logger() << F("Loaded from EEPROM") << L_endl;
	}
	logger() << F("MixValve created") << L_endl;
}

const __FlashStringHelper* Mix_Valve::name() {
	return (heat_relay->port() == 11 ? F("US ") : F("DS "));
}

uint8_t Mix_Valve::saveToEEPROM() const { // returns noOfBytes saved
	uint8_t i = 0;
	_ep->update(_eepromAddr, temp_sensr->getAddress());
	_ep->update(++i + _eepromAddr, _max_on_time);
	_ep->update(++i + _eepromAddr, _valve_wait_time);
	_ep->update(++i + _eepromAddr, _onTimeRatio);
	_ep->update(++i + _eepromAddr, _max_flowTemp);
	_ep->update(++i + _eepromAddr, _mixCallTemp);
	//logger() << F("MixValve saveToEEPROM") << L_tabs << (int)_max_on_time << (int)_valve_wait_time << (int)_onTimeRatio << (int)_max_flowTemp << (int)_mixCallTemp << L_endl;
	return i;
}

uint8_t Mix_Valve::loadFromEEPROM() { // returns noOfBytes saved
	uint8_t i = 0;
#ifdef TEST_MIX_VALVE_CONTROLLER
	_max_on_time = 140;
	_valve_wait_time = 10;
	_max_flowTemp = 55;
#else	
	temp_sensr->setAddress(_ep->read(_eepromAddr));
	_max_on_time = _ep->read(++i + _eepromAddr);
	_valve_wait_time = _ep->read(++i + _eepromAddr);
	++i; // ratio
	_max_flowTemp = _ep->read(++i + _eepromAddr);
	_mixCallTemp = _ep->read(++i + _eepromAddr);
#endif
	_onTimeRatio = 30;
	return i;
}

void Mix_Valve::setRequestTemp() { // called by MixValve_Slave.ino when master/slave mode changes
	_mixCallTemp = _max_flowTemp;
}

Mix_Valve::Mode Mix_Valve::algorithmMode(int call_flowDiff) const {
	bool dontWantHeat = _mixCallTemp <= e_MIN_FLOW_TEMP;
	bool valveIsAtLimit = abs(_valvePos) == _max_on_time && (dontWantHeat || (call_flowDiff * _valvePos) >= 0);

	if (_status == e_NewTempReq) return e_NewReq;
	else if (valveIsAtLimit) return e_AtLimit;
	else if (dontWantHeat) return e_DontWantHeat;
	else if (_onTime > 0) {
		if (motor_mutex && motor_mutex != this) return e_Mutex;
		else return e_Moving;
	} else if (_onTime < 0) return e_Wait;
	else return e_Checking;
}

void Mix_Valve::check_flow_temp() { // Called once every second. maintains mix valve temp.
	/* If temp > 2 deg off, set motor on until you get to the req temp, then zero Valve-Pos and wait.
	* At 1 deg overshoot, reverse until on target. Ratio = Valve-Pos, gives seconds per degree.
	* If it overshoots the other way, halve the valve-pos for reversing.
	* If after a wait the correction is not enough, add 50% to the ratio. Ratio is only used for 1-2 degree changes.
	*/
	// lambdas
	auto turnValveOff = [this]() { // Move valve to cool to prevent gravity circulation
		if (_sensorTemp <= e_MIN_FLOW_TEMP) {
			stopMotor();
			_valvePos = -_max_on_time;
			_journey = e_TempOK;
		} else {
			_journey = e_Moving_Coolest;
			_motorDirection = e_Cooling;
			_onTime = _max_on_time;
			logger() << name() << L_tabs << F("Turn Valve OFF:") << _valvePos << L_endl;
		}
	};

	int call_flowDiff = _mixCallTemp - _sensorTemp;
	serviceMotorRequest(); // -> wait if finished moving and -> check if finished waiting

	switch (algorithmMode(call_flowDiff)) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
	case e_NewReq:
		logger() << name() << L_tabs << F("New Temp Req:") << _mixCallTemp << L_endl;
		if (_valvePos == -_max_on_time) {
			if (/*_waitFlowTemp > _sensorTemp && */_sensorTemp >= _mixCallTemp ) {
				startWaiting(); // re-start waiting whilst the pump is cooling the temp sensor
			} else if (_sensorTemp < _mixCallTemp || _onTime == 0) { // finished waiting and temp has stopped falling
				_status = e_OK;
				//_onTimeRatio = _max_on_time / 2;
				_valvePos = -_max_on_time / 2;
				adjustValve(float(call_flowDiff));
			}
		} else {
			_status = e_OK;
			if (call_flowDiff != 0) {
				adjustValve(float(call_flowDiff));
			} else {
				_valvePos = 0;
				stopMotor();
			}
		}
		break;
	case e_AtLimit: // false as soon as it overshoots 
		if (_mixCallTemp > e_MIN_FLOW_TEMP && _onTime == 0) startWaiting();
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
			_onTimeRatio /= 2; // reduce ratio on each overshoot
			adjustValve(float(call_flowDiff)); // action becomes e_Moving
			//adjustValve(call_flowDiff / 2.0f); // action becomes e_Moving
		} else if (call_flowDiff * _journey > 0 && (_waitFlowTemp - _sensorTemp) * _journey > 0) { // Got worse during wait
			_waitFlowTemp = _sensorTemp;
			adjustValve(float(call_flowDiff));
		}
		break;
	case e_Checking: 
		if (call_flowDiff == 0) {
			if (_journey != e_TempOK) {
				_journey = e_TempOK;
				auto tempChange = _prevOKTemp - _sensorTemp;
				if (_valvePos != 0 && tempChange != 0) {
					_onTimeRatio = abs(_valvePos / tempChange);
				}
				_prevOKTemp = _sensorTemp;
				_valvePos = 0;
			}
		} else {
			if (_journey == e_TempOK) { // Previously OK temperature has drifted
				_prevOKTemp = _sensorTemp;
				adjustValve(float(call_flowDiff)); // action becomes e_Moving
			}
			else { // Undershot after a wait - last adjustment was too small
				//auto oldRatio = _onTimeRatio;
				adjustValve(float(call_flowDiff));
				//if (_waitFlowTemp == _sensorTemp) { // no change in temp during wait
					//call_flowDiff *= 2;
					_onTimeRatio *= 2;
					//oldRatio = 0;
				//} else {
					//_onTimeRatio /= 2; // move the rest of the distance, as if the ratio had been 50% bigger
					//call_flowDiff = call_flowDiff + (call_flowDiff + 1) / 2;
				//}
				//_onTimeRatio += oldRatio;
			} 
		}
		break;
	}
}

void Mix_Valve::adjustValve(float tempDiff) {
	// Get required direction.
	if (tempDiff < 0) _motorDirection = e_Cooling;  // cool valve 
	else _motorDirection = e_Heating;  // heat valve
	if (_journey != Journey(_motorDirection)) {
		//_valvePos = 0;
		_journey = Journey(_motorDirection);
	}

	if (_onTimeRatio < e_MIN_RATIO) _onTimeRatio = e_MIN_RATIO;
	else if (_onTimeRatio > e_MAX_RATIO) _onTimeRatio = e_MAX_RATIO;
	logger() << name() << L_tabs << F("Move") << tempDiff << F("\tRatio:") << (int)_onTimeRatio << L_endl;
	auto tempError = abs(tempDiff);

	float onTime = _onTimeRatio * tempError;
	if (onTime > _max_on_time / 2) _onTime = _max_on_time / 2; else _onTime = (int16_t)onTime;
}

bool Mix_Valve::activateMotor() {
	if (!motor_mutex || motor_mutex == this) { // we have control
		if (_motorDirection == e_Stop) motor_mutex = 0;
		else motor_mutex = this;
		cool_relay->set(_motorDirection == e_Cooling); // turn Cool relay on/off
		heat_relay->set(_motorDirection == e_Heating); // turn Heat relay on/off
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
	_onTime = -_valve_wait_time;
	_waitFlowTemp = _sensorTemp;
	stopMotor();
}

void Mix_Valve::serviceMotorRequest() {
	if (_onTime > 0) {
		if (activateMotor()) {
			logger() << name() << L_tabs << F("ValveMoving") << (_motorDirection == e_Heating ? "H" : (_motorDirection == e_Stop ? "Off" : "C")) << _onTime << F("ValvePos:") << _valvePos << F("Temp:") << _sensorTemp << F("Call") << _mixCallTemp << L_endl;
			--_onTime;
			_valvePos += _motorDirection;
			if (abs(_valvePos) == _max_on_time) {
				if (_motorDirection == e_Heating) {
					_status = e_Water_too_cool;
				} else _journey = e_TempOK;
				_onTime = 0;
				stopMotor();
			} else {
				if (_onTime == 0) {  // Turn motor off and wait
					startWaiting();
					if (_status == e_Water_too_cool) _status = e_OK;
				}
			}
			if (_onTime % 10 == 0) motor_mutex = 0; // Give up ownership every 10 seconds.
		}
	}
	else if (_onTime < 0)  {
		logger() << name() << L_tabs << F("WaitSettle:\t") << _onTime << F("\t\tTemp:") << _sensorTemp << L_endl;
		++_onTime;
	}
}

void checkPumpIsOn() {
}

uint16_t Mix_Valve::getRegister(int reg) const {
	//lambda
	auto motorActivity = [this]() -> uint16_t {if (_motorDirection == e_Cooling && _journey == e_Moving_Coolest) return e_Moving_Coolest; else return _motorDirection; };

	// To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most.
	// Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
	// Thus returning uint8_t as uint16_t puts LSB at the lower address, which is what we want!
	uint16_t value = 0;
	switch (reg) {
	// read only
	case status: value = _status; break;
	case mode: value = algorithmMode(_mixCallTemp - _sensorTemp); break; // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
	case count: value = abs(_onTime); break;
	case valve_pos: value = _valvePos; break;
	case state: value = motorActivity(); break; // Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
	// read/write
	case flow_temp: value = _sensorTemp;  break;
	case request_temp: value = _mixCallTemp; break;
	case ratio:	value = _onTimeRatio; break;
	case control: return 0;
	case temp_i2c_addr:	value = temp_sensr->getAddress(); break;
	case max_ontime: value = _max_on_time; break;
	case wait_time:	value = _valve_wait_time; break;
	case max_flow_temp:	value = _max_flowTemp; break;
	default: return 0;
	}
	value = (value >> 8) | ((value & 0xFF) << 8);
	return value; // Two-byte and negative values need to send MSB first by swapping bytes
}

void Mix_Valve::setRegister(int reg, uint8_t value) {
	bool saveToEE = true;
	// ********* Must NOT seral.print in here, cos called by ISR.
	switch (reg) {
	// register address. Only handle writable registers - they are all single byte.
	case flow_temp:		_sensorTemp = value; break;
	case ratio:			break;
	case control:		break;
	case temp_i2c_addr:	temp_sensr->setAddress(value);
		break;
	case max_ontime:	_max_on_time = value; break;
	case wait_time:		_valve_wait_time = value; break;
	case max_flow_temp:	_max_flowTemp = value; break;
	case request_temp: 
		if (_prevReqTemp != value) _prevReqTemp = value; // REQUEST TWICE TO REGISTER.
		else if (_mixCallTemp != value) {
			_mixCallTemp = value;
			_onTime = 0;
			if (_mixCallTemp > e_MIN_FLOW_TEMP) {
				_status = e_NewTempReq;
				_journey = e_TempOK;
				if (_mixCallTemp < _sensorTemp) _waitFlowTemp = _sensorTemp + 1; else _waitFlowTemp = _sensorTemp; // Trigger wait if valve was off.	
			}
		}
		break;
	default:
		saveToEE = false; // Non-writable registers
	}
	if (saveToEE) saveToEEPROM();
}