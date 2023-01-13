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
//#define SIM_MIXV

extern i2c_registers::I_Registers& mixV_registers;

namespace HardwareInterfaces {
	class Pin_Wag;
}
namespace I2C_Recovery {
	class I2C_Recover;
}

template <int length, typename intType = int16_t >
class Integrator {
public:
	Integrator() : _noOfValues(length), _integrator{ 0 } {}
	Integrator(int noOfValues) : _noOfValues(noOfValues), _integrator{ 0 } {}
	// Queries
	float getAverage() const;
	float getSum() const;
	intType getVal(int pos) const;
	uint8_t getNoOfValues() const { return _noOfValues; };
	// Modifiers
	void setNoOfValues(int noOfValues) { _noOfValues = noOfValues; }
	void addValue(intType val);
	void prime(intType val);
	void clear();
private:
	intType _integrator[length];
	uint8_t _pos = 0;
	uint8_t _noOfValues;
};

template <int l, typename intType>
void Integrator<l, intType>::addValue(intType val) {
	_integrator[_pos] = val;
	++_pos;
	if (_pos >= _noOfValues) _pos = 0;
}

template <int l, typename intType>
intType Integrator<l, intType>::getVal(int pos) const {
	return _integrator[pos];
}

template <int l, typename intType>
void Integrator<l, intType>::clear() {
	for (auto& val : _integrator) val = intType(0);
}

template <int l, typename intType>
float Integrator<l, intType>::getSum() const {
	float sum = 0;
	for (int pos = 0; pos < _noOfValues; ++pos) sum += _integrator[pos];
	return sum;
}

template <int l, typename intType>
float Integrator<l, intType>::getAverage() const {
	return getSum() / float(_noOfValues);
}

template <int l, typename intType>
void Integrator<l, intType>::prime(intType val) {
	for (int pos = 0; pos < _noOfValues; ++pos) _integrator[pos] = val;
}


class Mix_Valve
{
public:
	// DATA_READ = 0x80 (1000,0000), DATA_SENT = 0x40 (0100,0000), EXCHANGE_COMPLETE = 0xC0 (1100,0000)
	enum : uint8_t {
		R_PROG_WAITING_FOR_REMOTE_I2C_COMS = 1
		, DEVICE_CAN_WRITE = 0x38, DEVICE_IS_FINISHED = 0x07 /* 00,111,000 : 00,000,111 */
		, DATA_SENT = 0xC0, DATA_READ = 0x40, EXCHANGE_COMPLETE = 0x80 /* 11,000,000 : 01,000,000 : 10,000,000 */
		, HANDSHAKE_MASK = 0xC0, DATA_MASK = uint8_t(~HANDSHAKE_MASK) /* 11,000,000 : 00,111,111 */
	};
	enum MV_Device_State {F_EXCHANGE_COMPLETE, F_I2C_NOW, F_NO_PROGRAMMER, F_DS_TS_FAILED, F_US_TS_FAILED, F_NEW_TEMP_REQUEST, F_STORE_TOO_COOL, F_RECEIVED_INI, _NO_OF_FLAGS};
	enum MixValve_Volatile_Register_Names {
		// Registers provided by MixValve_Slave
		// Copies of the VOLATILE set provided in Programmer reg-set
		// All registers are single-byte.
		// If the valve is Slave and receives F_I2C_NOW it reads its own temp sensors.
		// otherwise, Programmer reads TS & writes temps to the registers.
		// Elsewhere it obtains the temprature from its registers.
		// All I2C transfers are initiated by Programmer: Reading status & sending new requests.

		// Receive
		R_REMOTE_REG_OFFSET // offset in destination reg-set, used by Master - not used by slave
		// Send on request
		, R_DEVICE_STATE	// MV_Device_State FlagEnum	
		, R_MODE	// Algorithm Mode
		, R_MOTOR_ACTIVITY	// Motor activity: e_Moving_Coolest, e_Cooling, e_Stop, e_Heating
		, R_COUNT
		, R_VALVE_POS
		, R_PSU_V
		, R_RATIO
		, R_ADJUST_MODE
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

