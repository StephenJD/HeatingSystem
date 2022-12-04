#pragma once
// This is the Multi-Master Arduino Mini Controller

// ******** NOTE select Arduino Mini Pro 328P 8MHz **********
// See A_SJD_MixerValve_Control.ini
// Arduino Mini Pro must be upgraded with boot-loader to enable watchdog timer.
// File->Preferences->Additional Boards Manager URLs: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json
// Tools->Board->Boards Manager->Install MiniCore
// 1. In Arduino: Load File-> Examples->ArduinoISP->ArduinoISP
// 2. Upload this to any board, including another Mini Pro.
// 3. THEN connect SPI pins to the Mini Pro: MOSI(11)->11, MISO(12)->12, SCK(13)->13, 10->Reset on MiniPro, Vcc and 0c pins.
// 4. Tools->Board->MiniCore Atmega 328
// 5. External 8Mhz clock, BOD disabled, LTO enabled, Variant 328P, UART0.
// 6. Tools->Programmer->Arduino as ISP
// 7. Tools->Burn Bootloader
// 8. Then upload your desired sketch to ordinary MiniPro board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

#include <TempSensor.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <PinObject.h>
#include <Arduino.h>
#include <EEPROM_RE.h>
#include <I2C_Registers.h>
#include <Flag_Enum.h>
#include <Static_Deque.h>

constexpr int MAX_VALVE_TIME = 150;

extern i2c_registers::I_Registers& mixV_registers;

namespace I2C_Recovery {
	class I2C_Recover;
}

class PowerSupply;
extern PowerSupply psu;

class Motor {
public:
	enum Direction { e_Cooling = -1, e_Stop, e_Heating };
	Motor(HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay);
	Direction direction() const { return _motion; }
	uint8_t heatPort() const { return _heat_relay->port(); }
	uint8_t curr_pos() const { return uint8_t(_pos); };

	const __FlashStringHelper* name() const { 
		return _heat_relay->port() == 11 ? F("US ") : F("DS "); 
		//return F("US "); 
	}
	uint8_t pos(PowerSupply& pwr);
	bool moving(Direction direction, PowerSupply& pwr);
	void start(Direction direction);
	void stop(PowerSupply& pwr);

	void setPosToZero() { _pos = 0; }
	void setPos(int pos) { 
		_pos = pos; }
private:
	HardwareInterfaces::Pin_Wag* _heat_relay = nullptr;
	HardwareInterfaces::Pin_Wag* _cool_relay = nullptr;
	mutable int16_t _pos = HardwareInterfaces::VALVE_TRANSIT_TIME / 2;
	Direction _motion = e_Stop;
};

class Mix_Valve
{
public:
	enum MV_Device_State { F_I2C_NOW, F_NO_PROGRAMMER, F_DS_TS_FAILED, F_US_TS_FAILED, F_NEW_TEMP_REQUEST, F_STORE_TOO_COOL, _NO_OF_FLAGS };

	enum MixValve_Volatile_Register_Names {
		// Registers provided by MixValve_Slave
		// Copies of the VOLATILE set provided in Programmer reg-set
		// All registers are single-byte.
		// If the valve is set as a multi-master it reads its own temp sensors.
		// If the valve is set as a slave it obtains the temprrature from its registers.
		// All I2C transfers are initiated by Programmer: Reading status & sending new requests.
		// In multi-master mode, Programmer reads temps from the registers, in slave mode it writes temps to the registers.

		// Receive
		R_REMOTE_REG_OFFSET // offset in destination reg-set, used my Master
		// Send on request
		, R_DEVICE_STATE	// MV_Device_State FlagEnum	
		, R_MODE	// Algorithm Mode
		, R_MOTOR_ACTIVITY	// Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
		, R_FLOW_TEMP_FRACT
		, R_VALVE_POS
		, R_PSU_MIN_V
		, R_PSU_MAX_V
		, R_PSU_MIN_OFF_V
		, R_PSU_MAX_OFF_V
		, R_PID_MODE
		, R_FLOW_TEMP // Received in Slave-Mode
		// Receive
		, R_REQUEST_FLOW_TEMP // also sent to confirm
		, MV_VOLATILE_REG_SIZE // 11
		, MV_NO_TO_READ = R_REQUEST_FLOW_TEMP - R_REMOTE_REG_OFFSET
	};

