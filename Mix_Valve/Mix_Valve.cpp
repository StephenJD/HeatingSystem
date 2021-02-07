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
		_valve_wait_time = 10;
		_onTimeRatio = 10;
		_max_flowTemp = defaultMaxTemp;
		saveToEEPROM();
		logger() << F("Saved defaults") << L_endl;
	} else {
		loadFromEEPROM();
		logger() << F("Loaded from EEPROM") << L_endl;
	}
	logger() << F("MixValve created") << L_endl;
}

uint8_t Mix_Valve::saveToEEPROM() const { // returns noOfBytes saved
	uint8_t i = 0;
	_ep->update(_eepromAddr, temp_sensr->getAddress());
	_ep->update(++i + _eepromAddr, _max_on_time);
	_ep->update(++i + _eepromAddr, _valve_wait_time);
	_ep->update(++i + _eepromAddr, _onTimeRatio);
	_ep->update(++i + _eepromAddr, _max_flowTemp);
	_ep->update(++i + _eepromAddr, _mixCallTemp);
	return i;
}

uint8_t Mix_Valve::loadFromEEPROM() { // returns noOfBytes saved
	uint8_t i = 0;
#ifdef TEST_MIX_VALVE_CONTROLLER
	_max_on_time = 140;
	_valve_wait_time = 10;
	_onTimeRatio = 10;
	_max_flowTemp = 55;
#else	
	temp_sensr->setAddress(_ep->read(_eepromAddr));
	_max_on_time = _ep->read(++i + _eepromAddr);
	_valve_wait_time = _ep->read(++i + _eepromAddr);
	_onTimeRatio = _ep->read(++i + _eepromAddr);
	_max_flowTemp = _ep->read(++i + _eepromAddr);
	_mixCallTemp = _ep->read(++i + _eepromAddr);

#endif
	return i;
}

void Mix_Valve::setRequestTemp(Role role) {
	if (role == e_Slave) {
		loadFromEEPROM();
	} else {
		_mixCallTemp = _max_flowTemp;
	}
}

int8_t Mix_Valve::getTemperature() const {
	if (role == e_Master) {
		temp_sensr->readTemperature();
		_sensorTemp = temp_sensr->get_temp();
	}
	return _sensorTemp;
}

Mix_Valve::Mode Mix_Valve::getMode() const {
	if (_onTime > 0) {
		if (motor_mutex && motor_mutex != this) return e_Mutex;
		else return e_Moving;
	}
	else if (_onTime < 0) return e_Wait;
	else return e_Checking;
}

bool Mix_Valve::check_flow_temp() { // maintains mix valve temp. Returns true if has been checked.
	int secsSinceLast = secondsSinceLastCheck(_lastTick);
	if (secsSinceLast) {
		serviceMotorRequest(secsSinceLast);
		Mode gotMode = getMode();
		int actualFlowTemp = getTemperature();
		int new_call_flowDiff = _mixCallTemp - actualFlowTemp;

		if (has_overshot(new_call_flowDiff)) {
			reverseOneDegree(new_call_flowDiff, actualFlowTemp);
		}
		else if (gotMode == e_Wait) { // wait for temp to change.
			waitForTempToSettle(secsSinceLast);
		}
		else if (gotMode == e_Checking) { // e_Checking. Finished moving and waiting, so check if temp is stable
			correct_flow_temp(new_call_flowDiff, actualFlowTemp);
			if (_state == e_Off || _valvePos == _max_on_time) {
				//checkPumpIsOn();
			}
		} // end _onTime == 0
		//Serial.print(" valvePos:"); Serial.println(_valvePos, DEC);
		return true;
	} else return false;
}

