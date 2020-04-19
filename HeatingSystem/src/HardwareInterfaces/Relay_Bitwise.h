#pragma once
#include "Arduino.h"
#include "Relay.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <RDB.h>
#include <i2c_Device.h>
#include <I2C_Recover.h>

namespace HardwareInterfaces {

	// cannot be constexpr because I_I2Cdevice_Recovery cannot be literal
	class RelaysPort : public Bitwise_RelayController, public I_I2Cdevice_Recovery {
	public:
		RelaysPort(RelayFlags connected_relays, I2C_Recovery::I2C_Recover & recovery, int addr, int zeroCrossPin, int resetPin);

		I2C_Talk_ErrorCodes::error_codes updateRelays() override;

		// Virtual Functions
		auto initialiseDevice()->I2C_Talk_ErrorCodes::error_codes override;
		auto testDevice()->I2C_Talk_ErrorCodes::error_codes override { return initialiseDevice(); }
	private:
		uint8_t _zeroCrossPin = 0;
		uint8_t _resetPin = 0;
	};

	class Bitwise_Relay : public Relay_B {
	public:
		Bitwise_Relay() : Relay_B(0, 0) {}
		// Queries
		RelationalDatabase::RecordID recordID() const { return _recordID; }
		// Modifiers
		void initialise(int recordID, int port, bool activeState) { _recordID = recordID; Relay_B::initialise(port, activeState); }

	private:
		RelationalDatabase::RecordID _recordID = 0;
	};
}