	enum MixValve_EEPROM_Register_Names { // Programmer does not have these registers
		// Receive
		R_TS_ADDRESS = MV_VOLATILE_REG_SIZE
		, R_HALF_TRAVERSE_TIME
		, R_DEFAULT_FLOW_TEMP
		, R_VERSION_MONTH
		, R_VERSION_DAY
		, MV_ALL_REG_SIZE // = 17
	};

	enum Mode {
		e_Moving, e_WaitingToMove, e_ValveOff, e_AtTargetPosition, e_FindingOff, e_HotLimit
		/*These are temporary triggers */, e_findOff, e_reachedLimit, e_Error
	};
	enum { PSUV_DIVISOR = 5 };
	using I2C_Flags_Obj = flag_enum::FE_Obj<MV_Device_State, _NO_OF_FLAGS>;
	using I2C_Flags_Ref = flag_enum::FE_Ref<MV_Device_State, _NO_OF_FLAGS>;

	Mix_Valve(I2C_Recovery::I2C_Recover& i2C_recover, uint8_t defaultTSaddr, HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay, EEPROMClass& ep, int reg_offset);
#ifdef SIM_MIXV
	Mix_Valve(I2C_Recovery::I2C_Recover& i2C_recover, uint8_t defaultTSaddr, HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay, EEPROMClass& ep, int reg_offset
		, uint16_t timeConst, uint8_t delay, uint8_t maxTemp);
#endif
	void begin(int defaultFlowTemp);
	// Queries
	const __FlashStringHelper* name() const;
	i2c_registers::RegAccess registers() const { return { mixV_registers, _regOffset }; }
	Mode mode() const {	return Mode(registers().get(R_MODE)); }
	void log() const;
	// Modifiers
	uint16_t update(int newPos);
	void setDefaultRequestTemp();
	bool doneI2C_Coms(I_I2Cdevice& programmer, bool newSecond);
	uint16_t currReqTemp_16() const;
	void setRegister(uint8_t regNo, uint8_t val) { registers().set(regNo, val); }
#ifdef SIM_MIXV
	bool atTarget() { return _motor.curr_pos() == _endPos; }
	uint8_t pos() { return _motor.pos(psu); }
	uint8_t curr_pos() const { return _motor.curr_pos(); }
	static constexpr uint8_t ROOM_TEMP = 20;
	int16_t finalTempForPosition() { 
		return int16_t((256 * (ROOM_TEMP + (_maxTemp - ROOM_TEMP) * _motor.pos(psu) / float(MAX_VALVE_TIME))) + .5f); }
	int16_t flowTemp() const { return registers().get(R_FLOW_TEMP) * 256 + registers().get(R_FLOW_TEMP_FRACT); }
	float get_Kp() const { return MAX_VALVE_TIME / ((_maxTemp - ROOM_TEMP) * 256.f); }
	uint16_t get_TC() const { return _timeConst; }
	uint8_t get_delay() const { return _delay; }
	uint8_t get_maxTemp() const { return _maxTemp; }
	bool waitToSettle() { return _timeAtOneTemp > 0; }
	// Modifiers
	void set_maxTemp(uint8_t max) { _maxTemp = max; }
	void setPos(int pos) { _motor.setPos(pos); }
	void setDelay(int delay) { _delay = delay; }
	void setTC(int tc) { _timeConst = tc; }
	void setTestTime() { _timeAtOneTemp = (get_TC() + get_delay()) * 20; }
	void addTempToDelayIntegral();
	void setFlowTempReg();
	void setIsTemp(uint8_t temp);
	void registerReqTemp(int temp) {
		registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp);
		update(_motor.curr_pos());
		registers().set(Mix_Valve::R_REQUEST_FLOW_TEMP, temp);
		update(_motor.curr_pos());
	}
