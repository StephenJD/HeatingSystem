#include <Arduino.h>
#include <I2C_Talk.h>
#include <I2C_Registers.h>
#include <Logging.h>
#include <Mix_Valve.h>
#include <../HeatingSystem/src/HardwareInterfaces/A__Constants.h>

#define I2C_RESET 14
constexpr uint32_t SERIAL_RATE = 115200;

//////////////////////////////// Start execution here ///////////////////////////////
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;

namespace arduino_logger {
	Logger& logger() {
		static Serial_Logger _log(SERIAL_RATE, L_startWithFlushing);
		return _log;
	}
}
using namespace arduino_logger;

//EEPROMClass& eeprom() {
//	return EEPROM;
//}

I2C_Talk i2C{}; // default 400kHz

// All Mix-Valve I2C transfers are initiated by this Master
enum Register_Constants {
	R_SLAVE_REQUESTING_INITIALISATION = 0
	, MV_REG_MASTER_0_OFFSET = 1
	, MV_REG_MASTER_1_OFFSET = MV_REG_MASTER_0_OFFSET + Mix_Valve::R_MV_VOLATILE_REG_SIZE
	, MV_REG_SLAVE_0_OFFSET = 0
	, MV_REG_SLAVE_1_OFFSET = MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_MV_ALL_REG_SIZE
	, SIZE_OF_ALL_REGISTERS = MV_REG_MASTER_0_OFFSET + Mix_Valve::R_MV_VOLATILE_REG_SIZE * NO_OF_MIXERS
	, MV_REQUESTING_INI = 1
}; 

i2c_registers::Registers<SIZE_OF_ALL_REGISTERS> mixV_x2_registers{ i2C };
uint8_t previous_valveStatus[NO_OF_MIXERS];

uint8_t _mixCallTemp = 35;
uint8_t _mixCallTemp2 = 55;

void sendSlaveIniData() {
	auto err = _OK;
	do {
		Serial.println(F("Sending INI to Slaves"));
		err = i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_TS_ADDRESS, DS_TEMPSENS_ADDR);
		if (err) logger() << F("IniErr:") << i2C.getStatusMsg(err) << L_endl;
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET + Mix_Valve::R_TS_ADDRESS, US_TEMPSENS_ADDR);
		
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_FULL_TRAVERSE_TIME, 130);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET + Mix_Valve::R_FULL_TRAVERSE_TIME, 150);
		
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_SETTLE_TIME, 10);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET + Mix_Valve::R_SETTLE_TIME, 15);
		
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_MAX_FLOW_TEMP, 60);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET + Mix_Valve::R_MAX_FLOW_TEMP, 70);
		
		mixV_x2_registers.setRegister(R_SLAVE_REQUESTING_INITIALISATION, 0);
	} while (err != _OK);
}

void sendReqTempsToMixValve() {
	logger() << F("Master Sent: ") << _mixCallTemp << L_endl;
	auto err = i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
	err |= i2C.write(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp2);
	
	if (err == _StopMarginTimeout) {
		i2C.setStopMargin(i2C.stopMargin() + 1);
		logger() << F("WriteErr. New StopMargin:") << i2C.stopMargin() << L_endl;
	}
}

void requestSlaveData() {
	i2C.read(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET, Mix_Valve::R_MV_VOLATILE_REG_SIZE, mixV_x2_registers.reg_ptr(MV_REG_MASTER_0_OFFSET));
	i2C.read(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET, Mix_Valve::R_MV_VOLATILE_REG_SIZE, mixV_x2_registers.reg_ptr(MV_REG_MASTER_1_OFFSET));
}