void Mix_Valve::correct_flow_temp(int new_call_flowDiff, int actualFlowTemp) {
	// called every second if gotMode == e_Checking
	State currentDirection = _call_flowDiff > 0 ? e_NeedsHeating : e_NeedsCooling;
	State oldDirection = (_state >= e_Off ? e_NeedsHeating : e_NeedsCooling);
	// Valve motor will be OFF, but state indicates last scheduled direction of movement (not recent corrections due to overshoot)
	if (_mixCallTemp <= e_MIN_FLOW_TEMP) {
		if (_valvePos > -_max_on_time) {
			if (_state != e_Moving_Coolest) { // Move valve to cool to prevent gravity circulation
				_state = e_Moving_Coolest;
				_motorDirection = e_cooling;
				_onTime = _max_on_time;
				logger() << F(" Turn Valve OFF:") << _valvePos << L_endl;
			}
		}
		else { // turn valve off
			_onTime = 0;
			_state = e_Off;
			_motorDirection = e_stop;
		}
	} else if (new_call_flowDiff == 0 && _state != e_Off) { // temp newly OK 
		if (_state == currentDirection) { // without overshoot
			if (_mixCallTemp != _lastOKflowTemp) { // update _onTimeRatio
				long newRatio = abs(_valvePos * 10L / (_mixCallTemp - _lastOKflowTemp));
				if (newRatio < 5)  _onTimeRatio = 5;
				else if (newRatio > 99) _onTimeRatio = 99;
				else _onTimeRatio = int8_t(newRatio);
			}
		} else { // with overshoot
			if (_onTimeRatio >= 10) _onTimeRatio = _onTimeRatio / 2; // min = 1/2 sec per degree
		}
		_valvePos = 0;
		_state = e_Off;
		_lastOKflowTemp = _mixCallTemp;
	}
	else if ((actualFlowTemp - _prevSensorTemp) * oldDirection > 0) {// going in right direction so wait longer
		_onTime = -_valve_wait_time;
	}
	else if (abs(new_call_flowDiff) > 0) { // New Temp or we are not getting nearer
		_state = new_call_flowDiff > 0 ? e_NeedsHeating : e_NeedsCooling;
		adjustValve(new_call_flowDiff);
	} // else we are stable on target
	_call_flowDiff = new_call_flowDiff;
	_prevSensorTemp = actualFlowTemp;
}

void Mix_Valve::adjustValve(int8_t tempDiff) {
	// Get required direction
	logger() << F("Move") << tempDiff << L_endl;
	if (tempDiff < 0) _motorDirection = e_cooling;  // cool valve 
	else if (tempDiff > 0) _motorDirection = e_heating;  // heat valve
	else _motorDirection = e_stop; // retain position

	long onTime = (_onTimeRatio * abs(tempDiff) + 5) / 10;
	if (onTime > 127) _onTime = 127; else _onTime = (int16_t)onTime;
	if (_onTime == 0) {
		_motorDirection = e_stop;
	}
	//Serial.print(_mixCallTemp, DEC); 
	//Serial.print(" adjustValve; TempDiff:"); Serial.print(tempDiff, DEC);
	//Serial.print(" TimeOn:"); Serial.println(_onTime, DEC);
	//Serial.print(" valvePos:"); Serial.println(_valvePos, DEC);
}

bool Mix_Valve::activateMotor () {
	//logger() << F("Switch\n");
	if (!motor_mutex || motor_mutex == this) { // we have control
		if (_motorDirection == e_stop) motor_mutex = 0;
		else motor_mutex = this;
		cool_relay->set(_motorDirection == e_cooling); // turn Cool relay on/off
		heat_relay->set(_motorDirection == e_heating); // turn Heat relay on/off
		enableRelays(_motorDirection != e_stop);
		return true;
	} else return false;
}

void Mix_Valve::serviceMotorRequest(int secsSinceLast) {
	if (_onTime > 0 && activateMotor()) {
		logger() << F("ValveMoving: ") << _onTime << L_endl;
		/*Serial.print(_mixCallTemp, DEC);
		Serial.print(" wait to finish moving. Time:"); Serial.print(_onTime, DEC);
		Serial.print(" secsSince:"); Serial.print(secsSinceLast, DEC);
		Serial.print(" Temp:"); Serial.println(getTemperature(), DEC);*/
		_onTime -= secsSinceLast;
		_valvePos += _motorDirection * secsSinceLast;
		if (abs(_valvePos) >= _max_on_time) {
			_onTime = 0;
			_motorDirection = e_stop;
			if (_motorDirection == e_heating) {
				_err = e_Water_too_cool;
				_valvePos = _max_on_time;
			} else _valvePos = -_max_on_time;
		}
		if (_onTime <= 0) {  // Turn motor off and wait
			_onTime = -_valve_wait_time;
			_motorDirection = e_stop;
			_err = e_OK;			
			activateMotor(); // does not change the valve status.
		}
	}
}

