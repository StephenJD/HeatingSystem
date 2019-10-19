#pragma once

#include <Arduino.h>
#include <inttypes.h>
#include <Wire.h>
#include <I2C_Talk_ErrorCodes.h>

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
namespace I2C_Recovery {
	class I2C_Recover;
}

class I2C_Talk {
public:
	//class I_I2CresetFunctor {
	//public:
	//	virtual uint8_t operator()(I2C_Talk & i2c, int addr) = 0;
	//};

	//typedef uint8_t (*TestFnPtr)(I2C_Talk &, int);
	
	// Basic Usage //
	enum {_single_master = 255, _no_address = 255};

	I2C_Talk(TwoWire & wire_port = Wire, int32_t i2cFreq = 100000) : I2C_Talk(_single_master, wire_port, i2cFreq) {} // cannot be constexpr as wire() is not constexpr
	
	auto read(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) -> I2C_Talk_ErrorCodes::error_codes; // dataBuffer may not be written to if read fails.
	auto read(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, char *dataBuffer) -> I2C_Talk_ErrorCodes::error_codes {return read(deviceAddr, registerAddress, numberBytes, (uint8_t *) dataBuffer);}
	auto readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer)-> I2C_Talk_ErrorCodes::error_codes ; // dataBuffer may not be written to if read fails.
	auto readEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, char *dataBuffer)-> I2C_Talk_ErrorCodes::error_codes  { return readEP(deviceAddr, pageAddress, numberBytes, (uint8_t *)dataBuffer); }
	auto write(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes;
	auto write(uint16_t deviceAddr, uint8_t registerAddress, uint8_t data)-> I2C_Talk_ErrorCodes::error_codes { return write(deviceAddr, registerAddress, 1, &data); }
	
	auto write_verify(uint16_t deviceAddr, uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes;
	auto writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::error_codes;
	auto writeEP(uint16_t deviceAddr, int pageAddress, uint8_t data) ->I2C_Talk_ErrorCodes::error_codes { return writeEP(deviceAddr, pageAddress, 1, &data); } // Writes 32-byte pages. #define I2C_EEPROM_PAGESIZE
	auto writeEP(uint16_t deviceAddr, int pageAddress, uint16_t numberBytes, char *dataBuffer)->I2C_Talk_ErrorCodes::error_codes {return writeEP(deviceAddr, pageAddress, numberBytes, (const uint8_t *)dataBuffer); }
	// Slave response
	auto write(const uint8_t *dataBuffer, uint16_t numberBytes)->I2C_Talk_ErrorCodes::error_codes; // Called by slave in response to request from a Master. 
	auto write(const char *dataBuffer)->I2C_Talk_ErrorCodes::error_codes {return write((const uint8_t*)dataBuffer, (uint8_t)strlen(dataBuffer)+1);}// Called by slave in response to request from a Master. 
	
	auto status(int deviceAddr)->I2C_Talk_ErrorCodes::error_codes;
	
	int32_t setI2CFrequency(int32_t i2cFreq);
	int32_t getI2CFrequency() const { return _i2cFreq; }
	virtual void writeInSync() {}

	bool restart();

	static const char * getStatusMsg(int errorCode);
	
	// Slave Usage //
	// Where there is a single master, you can set the slave arduino using setAsSlave(), which prohibits it from initiating a write.
	// Where multiple arduinos need to be able to initiate writes (even if they are not talking to each other),
	// each must use the setAsMaster() function, providing a unique I2C address.
	// Multi_master mode enables each arduino to act as a master and also respond as a slave.
	// To switch off multi-master mode, call setAsMaster() without supplying an address.
	// To resond as a slave, the onReceive() and onRequest() functions must be set.
	// *** The onReceive() function must not do a Serial.print when it receives a register address prior to a read.
	I2C_Talk(int multiMaster_MyAddress, TwoWire & wire_port = Wire, int32_t i2cFreq = 100000);
	void setAsSlave(int slaveAddress) { _myAddress = slaveAddress; _isMaster = false; restart(); }
	void setAsMaster(int multiMaster_MyAddress = _single_master) {_myAddress = multiMaster_MyAddress; _isMaster = true; restart();}
	void onReceive(void(*fn)(int)) { _wire_port.onReceive(fn); } // The supplied function is called when data is sent to a slave
	void onRequest(void(*fn)(void)){ _wire_port.onRequest(fn); } // The supplied function is called when data is requested from a slave
	uint8_t receiveFromMaster(int howMany, uint8_t *dataBuffer); // Data is written to dataBuffer. Returns how many are written.
	uint8_t receiveFromMaster(int howMany, char *dataBuffer) {return receiveFromMaster(howMany, (uint8_t *) dataBuffer);}
	bool isMaster() const { return _isMaster; }
	void waitForEPready(uint16_t deviceAddr);
	// required by template, may as well be publicly available
	static const int32_t MAX_I2C_FREQ = (VARIANT_MCK / 40) > 400000 ? 400000 : (VARIANT_MCK / 40); //100000; // 
	static const int32_t MIN_I2C_FREQ = VARIANT_MCK / 65288 * 2; //32644; //VARIANT_MCK / 65288; //36000; //
	static auto addressOutOfRange(int addr)->I2C_Talk_ErrorCodes::error_codes { return (addr > 127) || (addr < 0) ? I2C_Talk_ErrorCodes::_I2C_AddressOutOfRange : I2C_Talk_ErrorCodes::_OK; }

private:
	static int8_t TWI_BUFFER_SIZE;
	friend class I2C_Recovery::I2C_Recover;
	friend class I_I2C_Scan;

	void wireBegin() { _wire_port.begin(_myAddress); }
	auto beginTransmission(uint16_t deviceAddr) ->I2C_Talk_ErrorCodes::error_codes; // return false to inhibit access
	auto getData(uint16_t deviceAddr, uint16_t numberBytes, uint8_t *dataBuffer) -> I2C_Talk_ErrorCodes::error_codes;
	uint8_t getTWIbufferSize();
	auto endTransmission()->I2C_Talk_ErrorCodes::error_codes;
	virtual void setProcessTime() {}
	virtual void synchroniseWrite() {}

	uint32_t _lastWrite = 0;
	int32_t _i2cFreq = 100000;
	TwoWire & _wire_port;
	bool _isMaster = true;
	uint8_t _myAddress = _single_master;
};

class I2C_Talk_ZX : public I2C_Talk {
public:
	using I2C_Talk::I2C_Talk;
	void setZeroCross(int8_t zxPin) { s_zeroCrossPin = zxPin; } // Arduino pin signalling zero-cross detected. 0 = disable, +ve = signal on rising edge, -ve = signal on falling edge
	void setZeroCrossDelay(uint16_t zxDelay) { s_zxSigToXdelay = zxDelay; } // microseconds delay between signal on zero_cross_pin to next true zero-cross.

	void writeInSync() override { _waitForZeroCross = true; }

private:
	void synchroniseWrite() override;
	void setProcessTime() override;

	uint32_t relayDelay = 0;
	uint32_t relayStart = 0;
	uint16_t s_zxSigToXdelay = 700; // microseconds
	bool _waitForZeroCross = false;
	int8_t s_zeroCrossPin = 0;
};