bool slaveRequestsResendRequestTemp() {
	// All Mix-Valve I2C transfers are initiated by this Master
	i2C.read(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_0_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP, 1, mixV_x2_registers.reg_ptr(MV_REG_MASTER_0_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP));
	i2C.read(MIX_VALVE_I2C_ADDR, MV_REG_SLAVE_1_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP, 1, mixV_x2_registers.reg_ptr(MV_REG_MASTER_1_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP));
	return (mixV_x2_registers.getRegister(MV_REG_MASTER_0_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP) == 0 || mixV_x2_registers.getRegister(MV_REG_MASTER_1_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP) == 0);
}

void setup()
{
	logger().begin(SERIAL_RATE);
	logger() << F("Start") << L_flush;
  i2C.setAsMaster(PROGRAMMER_I2C_ADDR);
  i2C.setMax_i2cFreq(100000);
  i2C.setTimeouts(15000,I2C_Talk::WORKING_STOP_TIMEOUT);
  i2C.onReceive(mixV_x2_registers.receiveI2C);
  i2C.onRequest(mixV_x2_registers.requestI2C);
  i2C.begin();
  sendSlaveIniData();
}

void loop()
{
	// All Mix-Valve I2C transfers are initiated by this Master
  static int loopCount = 0;
  if (mixV_x2_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION)) sendSlaveIniData();

  if (loopCount == 0) {
	  loopCount = 10;
	  auto temp = _mixCallTemp;
	  _mixCallTemp = _mixCallTemp2;
	  _mixCallTemp2 = temp;

	  mixV_x2_registers.setRegister(MV_REG_MASTER_0_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp);
	  mixV_x2_registers.setRegister(MV_REG_MASTER_1_OFFSET + Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp2);
	  sendReqTempsToMixValve();
  }
  requestSlaveData();
  if (slaveRequestsResendRequestTemp()) sendReqTempsToMixValve();
  logMixValveOperation(1,true);
  logMixValveOperation(1+ Mix_Valve::R_MV_VOLATILE_REG_SIZE,true);
  --loopCount;
 delay(1000);
}

void logMixValveOperation(int regOffset, bool logThis) {
	int index = regOffset < Mix_Valve::R_MV_VOLATILE_REG_SIZE ? 0 : 1;
	auto algorithmMode = mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_MODE); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
	if (algorithmMode >= Mix_Valve::e_Error) {
		logger() << "MixValve Error: " << algorithmMode << L_endl;
	}

	bool ignoreThis = (algorithmMode == Mix_Valve::e_Checking && mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_FLOW_TEMP) == _mixCallTemp)
		|| algorithmMode == Mix_Valve::e_AtLimit;

	if (logThis || !ignoreThis || previous_valveStatus[index] != algorithmMode) {
		previous_valveStatus[index] = algorithmMode;
		auto motorActivity = mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_STATE); // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
		logger() << L_time << L_tabs
			<< (index==0 ? F("DS_Mix Req: ") : F("US_Mix Req: ")) << _mixCallTemp << F(" is ") << mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_FLOW_TEMP)
			<< showState(index, algorithmMode, motorActivity) << mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_COUNT) << mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_VALVE_POS) << mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_RATIO)
			<< mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_FROM_POS) << mixV_x2_registers.getRegister(regOffset + Mix_Valve::R_FROM_TEMP) << L_endl;
	}
}

const __FlashStringHelper* showState(int index, int algorithmMode, int motorActivity) {
	switch (algorithmMode) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
	case Mix_Valve::e_Wait: return F("Wt");
	case Mix_Valve::e_Checking: return F("Chk");
	case Mix_Valve::e_Mutex: return F("Mx");
	case Mix_Valve::e_NewReq: return F("Req");
	case Mix_Valve::e_AtLimit: return F("Lim");
	case Mix_Valve::e_DontWantHeat: return F("Min");
	case Mix_Valve::e_Moving:
		switch (motorActivity) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
		case Mix_Valve::e_Moving_Coolest: return F("Min");
		case Mix_Valve::e_Cooling: return F("Cl");
		case Mix_Valve::e_Heating: return F("Ht");
		default: return F("Stp"); // Now stopped!
		}
	default: return F("SEr");
	}
}