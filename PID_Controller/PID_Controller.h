#pragma once
#include <Arduino.h>
#include <Static_Deque.h>

class PID_Controller {
public:
	PID_Controller(uint8_t min, uint8_t max) : _minOut(min), _maxOut(max) {}
	void changeSetpoint(uint8_t val);
	uint8_t adjust(int measuredVal);
private:
	enum PID_State { NEW_TEMP, MOVE_TO_P, MOVE_TO_D, RETURN_TO_P, WAIT_FOR_TEMP_SETTLE, WAIT_FOR_DRIFT, GET_BACK_ON_TARGET };
	void constrainD();
	void adjustKd();
	uint8_t _minOut = 0;
	uint8_t _maxOut = 0;
	float _Kp = 1;
	float _Ki = 1;
	float _Kd = 1;
	Fat_Deque<5, int16_t> _err_past;
	uint8_t _setPoint = 0;
	uint8_t _lastSetPoint = 0;
	uint8_t _p = 0;
	uint8_t _i = 0;
	uint8_t _d = 0;
	uint8_t _output = 0;
	uint8_t _statePeriod = 0;
	bool _d_has_overshot = false;
	PID_State _state = NEW_TEMP;
};

