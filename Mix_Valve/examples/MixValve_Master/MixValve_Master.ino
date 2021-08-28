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
		static Serial_Logger _log(SERIAL_RATE);
		return _log;
	}
}
using namespace arduino_logger;

//EEPROMClass& eeprom() {
//	return EEPROM;
//}

I2C_Talk i2C{}; // default 400kHz

constexpr auto slave_requesting_initialisation = 0;
constexpr auto mix_register_offset = 1;
constexpr int SIZE_OF_ALL_REGISTERS = 1 + Mix_Valve::mixValve_volRegister_size * NO_OF_MIXERS;
i2c_registers::Registers<SIZE_OF_ALL_REGISTERS> all_register_set{ i2C };
uint8_t valveStatus[NO_OF_MIXERS];
//i2c_registers::I_Registers& all_registers{ all_register_set };

void sendSlaveIniData() {
	auto err = _OK;
	do {
		Serial.println(F("Sending INI to Slaves"));
		err = i2C.write(MIX_VALVE_I2C_ADDR, 0, mix_register_offset);
		if (err) logger() << F("IniErr:") << i2C.getStatusMsg(err) << L_endl;
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1+ Mix_Valve::temp_i2c_addr, DS_TEMPSENS_ADDR);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::mixValve_all_register_size + Mix_Valve::temp_i2c_addr, US_TEMPSENS_ADDR);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::full_traverse_time, 130);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::mixValve_all_register_size + Mix_Valve::full_traverse_time, 150);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::wait_time, 10);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::mixValve_all_register_size + Mix_Valve::wait_time, 15);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::max_flowTemp, 60);
		err |= i2C.write(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::mixValve_all_register_size + Mix_Valve::max_flowTemp, 70);
	} while (err != _OK);
	all_register_set.setRegister(slave_requesting_initialisation, 0);
}

void sendReqTempsToMixValve() {
	auto reqTemp = all_register_set.getRegister(1 + Mix_Valve::request_temp);
	logger() << F("Master Sent: ") << reqTemp << L_endl;
	auto err = i2C.write(MIX_VALVE_I2C_ADDR, 1+ Mix_Valve::request_temp, reqTemp);

	reqTemp = all_register_set.getRegister(1 + Mix_Valve::mixValve_volRegister_size + Mix_Valve::request_temp);

	err |= i2C.write(MIX_VALVE_I2C_ADDR, 1+ Mix_Valve::mixValve_all_register_size + Mix_Valve::flow_temp, reqTemp);
	
	if (err == _StopMarginTimeout) {
		i2C.setStopMargin(i2C.stopMargin() + 1);
		logger() << F("WriteErr. New StopMargin:") << i2C.stopMargin() << L_endl;
	}
}

void requestSlaveData() {
	i2C.read(MIX_VALVE_I2C_ADDR, 1, Mix_Valve::mixValve_volRegister_size, all_register_set.reg_ptr(1));
	i2C.read(MIX_VALVE_I2C_ADDR, 1 + Mix_Valve::mixValve_all_register_size, Mix_Valve::mixValve_volRegister_size, all_register_set.reg_ptr(1+ Mix_Valve::mixValve_volRegister_size));
}

void setup()
{
  logger() << L_allwaysFlush << F("Start") << L_endl;
  i2C.setAsMaster(PROGRAMMER_I2C_ADDR);
  i2C.setMax_i2cFreq(100000);
  i2C.setTimeouts(15000,I2C_Talk::WORKING_STOP_TIMEOUT);
  i2C.onReceive(all_register_set.receiveI2C);
  i2C.onRequest(all_register_set.requestI2C);
  i2C.begin();
  sendSlaveIniData();
}

uint8_t _mixCallTemp = 25;
uint8_t _mixCallTemp2 = 55;

void loop()
{
  static int loopCount = 0;
  if (all_register_set.getRegister(slave_requesting_initialisation)) sendSlaveIniData();

  if (loopCount == 0) {
	  loopCount = 10;
	  _mixCallTemp = (_mixCallTemp == 25 ? 55 : 25) ;
	  _mixCallTemp2 = (_mixCallTemp == 25 ? 55 : 25) ;

	  all_register_set.setRegister(1 + Mix_Valve::request_temp, _mixCallTemp);
	  all_register_set.setRegister(1 + Mix_Valve::mixValve_volRegister_size + Mix_Valve::request_temp, _mixCallTemp2);
	  sendReqTempsToMixValve();
  }

  requestSlaveData();
  logMixValveOperation(1,true);
  logMixValveOperation(1+ Mix_Valve::mixValve_volRegister_size,true);
  --loopCount;
 delay(1000);
}

void logMixValveOperation(int regOffset, bool logThis) {
	int index = regOffset < Mix_Valve::mixValve_volRegister_size ? 0 : 1;
	auto algorithmMode = all_register_set.getRegister(regOffset + Mix_Valve::mode); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
	if (algorithmMode >= Mix_Valve::e_Error) {
		logger() << "MixValve Error: " << algorithmMode << L_endl;
	}

	bool ignoreThis = (algorithmMode == Mix_Valve::e_Checking && all_register_set.getRegister(regOffset + Mix_Valve::flow_temp) == _mixCallTemp)
		|| algorithmMode == Mix_Valve::e_AtLimit;

	if (logThis || !ignoreThis || valveStatus[index] != algorithmMode) {
		valveStatus[index] = algorithmMode;
		auto motorActivity = all_register_set.getRegister(regOffset + Mix_Valve::state); // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
		logger() << L_time << L_tabs
			<< (index==0 ? F("DS_Mix Req: ") : F("US_Mix Req: ")) << _mixCallTemp << F(" is ") << all_register_set.getRegister(regOffset + Mix_Valve::flow_temp)
			<< showState(index, algorithmMode, motorActivity) << all_register_set.getRegister(regOffset + Mix_Valve::count) << all_register_set.getRegister(regOffset + Mix_Valve::valve_pos) << all_register_set.getRegister(regOffset + Mix_Valve::ratio)
			<< all_register_set.getRegister(regOffset + Mix_Valve::moveFromPos) << all_register_set.getRegister(regOffset + Mix_Valve::moveFromTemp) << L_endl;
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