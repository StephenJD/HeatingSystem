// This is the Multi-Master Arduino Mini Controller
#ifndef __AVR__
	#define SIM_MIXV
	#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#endif
//#undef SIM_MIXV
#include <PSU.h>
#include <Motor.h>

void PowerSupply::begin() {
	_psu_enable.begin(true);
	_key_requester = nullptr;
	_key_owner = nullptr;

	auto psuOffV = psu_v();
	//logger() << F("PSU.Begin() OffV: ") << psuOffV << L_endl;
}

void PowerSupply::setOn(bool on) {
	if (on) _psu_enable.set();
	else if (!_keep_psu_on) _psu_enable.clear();
}

bool PowerSupply::available(const void* requester) { // may be called repeatedly.
	if (_key_requester == nullptr && requester != _key_owner) {
		_key_requester = requester;
	}
	if (_key_owner == nullptr) {
		grantKey();
	}
	return (requester == _key_owner);
}

uint16_t PowerSupply::relinquishPower() {
	// Motor relays will be OFF.
	if (_key_owner) {
		logger() << static_cast<const Motor*>(_key_owner)->name() << L_tabs
			<< F("PSU_0") << static_cast<const Motor*>(_key_owner)->curr_pos() << millis() << L_endl;
	}
	no_load_v(true); // Record PSU-V with status-led ON to mimic relay LED on.
	_key_owner = nullptr;
	if (_key_requester != nullptr && !_keep_psu_on) _psu_enable.clear();
	return _motorsOffV;
}

int PowerSupply::powerPeriod() {
#ifdef SIM_MIXV
	auto power_period = int(_onTime_uS);
	if (_doStep) _onTime_uS = 0;
	else power_period = secondsSinceLastCheck(_onTime_uS);
#else
	auto power_period = secondsSinceLastCheck(_onTime_uS);
#endif
	if (power_period) {
		if (_key_time >= power_period) {
			_key_time -= power_period;
		}
		else if (_key_time > 0) { // key not being stolen
			_key_time = 0;
		}
		if (_key_requester != nullptr && _key_time == 0) {
			_key_time = -1;
			logger() << static_cast<const Motor*>(_key_owner)->name()
				<< L_tabs << F("->")
				<< static_cast<const Motor*>(_key_requester)->name()
				<< millis() << L_endl;
		}
	}
	return power_period;
}

void PowerSupply::keepPSU_on(bool keepOn) {
	_keep_psu_on = keepOn;
	_psu_enable.set(keepOn);
}
#ifdef SIM_MIXV
void PowerSupply::moveOn1Sec() { if (_doStep) ++_onTime_uS; }
void PowerSupply::doStep(bool step) {
	_doStep = step;
	if (_doStep) _onTime_uS = 0;
	else _onTime_uS = micros();
}

#endif

bool PowerSupply::grantKey() {
	_key_owner = _key_requester;
	_key_requester = nullptr;
	if (_key_owner) {
		_key_time = KEY_TIME;
		_onTime_uS = micros();
#ifdef SIM_MIXV
		if (_doStep) _onTime_uS = 0;
#endif
		_psu_enable.set();
		logger() << static_cast<const Motor*>(_key_owner)->name() << L_tabs
			<< F("PSU_1") << millis() << L_endl;
		return true;
	}
	return false;
}

uint16_t PowerSupply::psu_v() {
	// 4v ripple. 3.3v when PSU off, 17-19v when motors off, 16v motor-on
	// Analogue val = 856-868 PSU on. 920-932 when off.
	// Detect peak voltage during cycle
#ifdef SIM_MIXV
	uint16_t psuV = 980;
	if (_key_owner) {
		psuV = psu_loaded_v();
		auto _motor = static_cast<const Motor*>(_key_owner);
		if (_motor->direction() == Motor::e_Stop || _motor->curr_pos() < 5 && _motor->direction() == Motor::e_Cooling) psuV = 980;
		else if (_motor->curr_pos() >= HardwareInterfaces::VALVE_TRANSIT_TIME && _motor->direction() == Motor::e_Heating) psuV = 980;
		//psuMaxV = 80;
	}
	return psuV;
#endif
	auto psuMaxV = 0;
	auto noOfCycles = NO_OF_EQUAL_PSU_CYCLES;
	auto checkAgain =noOfCycles;
	auto timeout = Timer_mS(200);

	do {
		auto testComplete = Timer_mS(20);
		auto thisMaxV = 0;
		do {
			auto thisVoltage = analogRead(PSU_V_PIN);
			if (thisVoltage < 1024 && thisVoltage > thisMaxV) thisMaxV = thisVoltage;
		} while (!testComplete);
		if (thisMaxV == psuMaxV) --checkAgain;
		else {
			psuMaxV = thisMaxV;
			checkAgain = noOfCycles;
		}
	} while (!timeout && checkAgain > 0);

	//logger() << name() << L_tabs << F("PSU_V:") << L_tabs << psuMaxV << L_endl;
	return psuMaxV > 700 ? psuMaxV : 0;
}

uint16_t PowerSupply::no_load_v(bool with_satus_LED_on) {
	setOn(true);
	auto statusLED_state = digitalRead(LED_BUILTIN);
	digitalWrite(LED_BUILTIN, with_satus_LED_on);
	auto offV = psu_v();
	digitalWrite(LED_BUILTIN, statusLED_state);
	if (offV) {
		_motorsOffV = offV;
		_motors_off_diff_V = uint16_t(offV * ON_OFF_RATIO);
		logger() << F("OffV:\t") << _motorsOffV << L_endl;
	}
	return offV;
}

bool PowerSupply::has_no_load() {
	// Detects no-load when relay is ON. Turns status_led off for consistant reads.
	auto statusLED_state = digitalRead(LED_BUILTIN);
	digitalWrite(LED_BUILTIN, LOW);
	auto psuV = 0;
	int offCount = 5;
	do {
		psuV = psu_v();
		if (psuV > _motorsOffV - _motors_off_diff_V) {
			--offCount;
			if (offCount > 1) delay_mS(100);
		}
		else offCount = 0;
	} while (offCount > 1);
	digitalWrite(LED_BUILTIN, statusLED_state);
	return offCount == 1;
}
