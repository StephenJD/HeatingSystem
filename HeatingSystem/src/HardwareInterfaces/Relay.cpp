#include "Relay.h"
#include "A__Constants.h"
//#include "I2C_Helper.h"

namespace HardwareInterfaces {

	/////////////////////////////////////
	//       RelaysPort Functions      //
	/////////////////////////////////////

	void RelaysPort::setup(I2C_Helper & i2C, int i2cAddress, int zeroCrossPin, int resetPin)
		{
		Serial.println("RelaysPort::setup()");
		I2C_Helper::I_I2Cdevice::setI2Chelper(i2C);
		setAddress(i2cAddress);
		_zeroCrossPin = zeroCrossPin;
		_resetPin = resetPin;
		pinMode(abs(_zeroCrossPin), INPUT);
		digitalWrite(abs(_zeroCrossPin), HIGH); // turn on pullup resistors
		pinMode(abs(_resetPin), OUTPUT);
		digitalWrite(abs(_resetPin), (_resetPin < 0) ? HIGH : LOW);
	}

	uint8_t RelaysPort::initialiseDevice() {
		uint8_t dataBuffa[1];
		uint8_t hasFailed = _i2C->write_at_zero_cross(_address, REG_8PORT_OLAT, _relayRegister); // set latches
		hasFailed |= _i2C->write(_address, REG_8PORT_PullUp, 0x00); // clear all pull-up resistors
		hasFailed |= _i2C->write_at_zero_cross(_address, REG_8PORT_IODIR, 0x00); // set all as outputs

		hasFailed |= _i2C->read(_address, REG_8PORT_PullUp, 1, dataBuffa);
		if (!hasFailed) hasFailed = (dataBuffa[0] == 0 ? I2C_Helper::_OK : I2C_Helper::_I2C_ReadDataWrong);
		hasFailed |= _i2C->read(_address, REG_8PORT_IODIR, 1, dataBuffa);
		if (!hasFailed) hasFailed = (dataBuffa[0] == 0 ? I2C_Helper::_OK : I2C_Helper::_I2C_ReadDataWrong);
		Serial.println("RelaysPort::initialiseDevice()");
		return hasFailed;
	}

	uint8_t RelaysPort::setAndTestRegister() {
		uint8_t gotData, ANDmask = 0xFF;
		if (_i2C == 0) _error = I2C_Helper::_I2C_not_created;
		else if (_i2C->notExists(_address)) _error = I2C_Helper::_I2C_Device_Not_Found;
		else {
			_error = _i2C->write_at_zero_cross(_address, REG_8PORT_OLAT, _relayRegister);
			if (_error == I2C_Helper::_OK) {
				_error = _i2C->read(_address, REG_8PORT_OLAT, 1, &gotData);
				if ((_error == I2C_Helper::_OK) && ((gotData & ANDmask) != (_relayRegister & ANDmask))) {
					_error = I2C_Helper::_I2C_ReadDataWrong;
				}
			}
		}
		return _error;
	}

	/////////////////////////////////////
	//       Relay Functions      //
	/////////////////////////////////////

	Relay * relays;

	uint8_t * Relay::_relayRegister;

	bool Relay::getRelayState() const {
		uint8_t myBit = *_relayRegister & (1 << port());
		return !(myBit^activeState()); // Relay state
	}

	bool Relay::setRelay(uint8_t state) { // returns true if state is changed
		uint8_t myBitMask = 1 << port();
		uint8_t myBit = (*_relayRegister  & myBitMask) != 0;
		uint8_t currState = !(myBit^activeState());

		myBit = !(state^activeState()); // Required bit state 
		if (!myBit) { // clear bit
			*_relayRegister &= ~myBitMask;
		}
		else { // set this bit
			*_relayRegister |= myBitMask;
		}
		return currState != state; // returns true if state is changed
	}
}