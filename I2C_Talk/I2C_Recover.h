#pragma once
#include <Arduino.h>
class I2C_Talk;
class TwoWire;

class I2C_Recover {
public:
	I2C_Recover(I2C_Talk & i2C) : _i2C(&i2C) {}	
	// Polymorphic Functions for TryAgain
	virtual void newReadWrite() {}
	virtual bool tryReadWriteAgain(uint8_t status, uint16_t deviceAddr) { return false; }
	virtual void endReadWrite() {}
	// Polymorphic Functions for I2C_Talk
	virtual bool checkForTimeout(uint8_t status, uint16_t deviceAddr) { return false; }
	virtual uint8_t speedOK(uint16_t deviceAddr) { return 0; }
	virtual bool endTransmissionError(uint8_t status, uint16_t deviceAddr) { return false; }

protected:
	// Non-Polymorphic Queries for I2C_Recover
	I2C_Talk & i2C() const { return *_i2C; }
	unsigned long getFailedTime(int16_t devAddr) const;
	void setFailedTime(int16_t devAddr) const;
	TwoWire & wirePort() const;
	void wireBegin() const;
	
private:
	I2C_Talk * _i2C;
};

class TryAgain {
public:
	TryAgain(I2C_Recover * recovery, uint16_t deviceAddr) : 
		_recovery(recovery) 
		, _deviceAddr(deviceAddr)
	{ _recovery->newReadWrite(); }

	bool operator()(uint8_t status) const { return _recovery->tryReadWriteAgain(status,_deviceAddr); }

	~TryAgain() { _recovery->endReadWrite(); }
private:
	I2C_Recover * _recovery;
	uint16_t _deviceAddr;
};