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
#include <Arduino.h>
#include <EEPROM_RE.h>
#include <I2C_Registers.h>
#include <Flag_Enum.h>

extern i2c_registers::I_Registers& mixV_registers;

namespace HardwareInterfaces {
	class Pin_Wag;
}
namespace I2C_Recovery {
	class I2C_Recover;
}

class Mix_Valve
{
public:
	enum MV_Device_State { F_I2C_NOW, F_NO_PROGRAMMER, F_DS_TS_FAILED, F_US_TS_FAILED, F_NEW_TEMP_REQUEST, F_STORE_TOO_COOL, _NO_OF_FLAGS};
	
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
		, R_COUNT
		, R_VALVE_POS
		, R_FROM_POS
		, R_RATIO	
		, R_FROM_TEMP
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
		, R_SETTLE_TIME
		, R_DEFAULT_FLOW_TEMP
		, R_VERSION_MONTH
		, R_VERSION_DAY
		, MV_ALL_REG_SIZE // = 17
	};

	enum Mode {e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff, e_Error };
	enum Journey {e_Moving_Coolest = -2, e_CoolNorth, e_TempOK, e_WarmSouth};
	enum MotorDirection {e_Cooling = -1, e_Stop, e_Heating};
	using I2C_Flags_Obj = flag_enum::FE_Obj<MV_Device_State, _NO_OF_FLAGS>;
	using I2C_Flags_Ref = flag_enum::FE_Ref<MV_Device_State, _NO_OF_FLAGS>;

	Mix_Valve(I2C_Recovery::I2C_Recover& i2C_recover, uint8_t defaultTSaddr, HardwareInterfaces::Pin_Wag & _heat_relay, HardwareInterfaces::Pin_Wag & _cool_relay, EEPROMClass & ep, int reg_offset);
	void begin(int defaultFlowTemp);
	const __FlashStringHelper* name() const;
	void check_flow_temp();
	void setDefaultRequestTemp();
	bool doneI2C_Coms(I_I2Cdevice& programmer, bool newSecond);
	i2c_registers::RegAccess registers() const {return { mixV_registers, _regOffset };}
private:
	friend class TestMixV;
	enum { e_MIN_FLOW_TEMP = HardwareInterfaces::MIN_FLOW_TEMP, e_MIN_RATIO = 2, e_MAX_RATIO = 30};

	//friend void testMixer();
	//friend void testSlave();
	//friend void printModes();

	void stateMachine();
	bool valveIsAtLimit();
	void saveToEEPROM();
	bool checkForNewReqTemp();
	void refreshRegisters();
	Mode newTempMode();
	bool continueMove();
	bool continueWait();
	bool continueChecking();

	void adjustValve(int tempDiff);
	bool activateMotor_isMoving(); // Return true if is moving
	void stopMotor();
	void startWaiting();
	void loadFromEEPROM();
	void turnValveOff();

	int measurePSUVoltage(int period_mS = 25);

	// Object state
	HardwareInterfaces::TempSensor _temp_sensr;
	uint8_t _regOffset;

	// Injected dependancies
	HardwareInterfaces::Pin_Wag * _heat_relay;
	HardwareInterfaces::Pin_Wag * _cool_relay;
	EEPROMClass * _ep;

	// Temporary Data
	int16_t _onTime = 0; // +ve to move, -ve to wait.
	int16_t _valvePos = HardwareInterfaces::VALVE_TRANSIT_TIME;
	int16_t _moveFromPos = 0;
	Journey _journey = e_TempOK; // the requirement to heat or cool, not the actual motion of the valve.
	MotorDirection _motorDirection = e_Stop;

	uint8_t _currReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _newReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _flowTempAtStartOfWait = e_MIN_FLOW_TEMP;
	uint8_t _psuV;
	static Mix_Valve * motor_mutex; // address of Mix_valve is owner of the mutex
	static bool motor_queued; // address of Mix_valve is owner of the mutex
	static uint16_t _motorsOffV;
	static constexpr uint16_t _MOTORS_ON_DIFF_V = 40;
};