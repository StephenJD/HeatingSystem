#include "PID_Controller.h"

void PID_Controller::changeSetpoint(uint8_t val) {
    //Apply new setpoint
    auto tempError = int16_t(getAverage(_err_past));
    if (abs(tempError) > 2) tempError = 0;

    int16_t currTemp = _setPoint - tempError;
    int16_t errVal = val - currTemp;

    if (_lastSetPoint != _setPoint) {
        _lastSetPoint = _setPoint;
    }
    _setPoint = val;
    _p = uint8_t(_output - (_Kp * tempError));
    _p += uint8_t(_Kp * errVal);
    _p = uint8_t(_p + .5);
    _statePeriod = uint8_t(abs(_p - _output) + .5);
    //Apply d
    auto newKd = _Kd / (abs(errVal) + .5);
    _d = uint8_t(_Kp * errVal * newKd);

    _err_past.prime(errVal);

    // adjust Kd
    if (!_d_has_overshot) {
        _Kd *= 1.05f;
        constrainD();
    }
}

uint8_t PID_Controller::adjust(int measuredVal) {
    /*
    Output = p + i + d
    p is required output in steady-state with no error.This is set by changeSetpoint().
    Kp * err is added to p when request changes, or for drift error after steady-state reached.
    i accumulates error. When steady-state reached, i is used to correct Kp so i can become zero.
    Kd* err is added to d when steady-state correction or new request is made to add overshoot. d decays.
    Kd is adjusted to give 1/16 deg overshoot.
    */
    auto errVal = _setPoint - measuredVal;
    _err_past.push(errVal);
    _i = uint8_t(_Kp * _i * _Ki);
    _d = 0;
    uint8_t newOut = _p + _i;
    if (!_d_has_overshot) {
        if (_err_past.hasChangedDirection()) {
            _d = 25;
            _d_has_overshot = true;
        }
    }
    switch (_state) {
    case MOVE_TO_P:
        --_statePeriod;
        if (_statePeriod <= 0) {
            _statePeriod = uint8_t(abs(_d) + .5);
            _state = MOVE_TO_D;
        }
        break;
    case MOVE_TO_D:
        --_statePeriod;
        newOut += _d;
        _d = uint8_t(abs(_d) + .5) - _statePeriod;
        if (_statePeriod <= 0) {
            _statePeriod = abs(_d);
            _state = RETURN_TO_P;
        }
        break;
    case RETURN_TO_P:
        --_statePeriod;
        _d = _statePeriod;
        if (_statePeriod <= 0) {
            _state = WAIT_FOR_TEMP_SETTLE;
            _statePeriod = abs(_d) < 25 ? abs(_d) * 10 : 250;
        }
        break;
    case WAIT_FOR_TEMP_SETTLE:
        --_statePeriod;
        if (_statePeriod <= 0 || _d_has_overshot) {
            adjustKd();
            _state = GET_BACK_ON_TARGET;
            _d = -25;
        }
        break;
    case GET_BACK_ON_TARGET:
        _i += errVal;
        if (abs(errVal) < 1./8) {
            _state = WAIT_FOR_DRIFT;
        }
        break;
    case WAIT_FOR_DRIFT:
        if (abs(errVal) > 1) {
            _state = GET_BACK_ON_TARGET;
        }
        break;
    }
    if (_d < 0) _d *= -1;
    if (newOut > _maxOut) newOut = _maxOut;
    else if (newOut < _minOut) newOut = _minOut;
    _output = newOut;
    return newOut;
}

void PID_Controller::constrainD() {
    if (_Kd > 100.f) _Kd = 100.f;
    else if (_Kd < .01f) _Kd = .01f;
}

void PID_Controller::adjustKd() {
    // Adjust Kd for 1 / 16 deg overshoot.
    if (!_d_has_overshot) {
        auto overshoot = 0;
        //auto goodRange = 0;
        if (_setPoint > _lastSetPoint) { // temp increase
            overshoot = -_err_past.minVal();
            //goodRange = _err_past.maxVal() + (1./16.);
        }
        else { // temp decrease
            overshoot = _err_past.maxVal();
            //goodRange = -_err_past.minVal() + (1. / 16.);
        }
        if (overshoot > 1./16.) {
            _Kd /= 1.05f;
        }
    }
    else {
        _Kd *= 1.05f;
    }
    constrainD();
}
