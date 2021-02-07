#pragma once
// This is the Arduino Mini Controller
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
// 7. Burn Bootloader
// 8. Then upload your desired sketch to ordinary MiniPro board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

#include <Arduino.h>

class EEPROMClass;

namespace HardwareInterfaces {
	class TempSensor;
	class Pin_Wag;
}

enum Role { e_Slave, e_Master };
extern enum Role role;

class Mix_Valve
{
public:
	enum Error { e_OK, e_DS_Temp_failed, e_US_Temp_failed, e_Both_TS_Failed, e_I2C_failed, e_Water_too_cool, e_setup1, e_setup2, e_loop1, e_loop2};
	enum Registers {
		// All registers are treated as two-byte, with second-byte un-used for most. Thus single-byte read-write works.
		/*Read Only Data*/		status, //Has new data, Error code
		/*Read Only Data*/		mode, count, valve_pos, state = valve_pos + 2,
		/*Read/Write Data*/		flow_temp, request_temp, ratio, control,
		/*Read/Write Config*/	temp_i2c_addr, max_ontime, wait_time, max_flow_temp, eeprom_OK1, eeprom_OK2,
		/*End-Stop*/			reg_size
	};	
	enum Mode {e_Moving, e_Wait, e_Checking, e_Mutex, };
	enum State {e_Moving_Coolest = -2, e_NeedsCooling, e_Off, e_NeedsHeating};
	enum ControlSetting {e_nothing, e_stop_and_wait, e_new_temp};
	enum MotorDirection {e_cooling = -1, e_stop, e_heating};

	Mix_Valve(HardwareInterfaces::TempSensor & temp_sensr, HardwareInterfaces::Pin_Wag & heat_relay, HardwareInterfaces::Pin_Wag & cool_relay, EEPROMClass & ep, int eepromAddr, int defaultMaxTemp);
	int8_t getTemperature() const;
	uint16_t getRegister(int reg) const; // To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most.
	
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
	bool activateMotor(); // Return true if it owns mutex
	void reverseOneDegree(int new_call_flowDiff, int actualFlowTemp);
	void serviceMotorRequest(int secsSinceLast);
	void waitForTempToSettle(int secsSinceLast);
	void correct_flow_temp(int new_call_flowDiff, int actualFlowTemp);
	void checkPumpIsOn();
	uint8_t loadFromEEPROM(); // returns noOfBytes read

	// Injected dependancies
	HardwareInterfaces::TempSensor * temp_sensr;
	HardwareInterfaces::Pin_Wag * heat_relay;
	HardwareInterfaces::Pin_Wag * cool_relay;
	EEPROMClass * _ep;
	uint8_t _eepromAddr;
	// EEPROM saved data
	uint8_t _max_on_time;
	uint8_t _valve_wait_time;
	uint8_t _onTimeRatio;
	uint8_t _max_flowTemp;
	// Temporary Data
	int8_t _mixCallTemp = e_MIN_FLOW_TEMP;
	State _state = e_Off; // the requirement to heat or cool, not the actual motion of the valve.
	MotorDirection _motorDirection = e_stop;
	int8_t _call_flowDiff = 0;
	int16_t _onTime = 0;
	uint8_t _prevSensorTemp = e_MIN_FLOW_TEMP;
	mutable uint8_t _sensorTemp = e_MIN_FLOW_TEMP;
	uint8_t _lastOKflowTemp = e_MIN_FLOW_TEMP;
	int16_t _valvePos = 0;
	unsigned long _lastTick = 0;
	Error _err = e_OK;
	static Mix_Valve * motor_mutex; // address of Mix_valve is owner of the mutex
};

