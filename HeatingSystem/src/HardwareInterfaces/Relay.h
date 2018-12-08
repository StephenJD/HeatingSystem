#pragma once
#include "Arduino.h"
#include "I2C_Helper.h"

namespace HardwareInterfaces {

	class RelaysPort : public I2C_Helper::I_I2Cdevice {
	public:
		void setup(I2C_Helper & i2C, int i2cAddress, int zeroCrossPin, int resetPin);
		uint8_t setAndTestRegister();
		// Virtual Functions
		uint8_t initialiseDevice() override;
		uint8_t testDevice(I2C_Helper & i2c, int addr) override { return initialiseDevice(); }
		uint8_t _relayRegister = 0xFF;
	private:
		uint8_t _zeroCrossPin = 0;
		uint8_t _resetPin = 0;
		int _error = 0;
	};

	class Relay {
	public:
		// Queries
		bool getRelayState() const;
		// Modifiers
		bool setRelay(uint8_t state); // returns true if state is changed
		void setPort(int port) { _port = port; }
		static uint8_t * _relayRegister;

	private:
		uint8_t activeState() const { return _port & 1;}
		uint8_t port() const { return _port >> 1;}
		int8_t _port = 0; // port[0-15] and active state (lsb == active state)
	};

	extern Relay * relays; // Array of Relay provided by client
}