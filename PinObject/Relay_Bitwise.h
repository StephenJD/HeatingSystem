#pragma once
#include "Arduino.h"
#include "PinObject.h"
#include <I2C_Talk.h>
#include <I2C_Device.h>
#include <I2C_Recover.h>

namespace HardwareInterfaces {
	// *******  Bitwise-mapped relays *******

	using RelayPortWidth_T = uint8_t;
	/// <summary>
	/// Used to register bit-mapped relay control-state
	/// and write the bit-map to the hardware.
	/// Constructed with the connected relays mask.
	/// Client must provide derived updateRelays() implementation.
	/// </summary>
	class Bitwise_RelayController {
	public:
		void registerRelayChange(Flag relay);
		virtual auto updateRelays() -> I2C_Talk_ErrorCodes::error_codes  = 0;
		virtual auto readPorts() -> I2C_Talk_ErrorCodes::error_codes  = 0;
		bool portState(Flag relay);
	protected:
		Bitwise_RelayController(RelayPortWidth_T connected_relays) : _connected_relays(connected_relays) {}
		RelayPortWidth_T _connected_relays;
		RelayPortWidth_T _relayRegister = 0;
	};

	Bitwise_RelayController & relayController(); // to be provided by client

	/// <summary>
	/// Bitwise-mapped relay constructed with its active state and "Port Number".
	/// Port-number[0-64] is the bit-position for a collection of bit-mapped relays.
	/// Changing the relay state is registered with an FlagController object
	/// which provides updateRelays() to write to the hardware.
	/// The client must supply relayController() returning an FlagController reference.
	/// </summary>
	class Relay_B : public Flag { // Bitwise-mapped relays
	public:
		Relay_B(int port, bool activeState);
		explicit Relay_B(uint8_t relayRec);

		void initialise(int port, bool activeState);
		void initialise(uint8_t relayRec);
		bool set() { return set(true); }
		bool set(bool state); // returns true if state is changed			
		void checkControllerStateCorrect();
		void getStateFromContoller();
	};

	// cannot be constexpr because I_I2Cdevice_Recovery cannot be literal
	class RelaysPort : public Bitwise_RelayController, public I_I2Cdevice_Recovery {
	public:
		RelaysPort(RelayPortWidth_T connected_relays, I2C_Recovery::I2C_Recover & recovery, int addr);

		// Virtual Functions
		auto updateRelays()->I2C_Talk_ErrorCodes::error_codes override;
		auto readPorts()->I2C_Talk_ErrorCodes::error_codes override;
		auto initialiseDevice()->I2C_Talk_ErrorCodes::error_codes override;
		auto testDevice()->I2C_Talk_ErrorCodes::error_codes override;
	private:
	};
}