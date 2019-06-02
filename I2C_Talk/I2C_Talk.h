#pragma once

#include <Arduino.h>
#include <inttypes.h>
#include <Wire.h>
#include <I2C_Recover.h>

/**
Using this library requires a modified wire.cpp (Sam) or twi.c (AVR)
found in C:\Users\[UserName]\AppData\Roaming [ or Local] \Arduino15\packages\arduino\hardware\sam\1.6.4\libraries\Wire\src
or C:\Program Files (x86)\Arduino1.5\hardware\arduino\avr\libraries\Wire\
Small mods required so that in the event of a time-out Wire.endTransmission() returns error 1
and requestFrom() returns 0.
This in turn requires small mods to TWI_WaitTransferComplete(), TWI_WaitByteSent(), TWI_WaitByteReceived().
This library takes an optional void timoutFunction() as an argument to the setTimeoutFn method.
If supplied, the supplied function will get called whenever there is a time_out, allowing the I2C devices to be reset.
It should make a call to I2C_Talk.timeout_reset().
*/

#ifndef VARIANT_MCK
	#define VARIANT_MCK F_CPU
#endif

// The DEFAULT page size for I2C EEPROM.
// I2C_EEPROM_PAGESIZE must be multiple of 2 e.g. 16, 32 or 64
// 24LC256 -> 64 bytes
#define I2C_EEPROM_PAGESIZE 32

class I2C_Talk {
public:
	//class I_I2CresetFunctor {
	//public:
	//	virtual uint8_t operator()(I2C_Talk & i2c, int addr) = 0;
	//};

	//typedef uint8_t (*TestFnPtr)(I2C_Talk &, int);
	
	// Basic Usage //
	enum error_codes {_OK, _Insufficient_data_returned, _NACK_during_address_send, _NACK_during_data_send, _NACK_during_complete, _NACK_receiving_data, _Timeout, _speedError, _slave_shouldnt_write, _I2C_not_created, _I2C_Device_Not_Found, _I2C_ReadDataWrong, _I2C_AddressOutOfRange};
	enum {_single_master = 255, _no_address = 255};

	I2C_Talk(TwoWire &wire_port = Wire, int32_t i2cFreq = 100000);
	
