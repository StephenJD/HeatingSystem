#pragma once
#include <Arduino.h>
class I2C_Talk;
class TwoWire;
class I_I2Cdevice;

class I2C_Recover {
public:
	I2C_Recover(I2C_Talk & i2C) : _i2C(&i2C) {}
	// Polymorphic Functions for TryAgain
	virtual void newReadWrite() {}
	virtual bool tryReadWriteAgain(uint8_t status) { return false; }
	virtual void endReadWrite() {}
	// Polymorphic Functions for I2C_Talk
	virtual bool checkForTimeout(uint8_t status) { return false; }
	virtual uint8_t speedOK() { return 0; }
	virtual bool endTransmissionError(uint8_t status) { return false; }
	virtual void callTime_OutFn() {}
	virtual signed char findAworkingSpeed(I_I2Cdevice * deviceFailTest = 0) { return testDevice(deviceFailTest); }
	void setAddress(uint16_t deviceAddr) { _deviceAddr = deviceAddr; }
protected:
	virtual uint8_t testDevice(I_I2Cdevice * deviceFailTest);
	// Non-Polymorphic Queries for I2C_Recover
	I2C_Talk & i2C() const { return *_i2C; }
	unsigned long getFailedTime() const;
	void setFailedTime() const;
	TwoWire & wirePort() const;
	void wireBegin() const;
	uint16_t addr() { return _deviceAddr; }
private:
	I2C_Talk * _i2C;
	uint16_t _deviceAddr;
};

class TryAgain {
public:
	TryAgain(I2C_Recover * recovery, uint16_t deviceAddr) : 
		_recovery(recovery) 
	{
		_recovery->setAddress(deviceAddr);
		_recovery->newReadWrite(); 
	}

	bool operator()(uint8_t status) const { return _recovery->tryReadWriteAgain(status); }

	~TryAgain() { _recovery->endReadWrite(); }
private:
	I2C_Recover * _recovery;
};