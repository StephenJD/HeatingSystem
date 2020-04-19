#include "Relay_Bitwise.h"
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
	
	RelaysPort::RelaysPort(RelayFlags connected_relays, I2C_Recover & recovery, int addr, int zeroCrossPin, int resetPin)
		: Bitwise_RelayController(connected_relays)
		, I_I2Cdevice_Recovery{ recovery, addr }
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
			status = write_verify(REG_8PORT_OLAT, 1, &_relayRegister); // set latches
			writeInSync();
			status |= write_verify(REG_8PORT_IODIR, 1, pullUp_out); // set all as outputs
			if (status) logger() << "\nInitialise RelaysPort() lat-write failed at Freq: " << i2C().getI2CFrequency() << L_endl;
			//else logger() << "Initialise RelaysPort() succeeded at Freq:", i2C().getI2CFrequency());
		}
		return status;
	}

	error_codes RelaysPort::updateRelays() {
		writeInSync();
		uint8_t io_direction[] = { 255 }; // 0 == output
		auto status = read(REG_8PORT_IODIR, 1, io_direction); 
		if (status != _OK || io_direction[0] != 0) {
			io_direction[0] = 0;
			status = write_verify(REG_8PORT_IODIR, 1, io_direction); // set all as outputs
		}
		if (status == _OK) status = write_verify(REG_8PORT_OLAT & _connected_relays, 1, &_relayRegister);

		if (status) {
			auto strategy = static_cast<I2C_Recover_Retest &>(recovery()).strategy();
			strategy.stackTrace(++st_index, "RelayFail");
			if (!logger().isWorking()) {
				strategy.stackTrace(++st_index, "Logger Failed");
			} else logger() << L_time << "RelaysPort::setAndTestRegister() Register: 0x" << L_hex << (REG_8PORT_OLAT & _connected_relays) << " Data: 0x" << _relayRegister << i2C().getStatusMsg(status) << L_endl;
		}
		return status;
	}
}