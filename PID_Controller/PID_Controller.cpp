#include "PID_Controller.h"

void PID_Controller::changeSetpoint(int val) {
    //Apply new setpoint
    auto debugTemp = val / 256;
    auto tempError = int16_t(getAverage(_err_past));
    //if (abs(tempError) > 512) tempError = 0;

    int16_t currTemp = _setPoint - tempError;
    int16_t errVal = val - currTemp;
    if (_last_output == 255) {
        _setPoint = val;
        currTemp = val;
        errVal = val - currTemp;
        _d_has_overshot = true;
    } else {
        // Calculate Kp to match previous result
        auto lastMove = float(_output - _last_output);
        auto lastTempChange = float(currTemp - _lastSetPoint);
        float debugCurrT = currTemp / 256.f;
        float debuglastTempChange = lastTempChange / 256.f;
        if (abs(lastMove) > 1 && abs(lastTempChange) >= _acceptableError && abs(tempError) > 1.1f / _Kp) {
            _Kp = 0.010938; // (_Kp + (lastMove / lastTempChange)) / 2; // should be 0.010938
        }
    }

    // Apply new P & D
    //auto p = _output - (_Kp * tempError);
    //p += _Kp * errVal;
    auto p = _output + (_Kp * errVal);
    _p = uint8_t(p + .5);
    _statePeriod = uint8_t(abs(p - _output) + .5);
    //Apply d
    _d = d_contribution(errVal);
    _i = 0;
    _err_past.prime(errVal);

    // adjust Kd
    if (!_d_has_overshot) {
        adjustKd();
    }
    _lastSetPoint = currTemp;
    _last_output = _output;
    _setPoint = val;
    _d_has_overshot = false;
    _state = MOVE_TO_P;
}

uint8_t PID_Controller::adjust(int measuredVal) {
    /*
    Output = p + i + d
    p is required output in steady-state with no error.This is set by changeSetpoint().
    Kp * err is added to p when request changes, or for drift error after steady-state reached.
    i accumulates error. When steady-state reached, i is used to correct Kp so i can become zero.
    Kd* err is added to d when steady-state correction or new request is made to add overshoot. d decays.
    Kd is adjusted to give _smallestError overshoot.
    */
    auto errVal = _setPoint - measuredVal;
    _err_past.push(errVal);
    auto i = _Kp * _i * _Ki;
    auto newOut = int(_p + i + 0.5);
    auto debugSet = _setPoint / 256;
    auto debugLastSet = _lastSetPoint / 256;

    if (!_d_has_overshot) {
        if (_err_past.hasChangedDirection()) {
            _d_has_overshot = true;
            adjustKd();
        }
    }
    switch (_state) {
    case NEW_TEMP:
        _state = MOVE_TO_P;
        break;
    case MOVE_TO_P:
        --_statePeriod;
        if (_statePeriod <= 0) {
            _statePeriod = 5;
            _state = MOVE_TO_D;
        }
        break;
    case MOVE_TO_D:
        --_statePeriod;
        newOut += _d;
        if (_statePeriod <= 0) {
            _statePeriod = abs(_p - _output);
            _state = RETURN_TO_P;
        }
        break;
    case RETURN_TO_P:
        --_statePeriod;
        if (_statePeriod <= 0) {
            _statePeriod = 20;
            _state = WAIT_TO_SETTLE;
        }
        break;
    case WAIT_TO_SETTLE:
        --_statePeriod;
        if (_d_has_overshot || _statePeriod <= 0) {
            _state = GET_BACK_ON_TARGET;
        }        
        break;
    case GET_BACK_ON_TARGET:
        _i += errVal;
        if (abs(errVal) <= _smallestError) {
            _state = WAIT_FOR_DRIFT;
        }        
        break;
    case WAIT_FOR_DRIFT:
        _i += errVal;
        if (abs(errVal) > _acceptableError) {
            _state = GET_BACK_ON_TARGET;
        }
        break;
    }
    if (newOut > _maxOut) newOut = _maxOut;
    else if (newOut < _minOut) newOut = _minOut;
    _output = newOut;
    return newOut;
}

void PID_Controller::constrainD() {
    if (_Kd > 100.f) 
        _Kd = 100.f;
    else if (_Kd < .01f) 
        _Kd = .01f;
}

void PID_Controller::adjustKd() {
    // Adjust Kd for _smallestError overshoot.
    auto stepChange = _setPoint - _lastSetPoint;
    auto actualOvershoot = stepChange;
    auto debugSet = _setPoint / 256;
    auto debugLastSet = _lastSetPoint / 256;
    if (_d_has_overshot) {
        auto optimalOvershoot = stepChange * OVERSHOOT_FACTOR;
        auto overshoot = 0.f;
        auto attempted_d = _p + _d;
        auto actual_max_d = float(_maxOut - _p);
        if (_setPoint > _lastSetPoint) { // temp increase
            overshoot = -_err_past.minVal();
        }
        else { // temp decrease
            overshoot = -_err_past.maxVal();
            actual_max_d = float(_p - _minOut);
        }
        actualOvershoot = overshoot + stepChange;
        if (actual_max_d < _d) {
            _Kd *= actual_max_d / _d;
        } else {
            _Kd *= (optimalOvershoot / actualOvershoot);
        }
    } else {
        _Kd *= 1.2f;
    }
    _Kd = .7f;
    if (stepChange != 0) _oveshootRatio = float(actualOvershoot) / stepChange;
    constrainD();
}
