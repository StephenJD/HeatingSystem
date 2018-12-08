#pragma once
#include "A__Constants.h"
#include "D_Operations.h"
#include "D_EpData.h"
#include "I2C_Helper.h"

U1_byte getMixArduinoFailed(I2C_Helper & i2c, int addr);
uint8_t remoteDisplayFailed(I2C_Helper & i2c, int addr);
uint8_t tempSensorFailed(I2C_Helper & i2c, int addr);
uint8_t resetI2C(I2C_Helper & i2c, int addr);
U1_byte hardReset_Performed(I2C_Helper & i2c, int addr);
bool i2C_is_released();
uint8_t remoteDisplayIni(int addr);
uint8_t initialiseRemoteDisplaysFailed();
I2C_Helper::TestFnPtr getTestFunction(int addr);
U1_byte speedTestDevice(int addr);
uint8_t initialisePortsFailed(I2C_Helper & i2c, int addr);

class Relay_Run : public RunT<EpDataT<RELAY_DEF> > {
public:
	Relay_Run();
	bool getRelayState(bool readFromPort = false);
	bool setRelay(U1_byte state); // returns true if state is changed
	//void showMem();

	static S1_err testRelays();
	//static void testI2Cdevices();
	static S1_err resetSpeed(U1_byte failedRelayIndex = 0,U4_byte maxSpeed = 0);
	static U4_byte getI2CFrequency();
	static void setI2CFrequency(U4_byte freq){I2C_freq = freq;}
	static S1_err refreshRelays();
	static modes switch_mode;
	static U1_byte optRelayRegister; // copies of registers in ports
	//static U1_byte mixRelayRegister;
	static S1_err writeToRelay(U1_byte deviceAddr, U1_byte data);
	static bool I2C_OK;
private:
	static S1_err setAndTestRegister(U1_byte deviceAddr, U1_byte data); // returns 0 sucess or error codes, e.g.  ERR_PORTS_FAILED & setRelay codes
	static signed char i2cZWrite(uint8_t deviceAddr,uint8_t reg, uint8_t data);
	
	static U4_byte I2C_freq;
	static void resetRelays();
	static void reportErrors();
	static bool fixOPDir(uint8_t deviceAddr);
	//static bool canReduceI2CSpeed(int addr);
	static S1_err showSpeedTestFailed(I2C_Helper::TestFnPtr testFn, const char * device);
	static U1_byte prevOptRelayRegister; // copies of registers in ports
	//static U1_byte prevMixRelayRegister;
	static U1_count noOfErrors;
	static U1_count rewriteSuccess;
	static U1_count reduceSpeedSuccess;
	static U1_count iniSuccess;
	static U1_count resetSuccess;
	static U1_count latch_port_disagree;


};

template <int noDevices> class I2C_Helper_Auto_Speed;
extern I2C_Helper_Auto_Speed<27> * i2C;
class TwoWire;