	uint8_t read(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer); // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t read(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, char *dataBuffer) {return read(deviceAddr, registerAddress, numberBytes, (uint8_t *) dataBuffer);}
	uint8_t readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer); // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, char *dataBuffer) { return readEP(deviceAddr, pageAddress, numberBytes, (uint8_t *)dataBuffer); }
	uint8_t write(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer); // Return errCode.
	uint8_t write(uint16_t deviceAddr, uint8_t registerAddress, uint8_t data); // Return errCode.
	uint8_t write_verify(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer); // Return errCode.
	uint8_t writeEP(uint16_t deviceAddr, int pageAddress, uint8_t data); // Return errCode. Writes 32-byte pages. #define I2C_EEPROM_PAGESIZE
	uint8_t writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer); // Return errCode.
	uint8_t writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, char *dataBuffer) {return writeEP(deviceAddr, pageAddress, numberBytes, (const uint8_t *)dataBuffer); }
	// Slave response
	uint8_t write(const uint8_t *dataBuffer, uint16_t numberBytes); // Called by slave in response to request from a Master. Return errCode.
	uint8_t write(const char *dataBuffer) {return write((const uint8_t*)dataBuffer, (uint8_t)strlen(dataBuffer)+1);}// Called by slave in response to request from a Master. Return errCode.
	
	void writeAtZeroCross() { _waitForZeroCross = true; }
	static uint8_t notExists(I2C_Talk & i2c, int deviceAddr);
	uint8_t notExists(int deviceAddr);
	
	// Enhanced Usage //
	//I2C_Talk(TwoWire &wire_port, int8_t zxPin, uint8_t retries, uint16_t zxDelay, I_I2CresetFunctor * timeoutFunction = 0, int32_t i2cFreq = 100000);
	I2C_Talk(TwoWire &wire_port, int8_t zxPin, uint16_t zxDelay, I2C_Recover * recovery = 0, int32_t i2cFreq = 100000);
	
	int32_t setI2CFrequency(int32_t i2cFreq); // turns auto-speed off
	int32_t getI2CFrequency() { return _i2cFreq; }
	virtual int32_t setThisI2CFrequency(int16_t devAddr, int32_t i2cFreq) {return setI2Cfreq_retainAutoSpeed(i2cFreq);}
	virtual int32_t getThisI2CFrequency(int16_t devAddr) {return getI2CFrequency();}

	//void setNoOfRetries(uint8_t retries);
	//uint8_t getNoOfRetries();
	//void setTimeoutFn (I_I2CresetFunctor * timeoutFnPtr);
	//void setTimeoutFn (TestFnPtr timeoutFnPtr);
	//I_I2CresetFunctor * getTimeoutFn() {return timeoutFunctor;}

	bool restart();
	virtual void useAutoSpeed(bool set = true) {}
	virtual bool usingAutoSpeed() const { return false; }
	
	void static setZeroCross(int8_t zxPin); // Arduino pin signalling zero-cross detected. 0 = disable, +ve = signal on rising edge, -ve = signal on falling edge
	void static setZeroCrossDelay(uint16_t zxDelay); // microseconds delay between signal on zero_cross_pin to next true zero-cross. Default = 700
	static const char * getError(int errorCode);
	
	// Slave Usage //
	// Where there is a single master, you can set the slave arduino using setAsSlave(), which prohibits it from initiating a write.
	// Where multiple arduinos need to be able to initiate writes (even if they are not talking to each other),
	// each must use the setAsMaster() function, providing a unique I2C address.
	// Multi_master mode enables each arduino to act as a master and also respond as a slave.
	// To switch off multi-master mode, call setAsMaster() without supplying an address.
	// To resond as a slave, the onReceive() and onRequest() functions must be set.
	// *** The onReceive() function must not do a Serial.print when it receives a register address prior to a read.
	I2C_Talk(int multiMaster_MyAddress, TwoWire &wire_port = Wire, int32_t i2cFreq = 100000);
	void setAsSlave(int slaveAddress) { _myAddress = slaveAddress; _canWrite = false; restart(); }
	void setAsMaster(int multiMaster_MyAddress = _single_master) {_myAddress = multiMaster_MyAddress; _canWrite = true; restart();}
	void onReceive(void(*fn)(int)) { wire_port.onReceive(fn); } // The supplied function is called when data is sent to a slave
	void onRequest(void(*fn)(void)){ wire_port.onRequest(fn); } // The supplied function is called when data is requested from a slave
	uint8_t receiveFromMaster(int howMany, uint8_t *dataBuffer); // Data is written to dataBuffer. Returns how many are written.
	uint8_t receiveFromMaster(int howMany, char *dataBuffer) {return receiveFromMaster(howMany, (uint8_t *) dataBuffer);}
	
	// required by template, may as well be publicly available
	static const int32_t MAX_I2C_FREQ = (VARIANT_MCK / 40) > 400000 ? 400000 : (VARIANT_MCK / 40); //100000; // 
	static const int32_t MIN_I2C_FREQ = VARIANT_MCK / 65288 * 2; //32644; //VARIANT_MCK / 65288; //36000; //
	//uint8_t successAfterRetries;
	I2C_Recover & recovery() {return *_recovery;}
	int32_t setI2Cfreq_retainAutoSpeed(int32_t i2cFreq);
protected:
	virtual unsigned long getFailedTime(int16_t devAddr) { return 0; }
	virtual void setFailedTime(int16_t devAddr) {}

private:
	friend class I2C_Recover;
	friend class I2C_Scan;
	uint8_t check_endTransmissionOK(int addr);
	void waitForZeroCross();
	//void callTime_OutFn(int addr);
	void wireBegin() { wire_port.begin(_myAddress); }
	uint8_t beginTransmission(uint16_t deviceAddr); // return false to inhibit access
	void waitForDeviceReady(uint16_t deviceAddr);
	uint8_t getData(uint16_t deviceAddr, uint16_t numberBytes, uint8_t *dataBuffer);
	uint8_t getTWIbufferSize();
	TwoWire & wire_port;
	//uint8_t noOfRetries;
	bool _waitForZeroCross = false;

	//I_I2CresetFunctor * timeoutFunctor;
	//
	//class I2Creset_Functor : public I_I2CresetFunctor {
	//public:
	//	void set(TestFnPtr tfn) {_tfn = tfn;}
	//	uint8_t operator()(I2C_Talk & i2c, int addr) /*override*/ {return _tfn(i2c,addr);}
	//private:
	//	TestFnPtr _tfn;
	//};
	//I2Creset_Functor _i2CresetFunctor; // data member functor to wrap free reset function

	uint32_t relayDelay;
	uint32_t relayStart;
	static int8_t s_zeroCrossPin;
	static uint16_t s_zxSigToXdelay;
	static int8_t TWI_BUFFER_SIZE;
	int32_t _i2cFreq;
	uint8_t _myAddress;
	bool _canWrite;
	unsigned long _lastWrite = 0;
	I2C_Recover * _recovery;
	I2C_Recover _nullRecover;
};

