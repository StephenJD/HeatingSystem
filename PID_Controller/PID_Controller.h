#pragma once
#include <Arduino.h>
#include <Static_Deque.h>

inline int floatToInt_round(float f) {
	if (f >= 0) return int(f + 0.5f);
	else return int(f - 0.5f);
}

class PID_Controller {
public:
	PID_Controller(uint8_t min, uint8_t max, uint16_t minSetpoint, int smallestError, int acceptableError) : _minOut(min), _maxOut(max), _minSetpoint(minSetpoint), _smallestError(smallestError), _acceptableError(acceptableError) {}
	void changeSetpoint(int val);
	uint8_t adjust(int measuredVal);
	int integralPart() const { return floatToInt_round(_Kp * _i * _Ki); }
	float diffConst() const { return _Kd; }
	int diff() const { return _d; }
	float propConst() const { return _Kp; }
	float overshoot_ratio() const { return _oveshootRatio < 2 ? _oveshootRatio : 1; }
	void set_Kp(float Kp) { _Kp = Kp; }
	void set_Kd(float Kd) { _Kd = Kd; }
	uint8_t currOut() {	return _output;	}
	//void setOutput(int out) { _output = out; }
private:
	enum PID_State { NEW_TEMP, MOVE_TO_P, MOVE_TO_D, RETURN_TO_P, WAIT_TO_SETTLE, WAIT_FOR_DRIFT, GET_BACK_ON_TARGET, MOVE_TO_OFF };
	int16_t d_contribution(int16_t errVal);
	uint8_t effective_Kd(int16_t errVal) { return int(_Kp * errVal * _Kd); }
	void constrainD();
	void adjustKd();
	uint8_t _minOut = 0;
	uint8_t _maxOut = 0;
	uint16_t _minSetpoint = 0;
	int _acceptableError = 0;
	int _smallestError = 0;
	float _Kp = .013f;
	float _Ki = .005f; // oscillates on big delay/TC if Ki larger than .005
	float _Kd = .8f;
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
	static constexpr float OVERSHOOT_FACTOR = 1.125f;
	static constexpr int WAIT_AT_D = 5;
	static constexpr float MIN_KD = .005f;
};

