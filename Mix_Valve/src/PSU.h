#pragma once

#include <PinObject.h>
#include <Arduino.h>
#ifdef SIM_MIXV
class Motor;
#endif

class PowerSupply {
public:
	enum { e_PSU = 6, e_Slave_Sense = 7 };
	void begin();

	void setOn(bool on);

	bool available(const void* requester);

	bool requstToRelinquish() {	return _key_time < 0; }

	uint16_t relinquishPower();

	int powerPeriod();

	void keepPSU_on(bool keepOn = true);

	uint16_t psu_v();

	bool has_no_load();

#ifdef SIM_MIXV
	void moveOn1Sec();
	void doStep(bool step);
	uint16_t psu_loaded_v() { return _motorsOffV - 2 * _motors_off_diff_V; }
private:
	bool _doStep = false;
#endif
private:
	bool grantKey();
	uint16_t no_load_v(bool with_satus_LED_on);

	static constexpr int8_t KEY_TIME = 10;
	static constexpr int NO_OF_EQUAL_PSU_CYCLES = 4;
	static constexpr int PSU_V_PIN = A3;
	static constexpr float ON_OFF_RATIO = .06f;
	int16_t _motorsOffV = 970;
	int16_t _motors_off_diff_V = int16_t(_motorsOffV * ON_OFF_RATIO);

	const void* _key_owner = nullptr;
	const void* _key_requester = nullptr;
	uint32_t _onTime_uS = micros();
	HardwareInterfaces::Pin_Wag _psu_enable{ e_PSU, LOW, true };
	int8_t _key_time = 0;
	bool _keep_psu_on = true;
};

extern PowerSupply psu;