class I2C_Talk_Auto_Speed_Hoist : public I2C_Talk {
public:
	I2C_Talk_Auto_Speed_Hoist(TwoWire &wire_port = Wire, int32_t i2cFreq = 100000) : I2C_Talk(wire_port, i2cFreq){}
	I2C_Talk_Auto_Speed_Hoist(int multiMaster_MyAddress, TwoWire &wire_port = Wire, int32_t i2cFreq = 100000) : I2C_Talk(multiMaster_MyAddress, wire_port, i2cFreq){}
	//I2C_Helper_Auto_Speed_Hoist(TwoWire &wire_port, signed char zxPin, uint8_t retries, uint16_t zxDelay, I_I2CresetFunctor * timeoutFunction, int32_t i2cFreq = 100000) : I2C_Talk(wire_port, zxPin, retries, zxDelay, timeoutFunction, i2cFreq){}
	I2C_Talk_Auto_Speed_Hoist(TwoWire &wire_port, signed char zxPin, uint16_t zxDelay, I2C_Recover * recovery = 0, int32_t i2cFreq = 100000) : I2C_Talk(wire_port, zxPin, zxDelay, recovery, i2cFreq){}
	void useAutoSpeed(bool set = true) override { _useAutoSpeed = set; }
	bool usingAutoSpeed() const override { return _useAutoSpeed; }

protected:
	int32_t _getI2CFrequency(int16_t devAddr, int8_t * devAddrArr, const int32_t * i2c_speedArr, int noOfDevices);
	unsigned long _getFailedTime(int16_t devAddr, int8_t * devAddrArr, const unsigned long * failedTimeArr, int noOfDevices);
	int32_t _setI2CFrequency(int16_t devAddr, int32_t i2cFreq, int8_t * devAddrArr, int32_t * i2c_speedArr, int noOfDevices);
	void _setFailedTime(int16_t devAddr, int8_t * devAddrArr, unsigned long * failedTimeArr, int noOfDevices);
private:
	int _findDevice(int16_t devAddr, int8_t * devAddrArr, int noOfDevices);
	bool _useAutoSpeed = false;
};

template<int noOfDevices>
class I2C_Talk_Auto_Speed : public I2C_Talk_Auto_Speed_Hoist {
public:
	I2C_Talk_Auto_Speed(TwoWire & wire_port = Wire, int32_t i2cFreq = 100000) : I2C_Talk_Auto_Speed_Hoist(wire_port, i2cFreq){resetAddresses();}
	I2C_Talk_Auto_Speed(int multiMaster_MyAddress, TwoWire & wire_port = Wire, int32_t i2cFreq = 100000): I2C_Talk_Auto_Speed_Hoist(multiMaster_MyAddress, wire_port, i2cFreq){resetAddresses();}
	//I2C_Talk_Auto_Speed(TwoWire & wire_port, signed char zxPin, uint8_t retries, uint16_t zxDelay, I_I2CresetFunctor * timeoutFunction, int32_t i2cFreq = 100000): I2C_Helper_Auto_Speed_Hoist(wire_port, zxPin, retries, zxDelay, timeoutFunction, i2cFreq){resetAddresses();}
	I2C_Talk_Auto_Speed(TwoWire & wire_port, signed char zxPin, uint16_t zxDelay, I2C_Recover * recovery = 0, int32_t i2cFreq = 100000): I2C_Talk_Auto_Speed_Hoist(wire_port, zxPin, zxDelay, recovery, i2cFreq){resetAddresses();}
	
	int8_t getAddress(int index) const { return devAddrArr[index]; }
	unsigned long getFailedTime(int16_t devAddr) override { return _getFailedTime(devAddr, devAddrArr, lastFailedTimeArr, noOfDevices);}
	int32_t getThisI2CFrequency(int16_t devAddr) override {
		if ( usingAutoSpeed()) return _getI2CFrequency(devAddr, devAddrArr, i2c_speedArr, noOfDevices); 
		else return getI2CFrequency();
	}
	int32_t setThisI2CFrequency(int16_t devAddr, int32_t i2cFreq) override {return _setI2CFrequency(devAddr, i2cFreq, devAddrArr, i2c_speedArr, noOfDevices);}
	void setFailedTime(int16_t devAddr) override { _setFailedTime(devAddr, devAddrArr, lastFailedTimeArr, noOfDevices);}
private:
	void resetAddresses();
	int8_t devAddrArr[noOfDevices];
	int32_t i2c_speedArr[noOfDevices];
	unsigned long lastFailedTimeArr[noOfDevices];
};

template<int noOfDevices>
void I2C_Talk_Auto_Speed<noOfDevices>::resetAddresses() {
	for (int i=0; i< noOfDevices; ++i) {
		devAddrArr[i] = 0;
		i2c_speedArr[i] = 400000;
		lastFailedTimeArr[i] = millis();
	}
}
