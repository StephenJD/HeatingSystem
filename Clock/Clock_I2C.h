#pragma once

#define _REQUIRES_CLOCK_
#include <Clock_.h>
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Talk_ErrorCodes.h>

class I_Clock_I2C : public Clock {
public:
	auto saveTime()->uint8_t override;
protected:
	auto loadTime()->uint8_t override;

private:
	void _update() override { loadTime(); }	// called every 10 minutes - reads from RTC
	virtual auto readData(int start, int numberBytes, uint8_t* dataBuffer)->I2C_Talk_ErrorCodes::error_codes = 0;
	virtual auto writeData(int start, int numberBytes, const uint8_t* dataBuffer)->I2C_Talk_ErrorCodes::error_codes = 0;
	auto _timeFromRTC(int& minUnits, int& seconds)->Date_Time::DateTime;
};

template<I2C_Talk& i2c>
class Clock_I2C : public I_Clock_I2C, public I2Cdevice<i2c> {
public:
	using I2Cdevice<i2c>::I2Cdevice;

	Clock_I2C(int addr) : I2Cdevice<i2c>(addr) {
		// Rated at 5v/100kHz - NO 3.3v operation, min 2.2v HI.
		i2c.setMax_i2cFreq(100000); // Max freq supported by DS1307
		i2c.extendTimeouts(5000, 5, 1000); // process time to read clock. Potential freeze if Wire not constructed
		Clock::_lastCheck_mS = millis() - (60ul * 1000ul * 11ul);
		//Serial.println(F("Clock_I2C Constructor"));
	}

	bool ok() const override {
		for (uint32_t t = millis() + 5; millis() < t; ) {
			if (i2c.status(I2Cdevice<i2c>::getAddress()) == I2C_Talk_ErrorCodes::_OK) return true;
		}
		return false;
	}
	auto testDevice()->I2C_Talk_ErrorCodes::error_codes override;

private:
	auto readData(int start, int numberBytes, uint8_t* dataBuffer)->I2C_Talk_ErrorCodes::error_codes override {
		//if (I2Cdevice<i2c>::getStatus() != I2C_Talk_ErrorCodes::_OK) logger() << "RTC Unavailable" << L_endl;
		return I2Cdevice<i2c>::read(start, numberBytes, dataBuffer);
	}
	auto writeData(int start, int numberBytes, const uint8_t* dataBuffer)->I2C_Talk_ErrorCodes::error_codes override { return I2Cdevice<i2c>::write_verify(start, numberBytes, dataBuffer); }
};

template<I2C_Talk& i2C>
auto Clock_I2C<i2C>::testDevice() -> I2C_Talk_ErrorCodes::error_codes {
	//Serial.print(" RTC testDevice at "); Serial.println(i2c.getI2CFrequency(),DEC);
	uint8_t data[1] = { 0 };
	auto errCode = I2Cdevice<i2C>::write_verify(9, 1, data);
	data[0] = 255;
	if (errCode != I2C_Talk_ErrorCodes::_OK) errCode = I2Cdevice<i2C>::write_verify(9, 1, data);
	return errCode;
}