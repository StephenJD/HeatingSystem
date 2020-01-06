#pragma once
#include "Arduino.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <RDB.h>
#include <i2c_Device.h>
#include <I2C_Recover.h>

namespace HardwareInterfaces {

	class RelaysPort : public I_I2Cdevice_Recovery {
	public:
		RelaysPort(I2C_Recovery::I2C_Recover & recovery, int addr, int zeroCrossPin, int resetPin );

		auto setAndTestRegister()->I2C_Talk_ErrorCodes::error_codes;
		// Virtual Functions
		auto initialiseDevice()->I2C_Talk_ErrorCodes::error_codes override;
		auto testDevice()->I2C_Talk_ErrorCodes::error_codes override { return initialiseDevice(); }
		static uint8_t relayRegister;
	private:
		uint8_t _zeroCrossPin = 0;
		uint8_t _resetPin = 0;
	};

	class Relay {
	public:
		// Queries
		bool getRelayState() const;
		RelationalDatabase::RecordID recordID() const { return _recordID; }
		// Modifiers
		bool setRelay(uint8_t state); // returns true if state is changed
		void initialise(int recordID, int port) { _recordID = recordID; _port = port;}

	private:
		RelationalDatabase::RecordID _recordID = 0;
		uint8_t activeState() const { return _port & 1;}
		uint8_t port() const { return _port >> 1;}
		int8_t _port = 0; // port[0-15] and active state (lsb == active state)
	};
}