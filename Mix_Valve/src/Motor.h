#pragma once

#include <PinObject.h>
#include <Arduino.h>
class PowerSupply;

class Motor {
public:
	enum Direction { e_Cooling = -1, e_Stop, e_Heating };
	Motor(HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay);
	Direction direction() const { return _motion; }
	uint8_t heatPort() const { return _heat_relay->port(); }
	uint8_t curr_pos() const { return uint8_t(_pos); };

	const __FlashStringHelper* name() const { 
		return _heat_relay->port() == 11 ? F("US ") : F("DS "); 
	}
	uint8_t update_pos(PowerSupply& pwr);
	bool moving(Direction direction, PowerSupply& pwr);
	void start(Direction direction);
	uint16_t stop(PowerSupply& pwr);

	void setPosToZero() { _pos = 0; }
	void setPos(int pos) { 
		_pos = pos; }
private:
	HardwareInterfaces::Pin_Wag* _heat_relay = nullptr;
	HardwareInterfaces::Pin_Wag* _cool_relay = nullptr;
	mutable int16_t _pos = 140;
	Direction _motion = e_Stop;
};