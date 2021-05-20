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
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>

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
	enum Error { e_OK, e_NewTempReq, e_DS_Temp_failed, e_US_Temp_failed, e_Both_TS_Failed, e_I2C_failed, e_Water_too_cool, e_setup1, e_setup2, e_loop1, e_loop2};
	enum Registers {
		// All registers are treated as two-byte, with second-byte un-used for most. Thus single-byte read-write works.
		/*Read Only Data*/		status,
		/*Read Only Data*/		mode, count, valve_pos, state = valve_pos + 2,
		/*Read/Write Data*/		flow_temp, request_temp, ratio, moveFromTemp, moveFromPos,
		/*Read/Write Config*/	temp_i2c_addr, traverse_time, wait_time, max_flow_temp, eeprom_OK1, eeprom_OK2,
		/*End-Stop*/			reg_size
	};	
	enum Mode {e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat, e_Error };
	enum Journey {e_Moving_Coolest = -2, e_CoolNorth, e_TempOK, e_WarmSouth};
	enum MotorDirection {e_Cooling = -1, e_Stop, e_Heating};

	Mix_Valve(HardwareInterfaces::TempSensor & temp_sensr, HardwareInterfaces::Pin_Wag & heat_relay, HardwareInterfaces::Pin_Wag & cool_relay, EEPROMClass & ep, int eepromAddr, int defaultMaxTemp);
	uint16_t getRegister(int reg) const; // To make single-byte read-write work, all registers are treated as two-byte, with second-byte un-used for most.
	const __FlashStringHelper* name();
	void check_flow_temp();
	void setRegister(int reg, uint8_t value);
	void setRequestTemp();
private:
	enum { e_MIN_FLOW_TEMP = HardwareInterfaces::MIN_FLOW_TEMP, e_MIN_RATIO = 2, e_MAX_RATIO = 30};
	friend void testMixer();
	friend void testSlave();
	friend void printModes();

	Mode algorithmMode(int call_flowDiff) const;
	//bool has_overshot(int new_call_flowDiff);
	uint8_t saveToEEPROM() const; // returns noOfBytes saved

	void adjustValve(int tempDiff);
	bool activateMotor(); // Return true if it owns mutex
	void stopMotor();
	void startWaiting();
	//void reverseOneDegree(int new_call_flowDiff, int actualFlowTemp);
	void serviceMotorRequest();
	//void waitForTempToSettle();
	//void correct_flow_temp(int new_call_flowDiff, int actualFlowTemp);
	void checkPumpIsOn();
	uint8_t loadFromEEPROM(); // returns noOfBytes read

	// Injected dependancies
	HardwareInterfaces::TempSensor * temp_sensr;
	HardwareInterfaces::Pin_Wag * heat_relay;
	HardwareInterfaces::Pin_Wag * cool_relay;
	EEPROMClass * _ep;

	// Temporary Data
	int16_t _onTime = 0;
	int16_t _valvePos = 0;
	int16_t _moveFrom_Pos = 0;

	Error _status = e_OK;
	Journey _journey = e_TempOK; // the requirement to heat or cool, not the actual motion of the valve.
	MotorDirection _motorDirection = e_Stop;

	uint8_t _moveFrom_Temp = 0;
	uint8_t _prevReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _waitFlowTemp = e_MIN_FLOW_TEMP;
	mutable uint8_t _sensorTemp = e_MIN_FLOW_TEMP;
	
	uint8_t _eepromAddr;
	// EEPROM saved data
	uint8_t _traverse_time = 140;
	uint8_t _valve_wait_time;
	uint8_t _onTimeRatio = 30;
	uint8_t _max_flowTemp;
	int8_t _mixCallTemp = e_MIN_FLOW_TEMP;
	static Mix_Valve * motor_mutex; // address of Mix_valve is owner of the mutex
};

