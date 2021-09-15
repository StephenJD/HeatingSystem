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
// 7. Tools->Burn Bootloader
// 8. Then upload your desired sketch to ordinary MiniPro board. Tools->Board->Arduino Pro Mini / 328/3.3v/8MHz

#include <TempSensor.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>
#include <Arduino.h>

class EEPROMClass;

namespace HardwareInterfaces {
	class Pin_Wag;
}
namespace I2C_Recovery {
	class I2C_Recover;
}

class Mix_Valve
{
public:
	enum MV_Status { MV_OK, MV_DS_TS_FAILED, MV_US_TS_FAILED, MV_TS_FAILED, MV_I2C_FAILED, MV_NEW_TEMP_REQUEST, MV_STORE_TOO_COOL};
	
	enum MixValve_Volatile_Register_Names { 
		// Registers provided by MixValve_Slave
		// Copies of the VOLATILE set provided in Programmer reg-set
		// All registers are single-byte.
		// All Mix-Valve I2C transfers are initiated by Master

		// send
		R_MV_REG_OFFSET // offset in destination reg-set, used my Master
		, R_STATUS, R_MODE, R_STATE, R_RATIO
		, R_FROM_TEMP
		, R_COUNT
		, R_VALVE_POS
		, R_FROM_POS
		, R_FLOW_TEMP //Send/Receive
		// receive
		, R_REQUEST_FLOW_TEMP
		, R_MV_VOLATILE_REG_SIZE
	};

	enum MixValve_EEPROM_Register_Names { // Programmer does not have these registers
		  R_TS_ADDRESS = R_MV_VOLATILE_REG_SIZE
		, R_FULL_TRAVERSE_TIME
		, R_SETTLE_TIME
		, R_MAX_FLOW_TEMP
		, R_MV_ALL_REG_SIZE
	};

	enum Mode {e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat, e_Error };
	enum Journey {e_Moving_Coolest = -2, e_CoolNorth, e_TempOK, e_WarmSouth};
	enum MotorDirection {e_Cooling = -1, e_Stop, e_Heating};

	Mix_Valve(I2C_Recovery::I2C_Recover& i2C_recover, uint8_t defaultTSaddr, HardwareInterfaces::Pin_Wag & _heat_relay, HardwareInterfaces::Pin_Wag & _cool_relay, EEPROMClass & ep, int reg_offset, int defaultMaxTemp);
	uint8_t getReg(int reg) const;
	const __FlashStringHelper* name();
	MV_Status check_flow_temp();
	void setDefaultRequestTemp();
private:
	enum { e_MIN_FLOW_TEMP = HardwareInterfaces::MIN_FLOW_TEMP, e_MIN_RATIO = 2, e_MAX_RATIO = 30};
	void setReg(int reg, uint8_t value);
	//friend void testMixer();
	//friend void testSlave();
	//friend void printModes();

	Mode algorithmMode(int call_flowDiff) const;
	void saveToEEPROM();
	void checkForNewReqTemp();
	void refreshRegisters();

	void adjustValve(int tempDiff);
	bool activateMotor(); // Return true if it owns mutex
	void stopMotor();
	void startWaiting();
	void serviceMotorRequest();
	void checkPumpIsOn();
	void loadFromEEPROM();

	// Object state
	HardwareInterfaces::TempSensor _temp_sensr;
	uint8_t _regOffset;

	// Injected dependancies
	HardwareInterfaces::Pin_Wag * _heat_relay;
	HardwareInterfaces::Pin_Wag * _cool_relay;
	EEPROMClass * _ep;

	// Temporary Data
	int16_t _onTime = 0;
	int16_t _valvePos = 0;
	int16_t _moveFromPos = 0;
	Journey _journey = e_TempOK; // the requirement to heat or cool, not the actual motion of the valve.
	MotorDirection _motorDirection = e_Stop;

	uint8_t _currReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _newReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _flowTempAtStartOfWait = e_MIN_FLOW_TEMP;
	
	static Mix_Valve * motor_mutex; // address of Mix_valve is owner of the mutex
};

