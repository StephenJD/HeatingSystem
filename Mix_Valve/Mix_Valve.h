#pragma once
// This is the Arduino Mini Controller
#include <Arduino.h>

class EEPROMClass;
class I2C_Temp_Sensor;
class iRelay;

void sendMsg(const char* msg);
void sendMsg(const char* msg, long a);
enum Role { e_Slave, e_Master };
extern enum Role role;

class Mix_Valve
{
public:
	enum Error { e_OK, e_DS_Temp_failed, e_US_Temp_failed, e_Both_TS_Failed, e_I2C_failed, e_Water_too_cool, e_setup1, e_setup2, e_loop1, e_loop2};
	enum Registers {
		/*Read Only Data*/		status, //Has new data, Error code
		/*Read Only Data*/		mode, count, valve_pos, state = valve_pos + 2,
		/*Read/Write Data*/		flow_temp, request_temp, ratio, control,
		/*Read/Write Config*/	temp_i2c_addr, max_ontime, wait_time, max_flow_temp, eeprom_OK1, eeprom_OK2,
		/*End-Stop*/			reg_size
	};	
	enum Mode {e_Moving, e_Wait, e_Checking, e_Mutex, };
	enum State {e_Moving_Coolest = -2, e_NeedsCooling, e_Off, e_NeedsHeating};
	enum ControlSetting {e_nothing, e_stop_and_wait, e_new_temp};

	Mix_Valve(I2C_Temp_Sensor & temp_sensr, iRelay & heat_relay, iRelay & cool_relay, EEPROMClass & ep, int eepromAddr, int defaultMaxTemp);
	int8_t getTemperature() const;
	uint16_t getRegister(int reg) const;
	
	bool check_flow_temp();
	void setControl(uint8_t value);
	void setRegister(int reg, uint8_t value);
	void setRequestTemp(Role role);
private:
	enum { e_MIN_FLOW_TEMP = 30};
	friend void testMixer();
	friend void testSlave();
	friend void printModes();

	Mode getMode() const;
	bool has_overshot(int new_call_flowDiff) const;
	uint8_t saveToEEPROM() const; // returns noOfBytes saved
	int8_t getMotorState() const;

	void adjustValve(int8_t tempDiff);
	void activateMotor(int8_t direction);
	void reverseOneDegree(int new_call_flowDiff, int actualFlowTemp);
	void waitForValveToStop(int secsSinceLast);
	void waitForTempToSettle(int secsSinceLast);
	void correct_flow_temp(int new_call_flowDiff, int actualFlowTemp);
	void checkPumpIsOn();
	uint8_t loadFromEEPROM(); // returns noOfBytes read

	// Injected dependancies
	I2C_Temp_Sensor * temp_sensr;
	iRelay * heat_relay;
	iRelay * cool_relay;
	EEPROMClass * _ep;
	uint8_t _eepromAddr;
	// EEPROM saved data
	uint8_t _max_on_time;
	uint8_t _valve_wait_time;
	uint8_t _onTimeRatio;
	uint8_t _max_flowTemp;
	// Temporary Data
	int8_t _mixCallTemp;
	State _state; // the requirement to heat or cool, not the actual motion of the valve.
	int8_t _call_flowDiff;
	int16_t _onTime;
	uint8_t _sensorTemp;
	uint8_t _lastOKflowTemp;
	int16_t _valvePos;
	unsigned long _lastTick;
	Error _err;
	static Mix_Valve * motor_mutex; // address of Mix_valve is owner of the mutex
};

