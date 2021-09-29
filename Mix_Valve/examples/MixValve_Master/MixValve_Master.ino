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
};

enum SlaveRequestIni {
	NO_INI_REQUESTS
	, MV_US_REQUESTING_INI = 1
	, MV_DS_REQUESTING_INI = 2
	, ALL_REQUESTING = (MV_DS_REQUESTING_INI * 2) - 1
};

i2c_registers::Registers<SIZE_OF_ALL_REGISTERS> _prog_registers{ i2C };
uint8_t previous_valveStatus[NO_OF_MIXERS];

uint8_t _mixCallTemp[] = { 35, 55 };
uint8_t _regOffset[] = { MV_REG_MASTER_0_OFFSET , MV_REG_MASTER_1_OFFSET };
uint8_t slaveRegOffset[] = { MV_REG_SLAVE_0_OFFSET, MV_REG_SLAVE_1_OFFSET };
uint8_t _flowTS_addr[] = { US_FLOW_TEMPSENS_ADDR , DS_FLOW_TEMPSENS_ADDR };

void sendSlaveIniData(int i);
void logMixValveOperation(int i, bool logThis);
const __FlashStringHelper* showState(int i);
void sendRequestFlowTemp(int i, uint8_t callTemp);
uint8_t getReg(int i, int reg);
void setReg(int i, int reg, uint8_t value);
Error_codes writeToValve(int i, int reg, uint8_t value);
void readRegistersFromValve(int i);

void setup()
{
	logger().begin(SERIAL_RATE);
	logger() << F("Start") << L_flush;
	i2C.setAsMaster(PROGRAMMER_I2C_ADDR);
	i2C.setMax_i2cFreq(100000);
	i2C.setTimeouts(15000,I2C_Talk::WORKING_STOP_TIMEOUT);
	i2C.onReceive(_prog_registers.receiveI2C);
	i2C.onRequest(_prog_registers.requestI2C);
	i2C.begin();
	sendSlaveIniData(0);
	sendSlaveIniData(1);
}

void loop()
{
	// All Mix-Valve I2C transfers are initiated by this Master
	static int loopCount = 0;
	if (_prog_registers.getRegister(R_SLAVE_REQUESTING_INITIALISATION)) {
		sendSlaveIniData(0);
		sendSlaveIniData(1);
	}

	if (loopCount == 0) {
		loopCount = 10;
		auto temp = _mixCallTemp[0];
		_mixCallTemp[0] = _mixCallTemp[1];
		_mixCallTemp[1] = temp;
		sendRequestFlowTemp(0,_mixCallTemp[0]);
		sendRequestFlowTemp(1,_mixCallTemp[1]);
	}
	readRegistersFromValve(0);
	readRegistersFromValve(1);
	logMixValveOperation(0,true);
	logMixValveOperation(1,true);
	--loopCount;
	delay(1000);
}

