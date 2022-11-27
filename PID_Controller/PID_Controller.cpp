#include "PID_Controller.h"
#include <Logging.h>

using namespace arduino_logger;

void PID_Controller::changeSetpoint(uint16_t val) {
    //Apply new setpoint
#ifdef ZSIM
    auto debugTemp = val / 256;
#endif
    auto tempError = int16_t(getAverage(_err_past));

    int16_t currTemp = _setPoint - tempError;
    int16_t errVal = val - currTemp;
    if (_last_output == 255) {  // pid is just starting up...
        _setPoint = val;
        currTemp = val;
        errVal = val - currTemp;
        _d_has_overshot = true;
    } else if (abs(tempError) > _acceptableError) { // failed to reach requested temp
        _d_has_overshot = true;
    } else {
        // Calculate Kp to match previous result
        auto lastMove = float(_output - _last_output);
        auto lastTempChange = float(currTemp - _lastSetPoint);
#ifdef ZSIM
        float debugCurrT = currTemp / 256.f;
        float debuglastTempChange = lastTempChange / 256.f;
#endif
        float tempChangeForOnePosChange = 1.f / _Kp;
        if (abs(lastMove) > 1 && abs(lastTempChange) >= _acceptableError && abs(tempError) > tempChangeForOnePosChange) {
            _Kp = (_Kp + (lastMove / lastTempChange)) / 2;
            //_Kp = 0.011719;
        }
    }
    // Apply new P & D
    auto p = _output + (_Kp * errVal);
    if (p < 0) p = 0;
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
    if (val <= _minSetpoint) {
        _state = MOVE_TO_OFF;
        _d_has_overshot = true;
    }
    else if (_state == MOVE_TO_OFF) {
        _p = _Kp * (val - _minSetpoint);
        _state = MOVE_TO_P;
        _d_has_overshot = false;
    } else {
         _state = MOVE_TO_P;
        _d_has_overshot = false;
    }
    logger() << F("PID NewSetPoint") << L_endl;
}

uint8_t PID_Controller::adjust(int16_t measuredVal, uint8_t pos) {
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
    auto newOut = _p + integralPart();
#ifdef SIM_MIXV
    auto debugSet = _setPoint / 256;
    auto debugLastSet = _lastSetPoint / 256;
#endif
    if (!_d_has_overshot) {
        if (_err_past.hasChangedDirection() && _err_past.hasChangedSign()) {
            _d_has_overshot = true;
            adjustKd();
        }
    }
    switch (_state) {
    case NEW_TEMP:
        _state = MOVE_TO_P;
        break;
    case MOVE_TO_OFF:
        newOut = 0;
        break;
    case MOVE_TO_P:
        //--_statePeriod;
        //if (_statePeriod <= 0) {
        if (pos == _output) {
            //_statePeriod = abs(_d) + WAIT_AT_D; // extend time spent at _d
            _statePeriod = WAIT_AT_D; // extend time spent at _d
            _state = MOVE_TO_D;
        }
        break;
    case MOVE_TO_D:
        //;
        newOut += _d;
       // if (_statePeriod <= 0) {
        if (pos == _output && --_statePeriod <= 0) {
            //_statePeriod = abs(_p - _output);
            _state = RETURN_TO_P;
        }
        break;
    case RETURN_TO_P:
        //--_statePeriod;
        //if (_statePeriod <= 0) {
        if (pos == _output) {
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
        if (abs(errVal) > _acceptableError) {
            _state = GET_BACK_ON_TARGET;
        }
        break;
    }
    if (newOut > _maxOut) newOut = _maxOut;
    else if (newOut < _minOut) newOut = _minOut;
    _output = uint8_t(newOut);
    return _output;
}

void PID_Controller::constrainD() {
    if (_Kd > 2) 
        _Kd = 1.5f;
    else if (_Kd < MIN_KD)
        _Kd = MIN_KD;
}

int16_t PID_Controller::d_contribution(int16_t errVal) {
    float step = abs(errVal / 256.f);
    float stepFactor = _Kd * .9f * (.27f + exp(-step * .157f * _Kd));
    //stepFactor = _Kd;
    return floatToInt_round(_Kp * errVal * OVERSHOOT_FACTOR * stepFactor);
}

void PID_Controller::adjustKd() {
    // Adjust Kd for _smallestError overshoot.
    auto stepChange = _setPoint - _lastSetPoint;
    auto actualOvershoot = stepChange;
#ifdef SIM_MIXV
    auto debugSet = _setPoint / 256;
    auto debugLastSet = _lastSetPoint / 256;
#endif
    if (_d_has_overshot) {
        auto optimalOvershoot = stepChange * OVERSHOOT_FACTOR;
        auto overshoot = 0.f;
        auto attempted_d = _p + _d;
        auto actual_max_d = float(_maxOut - _p);
        if (_setPoint > _lastSetPoint) { // temp increase
            overshoot = float( - _err_past.minVal());
        }
        else { // temp decrease
            overshoot = float(-_err_past.maxVal());
            actual_max_d = float(_p - _minOut);
        }
        actualOvershoot = int(overshoot + stepChange);
        if (actual_max_d < _d) {
            _Kd *= actual_max_d / _d;
        }
        else {
            _Kd = (_Kd + _Kd * (optimalOvershoot / actualOvershoot))  / 2;
        }
    } else {
        _Kd *= 1.05f;
    }
    //_Kd = 1.0f;
    constrainD();
#ifdef SIM_MIXV
    if (stepChange != 0) _oveshootRatio = float(actualOvershoot) / stepChange;
#endif
}

void PID_Controller::log() const {
    auto showState = [this]() {
        switch (_state) {
        case NEW_TEMP: return F("NewT");
        case MOVE_TO_P: return F("ToP");
        case MOVE_TO_D: return F("ToD");
        case RETURN_TO_P: return F("FromD");
        case WAIT_TO_SETTLE: return F("W_Settle");
        case WAIT_FOR_DRIFT: return F("W_Drift");
        case GET_BACK_ON_TARGET: return F("GetOnTarget");
        case MOVE_TO_OFF: return F("ToOff");
        default: return F("Err_state");
        };
    };
    logger() << name() << L_tabs << F("Set:") << _setPoint/256.f 
        << F("Err is:") << _err_past.last() / 256.f 
        << F("Out:") << (int)_output
        << F("p,i,d is:") << (int)_p << integralPart() << _d
        << F("Mode:") << showState()
        << F("Kp/deg:") << _Kp * 256.f
        << F("Kd:") << _Kd;
}