#endif
private:
#ifdef SIM_MIXV
public:
#endif
	// Loop Functions
	uint16_t stateMachine(int newPos); // returns flow-temp
	// Transition Functions
	void stopMotor();
	void get_PSU_OffV();
	void turnValveOff();
	// New Temp Request Functions
	bool checkForNewReqTemp();
	// Non-Algorithmic Functions
	void setDirection() { _journey = _endPos > _motor.curr_pos() ? Motor::e_Heating : _endPos < _motor.curr_pos() ? Motor::e_Cooling : Motor::e_Stop;	}
	bool valveIsAtLimit();
	void saveToEEPROM();
	uint16_t getSensorTemp_16();
	void loadFromEEPROM();
	int measurePSUVoltage(int noOfCycles = NO_OF_EQUAL_PSU_CYCLES);
	// Object state
	HardwareInterfaces::TempSensor _temp_sensr;
	uint8_t _regOffset;

	// Injected dependancies
	Motor _motor;
	Motor::Direction _journey = Motor::e_Stop;
	EEPROMClass* _ep;

	// Algorithm Data
	int16_t _endPos = 0;

	// State data
	uint8_t _currReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _newReqTemp = 0; // Must have two the same to reduce spurious requests
	
	static constexpr float ON_OFF_RATIO = .06f;
	static constexpr int NO_OF_EQUAL_PSU_CYCLES = 4;
	int16_t _motorsOffV = 970;
	int16_t _motors_off_diff_V = int16_t(_motorsOffV * ON_OFF_RATIO);

#ifdef SIM_MIXV
	uint16_t _timeConst = 0;
	uint8_t _delay;
	Deque<100,int16_t> _delayLine{};
	float _reportedTemp = 25.f;
	uint8_t _maxTemp;
	int _timeAtOneTemp;
#endif
};

class PowerSupply {
public:
	enum { e_PSU = 6, e_Slave_Sense = 7 };
	void begin() {
		_psu_enable.begin(true);
		_key_requester = nullptr;
		_key_owner = nullptr;
	}

	void setOn(bool on) {
		if (on) _psu_enable.set();
		else if (!_keep_psu_on) _psu_enable.clear();
	}

	bool available(const void* requester) { // may be called repeatedly.
		if (_key_requester == nullptr && requester != _key_owner) {
			_key_requester = requester;
		} 
		if (_key_owner == nullptr) {
			grantKey();
		}
		return (requester == _key_owner);
	}

	bool requstToRelinquish() {	return _key_time < 0; }

	void relinquishPower() {
		if (_key_owner) {
			logger() << static_cast<const Motor*>(_key_owner)->name() << L_tabs
				<< F("relinquishPower at") << static_cast<const Motor*>(_key_owner)->curr_pos() << F("Time:") << millis() << L_endl;
		}
		_key_owner = nullptr;
		if (_key_requester != nullptr && !_keep_psu_on) _psu_enable.clear();
	}

	int powerPeriod() {
		//logger() << static_cast<const Motor*>(_key_owner)->name() << L_tabs << F("Last OnTime:") << _onTime_uS << L_endl;
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
					<< L_tabs << F("giving way to")
					<< static_cast<const Motor*>(_key_requester)->name() 
					<< F("Time:") << millis() << L_endl;
			}
		}
		return power_period;
	}

	void keepPSU_on(bool keepOn = true) {
		_keep_psu_on = keepOn;
		_psu_enable.set(keepOn);
	}
#ifdef SIM_MIXV
	void moveOn1Sec() {if (_doStep) ++_onTime_uS; }
	void doStep(bool step) {
		_doStep = step;
		if (_doStep) _onTime_uS = 0;
		else _onTime_uS = micros();
	}
	void showState() const {
		logger() << "Owner:" << L_tabs << long(_key_owner) << "Requester:" << long(_key_requester) << "Keytime:" << int(_key_time) << L_endl;
	}
private:
	bool _doStep = false;
#endif
private:
	bool grantKey() {
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
				<< F("granted key at Time:") << millis() << L_endl;
			return true;
		}
		return false;
	}

	static constexpr int8_t KEY_TIME = 10;
	const void* _key_owner = nullptr;
	const void* _key_requester = nullptr;
	uint32_t _onTime_uS = micros();
	HardwareInterfaces::Pin_Wag _psu_enable{ e_PSU, LOW, true };
	int8_t _key_time = 0;
	bool _keep_psu_on = true;
};