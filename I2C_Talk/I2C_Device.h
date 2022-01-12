#pragma once
#include <I2C_Talk.h>
#include <I2C_Talk_ErrorCodes.h>

/// <summary>
/// Non-Recovery read/write functions
/// </summary>
class I_I2Cdevice {
public:
	static constexpr auto START_SPEED_AFTER_FAILURE = 100000;
	// Queries
	virtual int32_t runSpeed() const { return i2C().getI2CFrequency(); }
	virtual int32_t getFailedTime() const { return 0; }
	virtual bool isEnabled() const { return true; }
	virtual auto getStatus() const-> I2C_Talk_ErrorCodes::Error_codes { return i2C().status(getAddress()); }
	const I2C_Talk& i2C() const { return const_cast<I_I2Cdevice*>(this)->i2C(); }
	uint8_t getAddress() const { return _address; }

	// Modifiers
	virtual I2C_Talk& i2C() = 0;
	virtual auto initialiseDevice()-> I2C_Talk_ErrorCodes::Error_codes {return I2C_Talk_ErrorCodes::_OK; }
	virtual auto testDevice()-> I2C_Talk_ErrorCodes::Error_codes {
		// NOTE: derived implementations should call base-class write/read functions to avoid recovery strategies being applied during tests.
		//Serial.print(" I_I2Cdevice.testDevice_read at: 0x"); Serial.println(getAddress(), HEX);
		uint8_t readBuffer;
		return I_I2Cdevice::read(0, 1, &readBuffer);
		//return i2C().status(getAddress()); 
	}
	virtual int32_t set_runSpeed(int32_t i2cFreq) { return i2C().setI2CFrequency(i2cFreq); }
	virtual void disable() {};
	virtual auto reEnable(bool immediatly = false)->I2C_Talk_ErrorCodes::Error_codes { return I2C_Talk_ErrorCodes::_OK; }
	virtual void reset() {};

	virtual auto read(int registerAddress, int numberBytes, uint8_t *dataBuffer) -> I2C_Talk_ErrorCodes::Error_codes {
		return i2C().read(getAddress(), registerAddress, numberBytes, dataBuffer); 
	} // Return errCode. dataBuffer may not be written to if read fails.
	virtual auto readEP(int pageAddress, int numberBytes, uint8_t *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return i2C().readEP(getAddress(), pageAddress, numberBytes, dataBuffer); }  // Return errCode. dataBuffer may not be written to if read fails.
	virtual auto write(int registerAddress, int numberBytes, const uint8_t *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return i2C().write(getAddress(), registerAddress, numberBytes, dataBuffer); }  // Return errCode.
	virtual auto writeEP(int pageAddress, int numberBytes, const uint8_t *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return i2C().writeEP(getAddress(), pageAddress, numberBytes, dataBuffer); } // Return errCode.
	virtual auto write_verify(int registerAddress, int numberBytes, const uint8_t *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return i2C().write_verify(getAddress(), registerAddress, numberBytes, dataBuffer); } // Return errCode.
	
	void setAddress(uint8_t addr) {
		_address = addr;
	}
	/// <summary>
	/// Non-Recovery
	/// Returns data in Native-endianness, i.e. for Arduino LSB at lowest address.
	/// Mask applied to device-byte order.
	/// </summary>
	auto read_verify_2bytes(int registerAddress, int16_t & dataBuffer, int requiredConsecutiveReads, int maxNoOfTries, uint16_t dataMask = 0xFFFF)->I2C_Talk_ErrorCodes::Error_codes;  // Non-Recovery

	/// <summary>
	/// Non-Recovery
	/// </summary>
	auto read_verify_1byte(int registerAddress, uint8_t & dataBuffer, int requiredConsecutiveReads, int maxNoOfTries, uint8_t dataMask = 0xFF)->I2C_Talk_ErrorCodes::Error_codes;  // Non-Recovery

	/// <summary>
	/// Non-Recovery
	/// </summary>
	auto read_verify(int registerAddress, int numberBytes, uint8_t* dataBuffer, int requiredConsecutiveReads, int maxNoOfTries)->I2C_Talk_ErrorCodes::Error_codes;  // Non-Recovery
	
	// delegating functions
	void writeInSync() { i2C().writeInSync(); }

