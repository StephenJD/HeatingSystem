#pragma once
#include <I2C_Talk.h>

class I_I2Cdevice {
public:
	I_I2Cdevice(I2C_Talk & i2C, uint8_t addr) : _address(addr) { _i2C = &i2C; }

	virtual uint8_t initialiseDevice() {return I2C_Talk_ErrorCodes::_OK; }
	virtual uint8_t testDevice() { return i2c_Talk().notExists(getAddress()); }
	virtual int32_t set_runSpeed(int32_t i2cFreq) { return i2cFreq; }
	virtual int32_t runSpeed() const { return i2c_Talk().getI2CFrequency(); }
	uint8_t getAddress() const { return _address; }
	void setAddress(uint8_t addr);
	virtual uint8_t getStatus() const {return I2C_Talk_ErrorCodes::_OK;}
	virtual int32_t getFailedTime() const { return 0; }
	virtual void setFailedTime() {}

	virtual uint8_t read(uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) { return i2c_Talk().read(getAddress(), registerAddress, numberBytes, dataBuffer); } // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t read(uint8_t registerAddress, uint16_t numberBytes, char *dataBuffer) { return read(registerAddress, numberBytes, (uint8_t *)dataBuffer); }
	uint8_t readEP(int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer) { return i2c_Talk().readEP(getAddress(), pageAddress, numberBytes, dataBuffer); }  // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t readEP(int pageAddress, uint16_t numberBytes, char *dataBuffer) { return readEP(pageAddress, numberBytes, (uint8_t *)dataBuffer); }
	virtual uint8_t write(uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) { return i2c_Talk().write(getAddress(), registerAddress, numberBytes, dataBuffer); }  // Return errCode.
	uint8_t write(uint8_t registerAddress, uint8_t data) { return write(registerAddress, 1, &data); } // Return errCode.
	uint8_t write_verify(uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) { return i2c_Talk().write_verify(getAddress(), registerAddress, numberBytes, dataBuffer); } // Return errCode.
	uint8_t writeEP(int pageAddress, uint8_t data) { return i2c_Talk().writeEP(getAddress(), pageAddress, data); } // Return errCode. Writes 32-byte pages. #define I2C_EEPROM_PAGESIZE
	uint8_t writeEP(int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer) { return i2c_Talk().writeEP(getAddress(), pageAddress, numberBytes, dataBuffer); } // Return errCode.
	uint8_t writeEP(int pageAddress, uint16_t numberBytes, char *dataBuffer) { return writeEP(pageAddress, numberBytes, (const uint8_t *)dataBuffer); }
	void writeAtZeroCross() const { i2c_Talk().writeAtZeroCross(); }
	uint8_t notExists() const { return i2c_Talk().notExists(getAddress()); }
	// slave response
	uint8_t write(const uint8_t *dataBuffer, uint16_t numberBytes) { return i2c_Talk().write(dataBuffer, numberBytes); }// Called by slave in response to request from a Master. Return errCode.
	uint8_t write(const char *dataBuffer) { return write((const uint8_t*)dataBuffer, (uint8_t)strlen(dataBuffer) + 1); }// Called by slave in response to request from a Master. Return errCode.


	static I2C_Talk & i2c_Talk() { return *_i2C; }
	static void set_I2C_Talk(I2C_Talk & i2C) { _i2C = &i2C; }
protected:
	I_I2Cdevice() = delete; 
	I_I2Cdevice(I2C_Talk & i2C) : I_I2Cdevice(i2C, 0) {} // Use to initialise an array of devices
	static I2C_Talk * _i2C;
private:
	uint8_t _address = 0;
};

class I2Cdevice : public I_I2Cdevice {
public:
	using I_I2Cdevice::I_I2Cdevice;
	I2Cdevice(I2C_Recover & recovery, uint8_t addr) : I_I2Cdevice(recovery.i2C(), addr) { _recovery = &recovery; } // initialiser for first array element 
	I2Cdevice(I2C_Recover & recovery) : I_I2Cdevice(recovery.i2C(), 0) { _recovery = &recovery; } // initialiser for first array element 
	I2Cdevice() : I_I2Cdevice(i2c_Talk()) { } // allow array to be constructed	unsigned long getFailedTime() {return lastFailedTime;}
	uint8_t getStatus() const override {return _i2c_speed == 0 ? I2C_Talk_ErrorCodes::_disabledDevice : I2C_Talk_ErrorCodes::_OK;}
	
	uint8_t read(uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) override; // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t write(uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) override;  // Return errCode.

	int32_t runSpeed() const override {	return _i2c_speed;	}

	int32_t set_runSpeed(int32_t i2cFreq) override { return _i2c_speed = i2cFreq; }
	void setFailedTime() override { _lastFailedTime = millis(); }
	int32_t getFailedTime() const override { return _lastFailedTime; }
private:
	static I2C_Recover * _recovery;
	I2C_Recover & recovery() const { return *_recovery; }
	
	int32_t _i2c_speed = 400000;
	unsigned long _lastFailedTime = 0;
};