void sendSlaveIniData(int i) {
	auto errCode = _OK;
	do {
		Serial.println(F("Sending INI to Slaves"));
		setReg(i, Mix_Valve::R_MV_REG_OFFSET, slaveRegOffset[i]);
		errCode = writeToValve(i, Mix_Valve::R_MV_REG_OFFSET, _regOffset[i]);
		errCode |= writeToValve(i, Mix_Valve::R_TS_ADDRESS, _flowTS_addr[i]);
		errCode |= writeToValve(i, Mix_Valve::R_FULL_TRAVERSE_TIME, VALVE_TRANSIT_TIME);
		errCode |= writeToValve(i, Mix_Valve::R_SETTLE_TIME, VALVE_WAIT_TIME);
		setReg(i, Mix_Valve::R_STATUS, Mix_Valve::MV_OK);
		setReg(i, Mix_Valve::R_MODE, Mix_Valve::e_Checking);
		setReg(i, Mix_Valve::R_STATE, Mix_Valve::e_Stop);
		setReg(i, Mix_Valve::R_RATIO, 30);
		setReg(i, Mix_Valve::R_FROM_TEMP, 55);
		setReg(i, Mix_Valve::R_COUNT, 0);
		setReg(i, Mix_Valve::R_VALVE_POS, 70);
		setReg(i, Mix_Valve::R_FROM_POS, 75);
		setReg(i, Mix_Valve::R_FLOW_TEMP, 55);
		setReg(i, Mix_Valve::R_REQUEST_FLOW_TEMP, 25);
		setReg(i, Mix_Valve::R_MAX_FLOW_TEMP, 55);
		i2C.write(MIX_VALVE_I2C_ADDR, getReg(i, Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS, Mix_Valve::R_MAX_FLOW_TEMP, _prog_registers.reg_ptr(_regOffset[i] + Mix_Valve::R_STATUS));
	} while (errCode != _OK);
	_prog_registers.setRegister(R_SLAVE_REQUESTING_INITIALISATION, 0);
	logger() << F("MixValveController::sendSlaveIniData()") << i2C.getStatusMsg(errCode) << L_endl;
}

void logMixValveOperation(int i, bool logThis) {
	auto algorithmMode = Mix_Valve::Mix_Valve::Mode(getReg(i, Mix_Valve::R_MODE)); // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq
	if (algorithmMode >= Mix_Valve::e_Error) {
		logger() << F("MixValve Error: ") << algorithmMode << L_endl;
	}

	bool ignoreThis = (algorithmMode == Mix_Valve::e_Checking && getReg(i, Mix_Valve::R_FLOW_TEMP) == _mixCallTemp[i])
		|| algorithmMode == Mix_Valve::e_AtLimit;

	if (logThis || !ignoreThis || previous_valveStatus[i] != algorithmMode) {
		previous_valveStatus[i] = algorithmMode;
		logger() << L_time << L_tabs
			<< (i==0 ? F("US_Mix Req: ") : F("DS_Mix Req: ")) << _mixCallTemp[i] << F(" is ") << getReg(i, Mix_Valve::R_FLOW_TEMP)
			<< showState(i) << getReg(i, Mix_Valve::R_COUNT) << getReg(i, Mix_Valve::R_VALVE_POS) << getReg(i, Mix_Valve::R_RATIO)
			<< getReg(i, Mix_Valve::R_FROM_POS) << getReg(i, Mix_Valve::R_FROM_TEMP) << L_endl;
	}
}

const __FlashStringHelper* showState(int i) {
	switch (getReg(i, Mix_Valve::R_MODE)) { // e_Moving, e_Wait, e_Checking, e_Mutex, e_NewReq, e_AtLimit, e_DontWantHeat
	case Mix_Valve::e_Wait: return F("Wt");
	case Mix_Valve::e_Checking: return F("Chk");
	case Mix_Valve::e_Mutex: return F("Mx");
	case Mix_Valve::e_NewReq: return F("Req");
	case Mix_Valve::e_AtLimit: return F("Lim");
	case Mix_Valve::e_DontWantHeat: return F("Min");
	case Mix_Valve::e_Moving:
		switch (int8_t(getReg(i, Mix_Valve::R_STATE))) { // e_Moving_Coolest, e_Cooling = -1, e_Stop, e_Heating
		case Mix_Valve::e_Moving_Coolest: return F("Min");
		case Mix_Valve::e_Cooling: return F("Cl");
		case Mix_Valve::e_Heating: return F("Ht");
		default: return F("Stp"); // Now stopped!
		}
	default: return F("SEr");
	}
}

void sendRequestFlowTemp(int i, uint8_t callTemp) {
	_mixCallTemp[i] = callTemp;
	setReg(i, Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp[i]);
	writeToValve(i, Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp[i]);
}

uint8_t getReg(int i, int reg) {
	return _prog_registers.getRegister(_regOffset[i] + reg);
}

void setReg(int i, int reg, uint8_t value) {
	//logger() << F("MixValveController::setReg:") << _regOffset + reg << ":" << value << L_endl;
	_prog_registers.setRegister(_regOffset[i] + reg, value);
}

Error_codes writeToValve(int i, int reg, uint8_t value) {
	// All writable registers are single-byte
	auto status = i2C.write(MIX_VALVE_I2C_ADDR, getReg(i, Mix_Valve::R_MV_REG_OFFSET) + reg, 1, &value); // recovery-write
	logger() << F("Mix_Valve::write reg: ") << getReg(i, Mix_Valve::R_MV_REG_OFFSET) + reg << F(" Val: ") << value << I2C_Talk::getStatusMsg(status) << L_endl;
	return status;
}

void readRegistersFromValve(int i) {
	logger() << F("Read :") << Mix_Valve::R_MAX_FLOW_TEMP - Mix_Valve::R_STATUS << F(" MVreg from: ") << getReg(i, Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS << F(" to : ") << _regOffset[i] + Mix_Valve::R_STATUS << L_endl;
	i2C.read(MIX_VALVE_I2C_ADDR, getReg(i, Mix_Valve::R_MV_REG_OFFSET) + Mix_Valve::R_STATUS, Mix_Valve::R_MAX_FLOW_TEMP - Mix_Valve::R_STATUS, _prog_registers.reg_ptr(_regOffset[i] + Mix_Valve::R_STATUS));

	if (getReg(i, Mix_Valve::R_REQUEST_FLOW_TEMP) != _mixCallTemp[i]) {
		logger() << F("Mix_Valve::R_REQUEST_FLOW_TEMP :") << getReg(i, Mix_Valve::R_REQUEST_FLOW_TEMP) << F(" Req: ") << _mixCallTemp[i] << L_endl;
		logger() << F("Mix_Valve::R_FLOW_TEMP :") << getReg(i, Mix_Valve::R_FLOW_TEMP) << L_endl;
		setReg(i, Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp[i]);
		auto status = writeToValve(i, Mix_Valve::R_REQUEST_FLOW_TEMP, _mixCallTemp[i]);
	}
}
