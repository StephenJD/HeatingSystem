#pragma once
#include "I2C_Talk.h"

class I_I2Cdevice {
public:
	I_I2Cdevice(I2C_Talk & i2C, uint8_t addr) : _address(addr) { _i2C = &i2C; }

	virtual uint8_t initialiseDevice() { return 0; }
	virtual uint8_t testDevice() { return 0; }
	uint8_t getAddress() const { return _address; }
	void setAddress(uint8_t addr) { _address = addr; }

	uint8_t read(uint8_t registerAddress, uint16_t numberBytes, uint8_t *dataBuffer) const { return i2c_Talk().read(getAddress(), registerAddress, numberBytes, dataBuffer); } // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t read(uint8_t registerAddress, uint16_t numberBytes, char *dataBuffer) const { return read(registerAddress, numberBytes, (uint8_t *)dataBuffer); }
	uint8_t readEP(int pageAddress, uint16_t numberBytes, uint8_t *dataBuffer) const { return i2c_Talk().readEP(getAddress(), pageAddress, numberBytes, dataBuffer); }  // Return errCode. dataBuffer may not be written to if read fails.
	uint8_t readEP(int pageAddress, uint16_t numberBytes, char *dataBuffer) const { return readEP(pageAddress, numberBytes, (uint8_t *)dataBuffer); }
	uint8_t write(uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) const { return i2c_Talk().write(getAddress(), registerAddress, numberBytes, dataBuffer); }  // Return errCode.
	uint8_t write(uint8_t registerAddress, uint8_t data) const { return i2c_Talk().write(getAddress(), registerAddress, data); } // Return errCode.
	uint8_t write_verify(uint8_t registerAddress, uint16_t numberBytes, const uint8_t *dataBuffer) const { return i2c_Talk().write_verify(getAddress(), registerAddress, numberBytes, dataBuffer); } // Return errCode.
	uint8_t writeEP(int pageAddress, uint8_t data) const { return i2c_Talk().writeEP(getAddress(), pageAddress, data); } // Return errCode. Writes 32-byte pages. #define I2C_EEPROM_PAGESIZE
	uint8_t writeEP(int pageAddress, uint16_t numberBytes, const uint8_t *dataBuffer) const { return i2c_Talk().writeEP(getAddress(), pageAddress, numberBytes, dataBuffer); } // Return errCode.
	uint8_t writeEP(int pageAddress, uint16_t numberBytes, char *dataBuffer) const { return writeEP(pageAddress, numberBytes, (const uint8_t *)dataBuffer); }
	void writeAtZeroCross() const { i2c_Talk().writeAtZeroCross(); }
	uint8_t notExists() const { return i2c_Talk().notExists(getAddress()); }
	// slave response
	uint8_t write(const uint8_t *dataBuffer, uint16_t numberBytes) { return i2c_Talk().write(dataBuffer, numberBytes); }// Called by slave in response to request from a Master. Return errCode.
	uint8_t write(const char *dataBuffer) { return write((const uint8_t*)dataBuffer, (uint8_t)strlen(dataBuffer) + 1); }// Called by slave in response to request from a Master. Return errCode.


	static I2C_Talk & i2c_Talk() { return *_i2C; }
	//static void setI2Chelper(I2C_Talk & i2C) { _i2C = &i2C; }
protected:
	I_I2Cdevice() = delete; 
	I_I2Cdevice(I2C_Talk & i2C) : I_I2Cdevice(i2C, 255) {} // Use to initialise an array of devices
private:
	//friend class I2C_Talk;
	inline static I2C_Talk * _i2C = 0;
	uint8_t _address = 255;
};