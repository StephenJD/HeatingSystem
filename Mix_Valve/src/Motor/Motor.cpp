// This is the Multi-Master Arduino Mini Controller
#ifndef __AVR__
	#define SIM_MIXV
#endif

#include <Motor.h>
#include <PSU.h>

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

uint16_t Motor::stop(PowerSupply& pwr) {
	if (_motion != e_Stop) {
		pos(pwr);
		_cool_relay->clear(); _heat_relay->clear();
		_motion = e_Stop;
		logger() << name() << L_tabs
			<< F("Stopping at") << millis() << L_endl;
		return pwr.relinquishPower();
	}
	return 0;
}