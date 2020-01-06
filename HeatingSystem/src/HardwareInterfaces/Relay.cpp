#include "Relay.h"
#include "A__Constants.h"
#include <Logging.h>
#include "I2C_RecoverRetest.h"

extern int st_index;

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
		logger() << "\nRelaysPort::setup()" << L_endl;
		digitalWrite(abs(_zeroCrossPin), HIGH); // turn on pullup resistors
		pinMode(abs(_resetPin), OUTPUT);
		digitalWrite(abs(_resetPin), (_resetPin < 0) ? HIGH : LOW);
	}

	error_codes RelaysPort::initialiseDevice() {
		uint8_t pullUp_out[] = { 0 };
		auto status = _OK;

		status = write_verify(REG_8PORT_PullUp, 1, pullUp_out); // clear all pull-up resistors
		if (status) {
			logger() << "\nInitialise RelaysPort() write-verify failed at Freq: " << i2C().getI2CFrequency() << L_endl;
		}
		else {
			status = write_verify(REG_8PORT_OLAT, 1, &relayRegister); // set latches
			writeInSync();
			status |= write_verify(REG_8PORT_IODIR, 1, pullUp_out); // set all as outputs
			if (status) logger() << "\nInitialise RelaysPort() lat-write failed at Freq: " << i2C().getI2CFrequency() << L_endl;
			//else logger() << "Initialise RelaysPort() succeeded at Freq:", i2C().getI2CFrequency());
		}
		return status;
	}

	error_codes RelaysPort::setAndTestRegister() {
		uint8_t ANDmask = 0x7F;

		writeInSync();
		auto status = write_verify(REG_8PORT_OLAT & ANDmask, 1, &relayRegister);

		if (status) {
			auto strategy = static_cast<I2C_Recover_Retest &>(recovery()).strategy();
			strategy.stackTrace(++st_index, "RelayFail");
			if (!logger().isWorking()) {
				strategy.stackTrace(++st_index, "Logger Failed");
				while (true); // cause timeout to reset...
			} else logger() << L_time << "RelaysPort::setAndTestRegister() Register: 0x" << L_hex << (REG_8PORT_OLAT & ANDmask) << " Data: 0x" << relayRegister << i2C().getStatusMsg(status) << L_endl;
		}
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
		uint8_t myBitBinaryState = (RelaysPort::relayRegister  & myBitMask) != 0;
		uint8_t currState = !(myBitBinaryState^activeState());
		myBitBinaryState = !(state^activeState()); // Required bit binary state 
		if (!myBitBinaryState) { // clear bit
			RelaysPort::relayRegister &= ~myBitMask;
		}
		else { // set this bit
			RelaysPort::relayRegister |= myBitMask;
		}
		//logger() << L_time << "setRelay Port: " << port() << " to " << state << " bits: 0x" << L_hex << RelaysPort::relayRegister << L_endl;
		return currState != state; // returns true if state is changed
	}
}