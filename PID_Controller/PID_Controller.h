#pragma once
#include <Arduino.h>
#include <Static_Deque.h>

class PID_Controller {
public:
	PID_Controller(uint8_t min, uint8_t max, int smallestError, int acceptableError) : _minOut(min), _maxOut(max), _smallestError(smallestError), _acceptableError(acceptableError) {}
	void changeSetpoint(int val);
	uint8_t adjust(int measuredVal);
	int integralPart() const { return int(_Kp * _i * _Ki); }
	float diffConst() const { return _Kd; }
	float propConst() const { return _Kp; }
	float overshoot_ratio() const { return _oveshootRatio; }
	void set_Kp(float Kp) { _Kp = Kp; }
	uint8_t currOut() {	return _output;	}
	//void setOutput(int out) { _output = out; }
private:
	enum PID_State { NEW_TEMP, MOVE_TO_P, MOVE_TO_D, RETURN_TO_P, WAIT_TO_SETTLE, WAIT_FOR_DRIFT, GET_BACK_ON_TARGET };
	int16_t d_contribution(int16_t errVal) { return int(_Kp * errVal * OVERSHOOT_FACTOR * _Kd); }
	uint8_t effective_Kd(int16_t errVal) { return int(_Kp * errVal * _Kd); }
	void constrainD();
	void adjustKd();
	uint8_t _minOut = 0;
	uint8_t _maxOut = 0;
	int _acceptableError = 0;
	int _smallestError = 0;
	float _Kp = .013f;
	float _Ki = .04f;
	float _Kd = 1.f;
	float _oveshootRatio;
	Fat_Deque<5, int16_t> _err_past;
	int _setPoint = 0;
	int _lastSetPoint = 0;
	uint8_t _output = 0;
	uint8_t _last_output = -1;
	uint8_t _p = 0;
	int _i = 0;
	int _d = 0;
	uint16_t _statePeriod = 0;
	bool _d_has_overshot = false;
	PID_State _state = NEW_TEMP;
	static constexpr float OVERSHOOT_FACTOR = 1.2f;
};

