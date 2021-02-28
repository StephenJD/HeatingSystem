#include <Arduino.h>
#include <I2C_Talk.h>
#include <Logging.h>
#include <Mix_Valve.h>

#define I2C_RESET 14
#define SERIAL_RATE 115200
constexpr uint8_t MIX_VALVE_I2C_ADDR = 0x10;

//////////////////////////////// Start execution here ///////////////////////////////
using namespace HardwareInterfaces;
using namespace I2C_Talk_ErrorCodes;

Logger & logger() {
	static Serial_Logger _log(SERIAL_RATE);
	return _log;
}

I2C_Talk i2C; // default 400kHz



void setup()
{
  logger() << L_allwaysFlush << F("Start") << L_endl;
  i2C.setI2CFrequency(100000);
  i2C.setTimeouts(15000,I2C_Talk::WORKING_STOP_TIMEOUT);
  i2C.begin();
}

byte x0 = 25;
byte x1 = 55;

void loop()
{
  static int loopCount = 0;
  if (loopCount == 0) {
	  loopCount = 10;
	  x0 = (x0 == 25 ? 55 : 25) ;
	  x1 = (x0 == 25 ? 55 : 25) ;
	  byte data[2];

	  logger() << F("Master Sent: ") << x0 << F(" then: ") << x1 << L_endl; 
	  auto err = i2C.write(MIX_VALVE_I2C_ADDR, Mix_Valve::request_temp, x0);
	  err |= i2C.write(MIX_VALVE_I2C_ADDR, Mix_Valve::flow_temp, x1);
	  if (err == _StopMarginTimeout) {
		i2C.setStopMargin(i2C.stopMargin() + 1);
		logger() << F("WriteErr. New StopMargin:") << i2C.stopMargin() << L_endl;
	  }

	  err = i2C.read(MIX_VALVE_I2C_ADDR, Mix_Valve::request_temp, 1, data);
	  err |= i2C.read(MIX_VALVE_I2C_ADDR, Mix_Valve::flow_temp, 1, &data[1]);
	  if (err == _StopMarginTimeout) {
		i2C.setStopMargin(i2C.stopMargin() + 1);
		logger() << F("ReadErr. New StopMargin:") << i2C.stopMargin() << L_endl;
	  }	  
	  logger() << F("Slave Sent ") << data[0] << F(" : ") << data[1] <<L_endl;
  }
  int16_t mode;
  int16_t pos;
  int16_t state;

  i2C.read(MIX_VALVE_I2C_ADDR, Mix_Valve::mode, 2, (uint8_t*)&mode);
  if (mode == Mix_Valve::e_Checking) i2C.read(MIX_VALVE_I2C_ADDR, Mix_Valve::valve_pos, 2, (uint8_t*)&pos);
  else i2C.read(MIX_VALVE_I2C_ADDR, Mix_Valve::count, 2, (uint8_t*)&pos);
  
  i2C.read(MIX_VALVE_I2C_ADDR, Mix_Valve::state, 2, (uint8_t*)&state);
  
  if (mode == 0 && state == 0) {
		logger() << F("MixValveController read error") << L_endl;
  }

  logger() << F("Valve Status:") << L_tabs << pos << showState(state, mode) << " State: " << state << " Action: " << mode << L_endl;

  readReg(Mix_Valve::mode);
  readReg(Mix_Valve::count);
  readReg(Mix_Valve::valve_pos);
  readReg(Mix_Valve::state);

  --loopCount;
 delay(1000);
}

const __FlashStringHelper * showState(int state, int mode) {
	switch (state) {
	case Mix_Valve::e_Moving_Coolest: return F("Min");
	case Mix_Valve::e_NeedsCooling: return F("Cl");
	case Mix_Valve::e_NeedsHeating: return F("Ht");
	case Mix_Valve::e_TempOK:
		switch (mode) {
		case Mix_Valve::e_Mutex: return F("Mx");
		case Mix_Valve::e_Wait : return F("Wt");
		case Mix_Valve::e_Checking: return F("Ck");
		default: return F("MEr");
		}
		break;
	default: return F("SEr");
	}
}

int16_t readReg(int reg) {
	int16_t value;
	i2C.read(MIX_VALVE_I2C_ADDR, reg, 2, (uint8_t*)&value);
	logger() << F("Raw-Read Reg 0x") << reg << " Val:" << L_dec << value << " : " << (value / 256) << L_endl;
	read_2bytes(reg, value);
	logger() << F("2-byte Read Reg 0x") << reg << " Val: " << L_dec << value << " : " << (value / 256) << L_endl;
}

void read_2bytes(int reg, int16_t & data) {
	// I2C devices use big-endianness: MSB at the smallest address: So a two-byte read is [MSB, LSB].
	// But Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
	// A two-byte read must reverse the byte-order.
	uint8_t dataBuffer[2];
	int16_t newRead;
	data = 0xAAAA; // 1010 1010 1010 1010
	i2C.read(MIX_VALVE_I2C_ADDR, reg, 2, dataBuffer);
	//logger() << F("Data[] Read Reg 0x") << reg << " Val: " << L_dec << dataBuffer[0] << " : " << dataBuffer[1] << L_endl;
	newRead = fromBigEndian(dataBuffer); // convert device to native endianness
	data = newRead;
}

