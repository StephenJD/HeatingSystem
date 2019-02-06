#include "Mix_Valve.h"
#include "Temp_Sensor.h"
#include "Relay.h"
#include "TimeLib.h"
#include <EEPROM.h>
#include <I2C_Helper.h>

void enableRelays(bool enable); // this function must be defined elsewhere
extern bool receivingNewData;
extern I2C_Helper * i2c;
extern U1_byte version_a;
extern U1_byte version_b;

const U1_byte I2C_MASTER_ADDR = 0x11;

Mix_Valve * Mix_Valve::motor_mutex = 0;

Mix_Valve::Mix_Valve(I2C_Temp_Sensor & temp_sensr, iRelay & heat_relay, iRelay & cool_relay, EEPROMClass & ep, int eepromAddr, int defaultMaxTemp)
	: temp_sensr(&temp_sensr),
	heat_relay(&heat_relay),
	cool_relay(&cool_relay),
	_ep(&ep),
	_eepromAddr(eepromAddr),
	_mixCallTemp(0),
	_state(e_Off),
	_call_flowDiff(0),
	_onTime(0),
	_sensorTemp(30),
	_lastOKflowTemp(getTemperature()),
	_valvePos(0),
	_lastTick(millis()),
	_err(e_OK)
{
	Serial.println("MixValve initialised");
	if (_ep->read(eeprom_OK1 + _eepromAddr) != version_a || _ep->read(eeprom_OK2 + _eepromAddr) != version_b) {
		_ep->update(eeprom_OK1 + _eepromAddr, version_a);
		_ep->update(eeprom_OK2 + _eepromAddr, version_b);
		_max_on_time = 140;
		_valve_wait_time = 10;
		_onTimeRatio = 10;
		_max_flowTemp = defaultMaxTemp;
		saveToEEPROM();
		Serial.println("Saved defaults");
	} else {
		loadFromEEPROM();
		Serial.println("Loaded from EEPROM");
	}
	_sensorTemp = getTemperature();
	Serial.println("MixValve created");
}

void sendMsg(const char* msg) {
	Serial.println(msg);
	//i2c->write(I2C_MASTER_ADDR,0,strlen(msg)+1,(uint8_t*)msg);
}

void sendMsg(const char* msg, long a) {
	char mssg[30];
	sprintf(mssg, "%s %d\n", msg, a); 
	sendMsg(mssg);	
}

uint8_t Mix_Valve::saveToEEPROM() const { // returns noOfBytes saved
	U1_count i = 0;
	_ep->update(_eepromAddr, temp_sensr->getAddress());
	_ep->update(++i + _eepromAddr, _max_on_time);
	_ep->update(++i + _eepromAddr, _valve_wait_time);
	_ep->update(++i + _eepromAddr, _onTimeRatio);
	_ep->update(++i + _eepromAddr, _max_flowTemp);
	_ep->update(++i + _eepromAddr, _mixCallTemp);
	return i;
}

uint8_t Mix_Valve::loadFromEEPROM() { // returns noOfBytes saved
	U1_count i = 0;
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
	//Serial.print("Loaded Temp Addr: 0x"); Serial.println(temp_sensr->getAddress(),HEX);
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
		Serial.println("Read TS");
		return temp_sensr->get_temp();
	} else {
		return _sensorTemp;
	}
}

Mix_Valve::Mode Mix_Valve::getMode() const {
	if (motor_mutex && motor_mutex != this) return e_Mutex;
	else if (_onTime > 0) return e_Moving;
	else if (_onTime < 0) return e_Wait;
	else return e_Checking;
}

