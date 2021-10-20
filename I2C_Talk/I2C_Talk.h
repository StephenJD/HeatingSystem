#pragma once

#include <Arduino.h>
#include <inttypes.h>
#include <Wire.h>
#include <I2C_Talk_ErrorCodes.h>
#include <PinObject.h>

/**
Using this library requires a modified wire.h, wire.cpp (AVR & Sam) and twi.h & twi.c (AVR)
NOTE: Arduino may not use the wire/twi versions in its install folder, so check...
   <Arduino_Install>\hardware\arduino\avr\libraries\Wire\src\   
   C:\Users\[UserName]\AppData\Local\arduino15\packages\arduino\hardware\avr\1.8.2\libraries\Wire\src\utility
   C:\Users\[UserName]\AppData\Local\arduino15\packages\MiniCore\hardware\avr\2.0.9\libraries\Wire\src\utility
   Sam version in: in C:\Users\[UserName]\AppData\Roaming [ or Local] \Arduino15\packages\arduino\hardware\sam\1.6.4\libraries\Wire\src

Library now un-blocks bus errors.
Small mods required so that in the event of a time-out Wire.endTransmission() returns error 1
and requestFrom() returns 0.
This in turn requires small mods to SAM TWI_WaitTransferComplete(), TWI_WaitByteSent(), TWI_WaitByteReceived().
*/

#ifndef VARIANT_MCK
	#define VARIANT_MCK F_CPU
#endif

// Degugging options
//#define DEBUG_TALK
#define DEBUG_SPEED_TEST
//#define DEBUG_RECOVER
//#define SHOW_TWI_DEBUG
//#define SHOW_TWI_TIMINGS
//#define DEBUG_TRY_ALL_SPEEDS

// The DEFAULT page size for I2C EEPROM.
// I2C_EEPROM_PAGESIZE must be multiple of 2 e.g. 16, 32 or 64
// 24LC256 -> 64 bytes
#define I2C_EEPROM_PAGESIZE 32
#define I2C_WRITE_DELAY  5000

namespace I2C_Recovery {
	class I2C_Recover;
}

class I2C_Talk {
public:
	// Basic Usage //
	enum {_single_master = 255, _no_address = 255};
	static int constexpr SPEED_TEST_INITIAL_STOP_TIMEOUT = 2;
	static int constexpr WORKING_STOP_TIMEOUT = 50;

	// No point in being constexpr as that implies an immutable object for which wire_port cannot be set.
	I2C_Talk(TwoWire & wire_port = Wire, int32_t max_I2Cfreq = 400000 ) : I2C_Talk(_single_master, wire_port, max_I2Cfreq) {}
	void ini(TwoWire & wire_port = Wire, int32_t max_I2Cfreq = 400000);
	bool begin();
	
