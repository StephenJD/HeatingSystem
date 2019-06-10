#pragma once
#include <Arduino.h>
class I2C_Talk;
class TwoWire;
class I_I2Cdevice;

class I2C_Recover {
public:
	I2C_Recover(I2C_Talk & i2C) : _i2C(&i2C) {}
	void registerDevice(I_I2Cdevice & device) { _device = &device; }
	I_I2Cdevice & device() { return *_device; }
	const I_I2Cdevice & device() const { return *_device; }
	// Polymorphic Functions for TryAgain
	virtual void newReadWrite() {}
	virtual bool tryReadWriteAgain(uint8_t status) { return false; }
	virtual void endReadWrite() {}
	// Polymorphic Functions for I2C_Talk
	virtual uint8_t findAworkingSpeed() { return testDevice(1,0); }
	virtual uint8_t testDevice(int noOfTests, int allowableFailures);
	I2C_Talk & i2C() const { return *_i2C; }
protected:
	//I2C_Recover() = default;
	virtual void set_I2C_Talk(I2C_Talk & i2C) { _i2C = &i2C; }
	virtual void ensureNotFrozen() {}

	// Non-Polymorphic Queries for I2C_Recover
	TwoWire & wirePort() const;
	void wireBegin() const;
private:
	friend class I2C_Talk;
	I2C_Talk * _i2C = 0;
	I_I2Cdevice * _device = 0;
};

class TryAgain {
public:
	TryAgain(I2C_Recover & recovery, I_I2Cdevice & device) :
		_recovery(&recovery) 
	{
		_recovery->registerDevice(device);
		_recovery->newReadWrite(); 
	}

	bool operator()(uint8_t status) const { return _recovery->tryReadWriteAgain(status); }

	~TryAgain() { _recovery->endReadWrite(); }
private:
	I2C_Recover * _recovery;
};