bool Mix_Valve::check_flow_temp() { // maintains mix valve temp. Returns true if has been checked.
	int secsSinceLast = Time::secondsSinceLastCheck(_lastTick);
	if (secsSinceLast) {
		//sendMsg("Adjust");
		Mode gotMode = getMode();
		if (gotMode == e_Mutex) {
			sendMsg("Mutex");
			return false;
		} // return if other valve is operating motor,
		int actualFlowTemp = getTemperature();
		int new_call_flowDiff = _mixCallTemp - actualFlowTemp;

		if (has_overshot(new_call_flowDiff)) {
			reverseOneDegree(new_call_flowDiff, actualFlowTemp);
		}
		else if (gotMode == e_Moving) { // wait for valve to finish moving
			waitForValveToStop(secsSinceLast);
		}
		else if (gotMode == e_Wait) { // wait for temp to change.
			waitForTempToSettle(secsSinceLast);
		}
		else { // e_Checking. Finished moving and waiting, so check if temp is stable
			sendMsg("Check");
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
	sendMsg("adjust");
	State currentDirection = _call_flowDiff > 0 ? e_NeedsHeating : e_NeedsCooling;
	State oldDirection = (_state >= e_Off ? e_NeedsHeating : e_NeedsCooling);
	//Serial.print(_mixCallTemp, DEC); Serial.print(" check for stable temp:"); Serial.println(getTemperature(), DEC);
	// Valve motor will be OFF, but state indicates last scheduled direction of movement (not recent corrections due to overshoot)
	if (_mixCallTemp <= e_MIN_FLOW_TEMP) {
		_onTime = 0;
		if (_valvePos > -_max_on_time) { // Move valve to cool to prevent gravity circulation
			_state = e_Moving_Coolest;
			--_valvePos;
			activateMotor(-1);
			//Serial.print(" Turn Valve OFF:"); Serial.println(_valvePos, DEC);
		}
		else { // turn valve off
			_state = e_Off;
			activateMotor(0);
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
	else if ((actualFlowTemp - _sensorTemp) * oldDirection > 0) {// going in right direction so wait longer
		_onTime = -_valve_wait_time;
	}
	else if (abs(new_call_flowDiff) > 0) { // We are not getting nearer
		_state = new_call_flowDiff > 0 ? e_NeedsHeating : e_NeedsCooling;
		adjustValve(new_call_flowDiff);
	} // else we are stable on target
	_call_flowDiff = new_call_flowDiff;
	_sensorTemp = actualFlowTemp;
}

void Mix_Valve::adjustValve(int8_t tempDiff) {
	int direction;
	// Get required direction
	sendMsg("Move",tempDiff);
	if (tempDiff < 0) direction = -1;  // cool valve 
	else if (tempDiff > 0) direction = 1;  // heat valve
	else direction = 0; // retain position

	long onTime = (_onTimeRatio * abs(tempDiff) + 5) / 10;
	if (onTime > 127) _onTime = 127; else _onTime = (int16_t)onTime;
	if (_onTime == 0) {
		direction = 0;
	} else {
		int16_t newValvePos = _valvePos + _onTime * direction;
		if (abs(newValvePos) > _max_on_time) {
			_onTime = _max_on_time - abs(_valvePos);
			//Serial.print(_mixCallTemp, DEC); Serial.print(" adjustValve > Max:"); Serial.println(newValvePos, DEC);
			if (direction == 1) _err = e_Water_too_cool;
			if (_onTime == 0) direction = 0;
		}
	}
	activateMotor(direction);
	_valvePos += _onTime * direction;

	//Serial.print(_mixCallTemp, DEC); 
	//Serial.print(" adjustValve; TempDiff:"); Serial.print(tempDiff, DEC);
	//Serial.print(" TimeOn:"); Serial.println(_onTime, DEC);
	//Serial.print(" valvePos:"); Serial.println(_valvePos, DEC);
}

void Mix_Valve::activateMotor (int8_t direction) {
	sendMsg("Switch");
	if (direction != 0) motor_mutex = this; else motor_mutex = 0;
	if (!cool_relay || !heat_relay) return;
	cool_relay->set(direction == -1); // turn Cool relay on/off
	heat_relay->set(direction == 1); // turn Heat relay on/off
	enableRelays(direction!=0);
}

bool Mix_Valve::has_overshot(int new_call_flowDiff) const {
	sendMsg("Overshot?");
	bool overshoot = new_call_flowDiff * _state < 0;
	int biggerOvershoot = (_call_flowDiff - new_call_flowDiff) * _state;
	return overshoot && biggerOvershoot > 0;
}

void Mix_Valve::reverseOneDegree(int new_call_flowDiff, int actualFlowTemp) { // retains valve state
	sendMsg("Reverse");
	if (_onTime > 0) { // if motor going too far then take off remaining time from ValvePos
		_valvePos -= _onTime * _state;
	}
	_call_flowDiff = new_call_flowDiff;
	_sensorTemp = actualFlowTemp;
	adjustValve(-_state);   // 1 degree shift is expected. Will set _onTime.
	//Serial.print(_mixCallTemp, DEC); Serial.print(" Overshoot. Time:"); Serial.println(_onTime, DEC);
}

void Mix_Valve::waitForValveToStop(int secsSinceLast) {
	sendMsg("waitStop", _onTime);
	/*Serial.print(_mixCallTemp, DEC); 
	Serial.print(" wait to finish moving. Time:"); Serial.print(_onTime, DEC); 
	Serial.print(" secsSince:"); Serial.print(secsSinceLast, DEC);
	Serial.print(" Temp:"); Serial.println(getTemperature(), DEC);*/
	_onTime -= secsSinceLast;
	if (_onTime <= 0) {  // Turn motor off and wait
		_onTime = -_valve_wait_time;
		activateMotor(0); // does not change the valve status.
	}
}

void Mix_Valve::waitForTempToSettle(int secsSinceLast) {
	sendMsg("waitSettle",_onTime);
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
	else if (cool_relay->get()) return -1;	
	else if (heat_relay->get()) return 1;
	else return 0;
}

uint16_t Mix_Valve::getRegister(int reg) const {
	switch (reg) {
	// read only
	case status: return _err;
	case mode: return getMode();
	case count: return abs(_onTime);
	case valve_pos: return _valvePos;
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
		Serial.print("Receive Req Temp:"); Serial.println(value);
		break;
	default:
		saveToEE = false;
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
			break;
		}
	default: ;
	} 
}