#include "Relay.h"
#include "A__Constants.h"
#include <Logging.h>

namespace HardwareInterfaces {
	using namespace I2C_Recovery;
	using namespace I2C_Talk_ErrorCodes;
	/////////////////////////////////////
	//       RelaysPort Functions      //
	/////////////////////////////////////
	uint8_t RelaysPort::relayRegister = 0xFF;

	RelaysPort::RelaysPort(I2C_Recover & recovery, int addr, int zeroCrossPin, int resetPin)
		: I_I2Cdevice_Recovery{ recovery, addr }
		, _zeroCrossPin( zeroCrossPin )
		, _resetPin( resetPin )
	{
		pinMode(abs(_zeroCrossPin), INPUT);
		logger().log("RelaysPort::setup()");
		digitalWrite(abs(_zeroCrossPin), HIGH); // turn on pullup resistors
		pinMode(abs(_resetPin), OUTPUT);
		digitalWrite(abs(_resetPin), (_resetPin < 0) ? HIGH : LOW);
	}

	error_codes RelaysPort::initialiseDevice() {
		uint8_t pullUp_out[] = { 0 };
		auto status = _OK;

		status = write_verify(REG_8PORT_PullUp, 1, pullUp_out); // clear all pull-up resistors
		if (status) {
			logger().log("Initialise RelaysPort() write-verify failed at Freq:", i2C().getI2CFrequency());
		}
		else {
			status = write_verify(REG_8PORT_OLAT, 1, &relayRegister); // set latches
			writeInSync();
			status |= write_verify(REG_8PORT_IODIR, 1, pullUp_out); // set all as outputs
			if (status) logger().log("Initialise RelaysPort() lat-write failed at Freq:", i2C().getI2CFrequency());
			//else logger().log("Initialise RelaysPort() succeeded at Freq:", i2C().getI2CFrequency());
		}
		return status;
	}

	error_codes RelaysPort::setAndTestRegister() {
		uint8_t ANDmask = 0x7F;
		//if (i2C().status(_address)) error = I2C_Talk::_I2C_Device_Not_Found;
		//else {
			writeInSync();
			auto status = write_verify(REG_8PORT_OLAT & ANDmask, 1, &relayRegister);
		//}
		//logger().log("RelaysPort::setAndTestRegister() addr:", _address,i2C().getStatusMsg(error));
		return status;
	}

	/////////////////////////////////////
	//       Relay Functions      //
	/////////////////////////////////////

	bool Relay::getRelayState() const {
		uint8_t myBit = RelaysPort::relayRegister & (1 << port());
		return !(myBit^activeState()); // Relay state
	}

	bool Relay::setRelay(uint8_t state) { // returns true if state is changed
		uint8_t myBitMask = 1 << port();
		uint8_t myBit = (RelaysPort::relayRegister  & myBitMask) != 0;
		uint8_t currState = !(myBit^activeState());

		myBit = !(state^activeState()); // Required bit state 
		if (!myBit) { // clear bit
			RelaysPort::relayRegister &= ~myBitMask;
		}
		else { // set this bit
			RelaysPort::relayRegister |= myBitMask;
		}
		return currState != state; // returns true if state is changed
	}
}