	auto read(int deviceAddr, int registerAddress, int numberBytes, uint8_t *dataBuffer) -> I2C_Talk_ErrorCodes::Error_codes; // dataBuffer may not be written to if read fails.
	auto read(int deviceAddr, int registerAddress, int numberBytes, char *dataBuffer) -> I2C_Talk_ErrorCodes::Error_codes {return read(deviceAddr, registerAddress, numberBytes, (uint8_t *) dataBuffer);}
	auto write(int deviceAddr, int registerAddress, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes;
	auto write(int deviceAddr, int registerAddress, uint8_t data)-> I2C_Talk_ErrorCodes::Error_codes { return write(deviceAddr, registerAddress, 1, &data); }
	auto write_verify(int deviceAddr, int registerAddress, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes;
	virtual void writeInSync() {}
	
	// EEPROM specialised versions
	auto readEP(int deviceAddr, int pageAddress, int numberBytes, uint8_t *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes ; // dataBuffer may not be written to if read fails.
	auto readEP(int deviceAddr, int pageAddress, int numberBytes, char *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes  { return readEP(deviceAddr, pageAddress, numberBytes, (uint8_t *)dataBuffer); }
	auto writeEP(int deviceAddr, int pageAddress, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes;
	auto writeEP(int deviceAddr, int pageAddress, uint8_t data) ->I2C_Talk_ErrorCodes::Error_codes { return writeEP(deviceAddr, pageAddress, 1, &data); } // Writes 32-byte pages. #define I2C_EEPROM_PAGESIZE
	auto writeEP(int deviceAddr, int pageAddress, int numberBytes, char *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes {return writeEP(deviceAddr, pageAddress, numberBytes, (const uint8_t *)dataBuffer); }
	
	auto status(int deviceAddr)->I2C_Talk_ErrorCodes::Error_codes;
	
	void setMax_i2cFreq(int32_t max_I2Cfreq);
	int32_t max_i2cFreq() {	return _max_i2cFreq; }
	int32_t setI2CFrequency(int32_t i2cFreq);
	int32_t getI2CFrequency() const { return _i2cFreq; }
	
	void setTimeouts(uint32_t slaveByteProcess_uS = 5000, int stopMargin_uS = WORKING_STOP_TIMEOUT, uint32_t busRelease_uS = 1000);
	void extendTimeouts(uint32_t slaveByteProcess_uS = 5000, int stopMargin_uS = 3, uint32_t busRelease_uS = 1000); // make longer but not shorter
	uint8_t stopMargin() const {return _stopMargin_uS;}
	uint32_t slaveByteProcess() const {return _slaveByteProcess_uS;}
	void setStopMargin(uint8_t margin);
	static auto getStatusMsg(int errorCode) -> const __FlashStringHelper *;
	static uint16_t fromBigEndian(const uint8_t* byteArr) { return (byteArr[0] << 8) + byteArr[1]; }
	typedef const uint8_t Bytes[2];
	struct Big {
		Bytes arr;
		Bytes& operator()() { return arr; }
	} ;

	static auto toBigEndian(const uint16_t val) -> Big { // usage: auto byteArr = toBigEndian(value)();
		Big big{ uint8_t(val >> 8), uint8_t(val) };
		return big;
	}
	// Slave Usage //
	// Where there is a single master, you can set the slave arduino using setAsSlave(), which prohibits it from initiating a write.
	// Where multiple arduinos need to be able to initiate writes (even if they are not talking to each other),
	// each must use the setAsMaster() function, providing a unique I2C address.
	// Multi_master mode enables each arduino to act as a master and also respond as a slave.
	// To switch off multi-master mode, call setAsMaster() without supplying an address.
	// To respond as a slave, the onReceive() and onRequest() functions must be set.
	// *** The onReceive() function must not do a Serial.print when it receives a register address prior to a read.
	I2C_Talk(int multiMaster_MyAddress, TwoWire & wire_port = Wire, int32_t max_I2Cfreq = 400000);
	void setAsSlave(int slaveAddress) { _myAddress = slaveAddress; _isMaster = false; begin(); }
	void setAsMaster(int multiMaster_MyAddress = _single_master) {_myAddress = multiMaster_MyAddress; _isMaster = true; begin();}
	bool isMaster() const { return _isMaster; }
	uint8_t address() const { return _myAddress; }
	// Slave response
	auto write(const uint8_t *dataBuffer, int numberBytes)->I2C_Talk_ErrorCodes::Error_codes; // Called by slave in response to request from a Master. 
	auto write(const char *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes {return write((const uint8_t*)dataBuffer, (uint8_t)strlen(dataBuffer)+1);}// Called by slave in response to request from a Master. 
	

	/// <summary>		
	/// The supplied function is called when data is sent by a Master, telling this slave how many bytes have been sent.
	/// First byte is the register address. If only regAddr sent, then Master wants to read.
	/// Any further bytes will be data to write.
	/// Unwanted data can be left unread.
	/// </summary>	
	void onReceive(void(*fn)(int howMany)) { _wire().onReceive(fn); }

	/// <summary>
	/// The supplied function is called when data is requested by a Master.
	/// The Master will send a NACK when it has received the requested number of bytes, so we should offer as many as could be requested.
	/// Arduino uses little endianness: LSB at the smallest address: So a uint16_t is [LSB, MSB].
	/// But I2C devices use big-endianness: MSB at the smallest address: So a uint16_t is [MSB, LSB].
	/// Thus a single read of a 2-byte value returns the MSB (as in a Temp Sensor); 2-byte read is required to get the LSB.
	/// So before sending the data we need to reverse the byte-order.
	/// </summary>	
	void onRequest(void(*fn)(void)){ _wire().onRequest(fn); }

	uint8_t receiveFromMaster(int howMany, uint8_t *dataBuffer); // Data is written to dataBuffer. Returns how many are written.
	uint8_t receiveFromMaster(int howMany, char *dataBuffer) {return receiveFromMaster(howMany, (uint8_t *) dataBuffer);}
	
	// required by template, may as well be publicly available
	static const int32_t MIN_I2C_FREQ = VARIANT_MCK / 65288 * 2;
	static auto addressOutOfRange(int addr)->I2C_Talk_ErrorCodes::Error_codes { return (addr > 127) || (addr < 0) ? I2C_Talk_ErrorCodes::_I2C_AddressOutOfRange : I2C_Talk_ErrorCodes::_OK; }

private:
	static int8_t TWI_BUFFER_SIZE;
	friend class I2C_Recovery::I2C_Recover;
	friend class I_I2C_Scan;
	friend class I_I2C_SpeedTest;

	auto validAddressStatus(int addr)->I2C_Talk_ErrorCodes::Error_codes {
		if (!isMaster()) return I2C_Talk_ErrorCodes::_slave_shouldnt_write; else return addressOutOfRange(addr);
	}

	TwoWire & _wire() {
		if (_wire_port == 0) Serial.println(F("_wire_port == 0"));
		return *_wire_port;
	}

	auto beginTransmission(int deviceAddr) ->I2C_Talk_ErrorCodes::Error_codes; // return false to inhibit access
	auto getData(I2C_Talk_ErrorCodes::Error_codes status, int deviceAddr, int numberBytes, uint8_t *dataBuffer) -> I2C_Talk_ErrorCodes::Error_codes;
	uint8_t getTWIbufferSize();
	auto endTransmission()->I2C_Talk_ErrorCodes::Error_codes;

	virtual void setProcessTime() {}
	virtual void synchroniseWrite() {}

	uint32_t _lastWrite = 0;
	int32_t _max_i2cFreq = (VARIANT_MCK / 36);
	int32_t _i2cFreq = 100000;
	uint32_t _slaveByteProcess_uS = 5000; // timeouts saved here, so they can be set before Wire has been initialised.
	uint32_t _busRelease_uS = 1000;
	TwoWire * _wire_port = 0;
	bool _isMaster = true;
	uint8_t _stopMargin_uS = 3;
	uint8_t _myAddress = _single_master;
};

class I2C_Talk_ZX : public I2C_Talk {
public:
	using I2C_Talk::I2C_Talk;
	void setZeroCross(HardwareInterfaces::Pin_Watch zxPinWatch) { s_zeroCrossPinWatch = zxPinWatch; } // Arduino pin signalling zero-cross detected. 0 = disable, +ve = signal on rising edge, -ve = signal on falling edge
	void setZeroCrossDelay(uint16_t zxDelay) { s_zxSigToXdelay = zxDelay; } // microseconds delay between signal on zero_cross_pin to next true zero-cross.

	void writeInSync() override { _waitForZeroCross = true; }

private:
	void synchroniseWrite() override;
	void setProcessTime() override;

	uint32_t relayStart = 0;
	uint16_t relayDelay = 0;
	uint16_t s_zxSigToXdelay = 700; // microseconds
	bool _waitForZeroCross = false;
	HardwareInterfaces::Pin_Watch s_zeroCrossPinWatch;
};