bool Mix_Valve::has_overshot(int new_call_flowDiff) const {
	bool overshoot = new_call_flowDiff * _state < 0;
	int biggerOvershoot = (_call_flowDiff - new_call_flowDiff) * _state;
	return overshoot && biggerOvershoot > 0;
}

void Mix_Valve::reverseOneDegree(int new_call_flowDiff, int actualFlowTemp) { // Has overshot. Retains _state
	logger() << F("Reverse\n");
	//if (_onTime > 0) { // if motor going too far then take off remaining time from ValvePos
	//	_valvePos -= _onTime * _state;
	//}
	_call_flowDiff = new_call_flowDiff;
	_prevSensorTemp = actualFlowTemp;
	adjustValve(-_state);   // 1 degree shift is expected. Will set _onTime and motor will adjust _valvePos.
	//Serial.print(_mixCallTemp, DEC); Serial.print(" Overshoot. Time:"); Serial.println(_onTime, DEC);
}


void Mix_Valve::waitForTempToSettle(int secsSinceLast) {
	logger() << F("WaitSettle: ") << _onTime << L_endl;
	//Serial.print(_mixCallTemp, DEC); 
	//Serial.print(" wait for temp change. Time:"); Serial.print(_onTime, DEC); 
	//Serial.print(" secsSince:"); Serial.print(secsSinceLast, DEC);
	//Serial.print(" Temp:"); Serial.println(getTemperature(), DEC);
	_onTime += secsSinceLast;
	if (_onTime >= 0) {
		_onTime = 0;
	}
}

void checkPumpIsOn() {

}

int8_t Mix_Valve::getMotorState() const {
	if (_state == e_Moving_Coolest) return -2;
	else if (cool_relay->logicalState()) return -1;	
	else if (heat_relay->logicalState()) return 1;
	else return 0;
}

uint16_t Mix_Valve::getRegister(int reg) const {
	// To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most.
	// Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
	// Thus returning uint8_t as uint16_t puts LSB at the lower address, which is what we want!
	switch (reg) {
	// read only
	case status: return _err;
	case mode: return getMode();
	case count: return abs(_onTime);
	case valve_pos: return _valvePos; // two-byte value
	case state: return getMotorState();
	// read/write
	case flow_temp: return _sensorTemp; // can't call getTemperature() 'cos it does an I2C read
	case request_temp: 		
		return _mixCallTemp;
	case ratio:	return	_onTimeRatio;
	case control: return 0;
	case temp_i2c_addr:	return temp_sensr->getAddress();
	case max_ontime: return _max_on_time;
	case wait_time:	return _valve_wait_time;
	case max_flow_temp:	return _max_flowTemp;
	default: return 0;
	}
}

void Mix_Valve::setRegister(int reg, uint8_t value) {
	bool saveToEE = true;

	switch (reg) {
	// register address. Only handle writable registers - they are all single byte.
	case flow_temp:		_sensorTemp = value; break;
	case ratio:			_onTimeRatio = value; break;
	case control:		setControl(value); break;
	case temp_i2c_addr:	temp_sensr->setAddress(value);
		//Serial.print("Save Temp Addr:"); Serial.println(value); 
		break;
	case max_ontime:	_max_on_time = value; break;
	case wait_time:		_valve_wait_time = value; break;
	case max_flow_temp:	_max_flowTemp = value; break;
	case request_temp:	_mixCallTemp = value; // Save request temp. so it resumes after reset 
		//logger() << F("Receive Req Temp:") << value << L_endl;
		break;
	default:
		saveToEE = false; // Non-writable registers
	}
	if (saveToEE) saveToEEPROM();
}

void Mix_Valve::setControl(uint8_t setting) {
	switch (setting) {
	case e_stop_and_wait:
		{
			_onTime = 1; 
			break;
		}
	case e_new_temp:
		{
			_call_flowDiff = _mixCallTemp - _sensorTemp;
			_onTime = 0;
			_state = e_Off;
			break;
		}
	default: ;
	} 
}