	enum Mode { e_NewReq, e_Moving, e_Wait, e_Mutex, e_Checking, e_HotLimit, e_WaitToCool, e_ValveOff, e_StopHeating, e_FindOff, e_Error };
	enum Tune { init, findOff, waitForCool, riseToSetpoint, findMax, fallToSetPoint, findMin, lastRise, calcPID, turnOff, restart };
	enum Journey { e_Moving_Coolest = -2, e_CoolNorth, e_TempOK, e_WarmSouth };
	enum MotorDirection { e_Cooling = -1, e_Stop, e_Heating };
	enum { PSUV_DIVISOR = 5 };
	enum AdjustMode : uint8_t { A_GOOD_RATIO, A_UNDERSHOT, A_OVERSHOT, PID_CHECK };
	using I2C_Flags_Obj = flag_enum::FE_Obj<MV_Device_State, _NO_OF_FLAGS>;
	using I2C_Flags_Ref = flag_enum::FE_Ref<MV_Device_State, _NO_OF_FLAGS>;

	Mix_Valve(I2C_Recovery::I2C_Recover& i2C_recover, uint8_t defaultTSaddr, HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay, EEPROMClass& ep, int reg_offset);
#ifdef SIM_MIXV
	Mix_Valve(I2C_Recovery::I2C_Recover& i2C_recover, uint8_t defaultTSaddr, HardwareInterfaces::Pin_Wag& _heat_relay, HardwareInterfaces::Pin_Wag& _cool_relay, EEPROMClass& ep, int reg_offset
		, uint16_t timeConst, uint16_t delay, uint8_t maxTemp);
#endif
	void begin(int defaultFlowTemp);
	const __FlashStringHelper* name() const;
	bool ts_OK() const;
	void check_flow_temp();
	void changeRole(bool isMaster);
	bool doneI2C_Coms(I_I2Cdevice& programmer, bool newSecond);
	i2c_registers::RegAccess registers() const { return { mixV_registers, _regOffset }; }
	uint8_t getPIDconstants();
	void logPID();
#ifdef SIM_MIXV
	void simulateFlowTemp();
	void set_maxTemp(uint8_t max) { _maxTemp = max; }
#endif
private:
	friend class TestMixV;
	enum { e_MIN_FLOW_TEMP = HardwareInterfaces::MIN_FLOW_TEMP, e_MIN_RATIO = 2, e_MAX_RATIO = 30 };

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
	void moveValveTo(int pos);
	int measurePSUVoltage(int period_mS = 25);
	void runPIDstate();
	bool receive_handshakeData(volatile uint8_t& data);
	bool endMaster(I_I2Cdevice& programmer, uint8_t remoteReg);
	// Object state
	HardwareInterfaces::TempSensor _temp_sensr;
	uint8_t _regOffset;

	// Injected dependancies
	HardwareInterfaces::Pin_Wag* _heat_relay;
	HardwareInterfaces::Pin_Wag* _cool_relay;
	EEPROMClass* _ep;

	// Temporary Data
	int16_t _onTime = 0; // +ve to move, -ve to wait.
	int16_t _valvePos = HardwareInterfaces::VALVE_TRANSIT_TIME;
	Journey _journey = e_TempOK; // the requirement to heat or cool, not the actual motion of the valve.
	MotorDirection _motorDirection = e_Stop;

	uint8_t _currReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _newReqTemp = 0; // Must have two the same to reduce spurious requests
	uint8_t _flowTempAtStartOfWait = e_MIN_FLOW_TEMP;
	uint8_t _flowTempFract = 0;
	Integrator<10> _integrator{};

	// PID Constants
	Tune _pidState = init;
	uint16_t _half_period, _period;
	int16_t _min_temp_16ths, _max_temp_16ths;
#ifdef SIM_MIXV
	int16_t _actualFlowTemp_16ths;
	uint16_t _timeConst = 0;
	Integrator<100> _delayLine{};
	float _reportedTemp = 25;
	uint8_t _maxTemp;
#endif

	// Shared Motor Supply
	static Mix_Valve* motor_mutex; // address of Mix_valve is owner of the mutex
	static bool motor_queued; // address of Mix_valve is owner of the mutex
	static int16_t _motorsOffV;
	static int16_t _motors_off_diff_V;
};