#include "Relay.h"

namespace HardwareInterfaces {

	void Bitwise_RelayController::registerRelayChange(I_Relay relay) {
		uint8_t relayFlagMask = 1 << relay.port();
		if (relay.controlState()) {  // set this bit
			_relayRegister |= relayFlagMask;
		} else {// clear bit
			_relayRegister &= ~relayFlagMask;
		}
		//logger() << L_time << "set RelayPort: " << port() << " to " << state << " bits: 0x" << L_hex << RelaysPort::relayRegister << L_endl;
	}

}