	auto read(int registerAddress, int numberBytes, char *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return read(registerAddress, numberBytes, (uint8_t *)dataBuffer); }
	auto readEP(int pageAddress, int numberBytes, char *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return readEP(pageAddress, numberBytes, (uint8_t *)dataBuffer); }
	auto write(int registerAddress, uint8_t data)-> I2C_Talk_ErrorCodes::Error_codes { return write(registerAddress, 1, &data); } // Return errCode.
	auto writeEP(int pageAddress, uint8_t data)-> I2C_Talk_ErrorCodes::Error_codes { return writeEP(pageAddress, 1, &data); } // Return errCode. Writes 32-byte pages. #define I2C_EEPROM_PAGESIZE
	auto writeEP(int pageAddress, int numberBytes, const char *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return writeEP(pageAddress, numberBytes, (const uint8_t *)dataBuffer); }
	
	// slave response
	auto write(const uint8_t *dataBuffer, int numberBytes)-> I2C_Talk_ErrorCodes::Error_codes { return i2C().write(dataBuffer, numberBytes); }// Called by slave in response to request from a Master. Return errCode.
	auto write(const char *dataBuffer)-> I2C_Talk_ErrorCodes::Error_codes { return write((const uint8_t*)dataBuffer, (uint8_t)strlen(dataBuffer) + 1); }// Called by slave in response to request from a Master. Return errCode.


	//static const __FlashStringHelper * getStatusMsg(uint8_t status) { return I2C_Talk::getStatusMsg(status); }
protected:
	constexpr explicit I_I2Cdevice(int addr) : _address(static_cast<uint8_t>(addr)) {}
	I_I2Cdevice() = default; 
private:
	uint8_t _address = 0;
};

/// <summary>
/// Non-Recovery read/write functions
/// </summary>
template <I2C_Talk & i2c>
class I2Cdevice : public I_I2Cdevice {
public:
	using I_I2Cdevice::I_I2Cdevice;	
	using I_I2Cdevice::i2C;
	I2C_Talk & i2C() override { return i2c; }
};

/// <summary>
/// Read/Write functions retry and apply recovery strategy
/// </summary>
class I_I2Cdevice_Recovery : public I_I2Cdevice { // cannot be constexpr because of use of non-const class static in constructors
public:
	static constexpr decltype(millis()) DISABLE_PERIOD_ON_FAILURE = 60000; // 60 secs
	static constexpr auto I2C_RETRIES = 20;

	I_I2Cdevice_Recovery(I2C_Recovery::I2C_Recover & recover, int addr) : I_I2Cdevice(addr), _recover(&recover) { set_recover = _recover; 
	//logger() << F("I_I2Cdevice_Recovery done for 0x") << L_hex << addr << L_endl;
	} // initialiser for first array element 
	I_I2Cdevice_Recovery(I2C_Recovery::I2C_Recover & recover) : _recover(&recover) {set_recover = _recover;}
	I_I2Cdevice_Recovery(int addr) : I_I2Cdevice(addr), _recover(set_recover) {}; // initialiser for subsequent array elements 
	I_I2Cdevice_Recovery() : I_I2Cdevice(), _recover(set_recover) {}; // initialiser for subsequent array elements 
	
	// Queries
	bool isEnabled() const override { return _i2c_speed != 0; }
	int32_t runSpeed() const override { return _i2c_speed; }
	int32_t getFailedTime() const override { return _lastFailedTime; }
	bool isUnrecoverable() const;
	I2C_Recovery::I2C_Recover & recovery() const { return *_recover; }
	auto getStatus() const ->I2C_Talk_ErrorCodes::Error_codes override;
	// Modifiers
	void disable() override { _lastFailedTime = millis(); _i2c_speed = 0; }
	auto reEnable(bool immediatly = false)->I2C_Talk_ErrorCodes::Error_codes override;
	void reset() override { _i2c_speed = START_SPEED_AFTER_FAILURE;	}

	using I_I2Cdevice::write;
	using  I_I2Cdevice::read;
	auto read(int registerAddress, int numberBytes, uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes override; // Return errCode. dataBuffer may not be written to if read fails.
	auto write(int registerAddress, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes override;  // Return errCode.
	/// <summary>
	/// Recovery
	/// </summary>
	auto write_verify(int registerAddress, int numberBytes, const uint8_t *dataBuffer)->I2C_Talk_ErrorCodes::Error_codes override;

	using I_I2Cdevice::i2C;
	I2C_Talk & i2C() override;

	void setRecovery(I2C_Recovery::I2C_Recover & recovery) { _recover = &recovery; }
	int32_t set_runSpeed(int32_t i2cFreq) override;

	static I2C_Recovery::I2C_Recover * set_recover;
private:
	int32_t _i2c_speed = START_SPEED_AFTER_FAILURE;
	unsigned long _lastFailedTime = 0;
	I2C_Recovery::I2C_Recover * _recover = 0;
};