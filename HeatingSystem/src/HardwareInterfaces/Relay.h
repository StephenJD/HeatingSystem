#pragma once
#include "Arduino.h"
#include <I2C_Talk\I2C_Talk.h>
#include <I2C_Talk/I2C_Device.h>
#include <RDB\src\RDB.h>

namespace HardwareInterfaces {

	class RelaysPort : public I_I2Cdevice {
	public:
		RelaysPort(I2C_Talk & i2C, int addr, int zeroCrossPin, int resetPin );

		uint8_t setAndTestRegister();
		// Virtual Functions
		uint8_t initialiseDevice() override;
		uint8_t testDevice() override { return initialiseDevice(); }
		inline static uint8_t relayRegister = 0xff;
	private:
		uint8_t _zeroCrossPin = 0;
		uint8_t _resetPin = 0;
	};

	class Relay {
	public:
		// Queries
		bool getRelayState() const;
		RelationalDatabase::RecordID recID() const { return _recID; }
		// Modifiers
		bool setRelay(uint8_t state); // returns true if state is changed
		void initialise(int recID, int port) { _recID = recID; _port = port;}

	private:
		RelationalDatabase::RecordID _recID = 0;
		uint8_t activeState() const { return _port & 1;}
		uint8_t port() const { return _port >> 1;}
		int8_t _port = 0; // port[0-15] and active state (lsb == active state)
	};
}