#pragma once
#include <Bitfield.h>
#include <I2C_Talk_ErrorCodes.h>

void sendMsg(const char *);
void sendMsg(const char *, long a);

namespace HardwareInterfaces {

	/// <summary>
	/// Non-polymorphic baseclass, Char-sized object that maintains the current relay state
	/// and is constructed with its active state and "Port Number".
	/// Port-number[0-64] may be a pin-number, flag-bit position, register address etc.
	/// </summary>
	class I_Relay {
	public:
		// Queries
		bool logicalState() const { return _logical_state; }
		bool activeState() const { return _activeState; }
		bool controlState() const { return !(logicalState() ^ activeState()); }
		uint8_t port() const { return _port; }
		// Modifiers
		void initialise(int port, bool activeState) {
			_port = port;
			_activeState = activeState;
			_logical_state = false;;
		}

	protected:
		I_Relay(int port, bool activeState) { initialise(port, activeState); }

		union {
			BitFields::UBitField<uint8_t, 0, 1> _activeState;
			BitFields::UBitField<uint8_t, 1, 1> _logical_state;
			BitFields::UBitField<uint8_t, 2, 6> _port;
		};
	};

	// *******  Directly connected relay *******
	/// <summary>
	/// Directly connected relay constructed with its active state and "Port Number".
	/// Port-number[0-64] is the pin-number.
	/// </summary>
	class Relay_D : public I_Relay { // Directly connected relay
	public:
		Relay_D(int port, bool activeState) : I_Relay(port, activeState) {}

		bool set(uint8_t state) { // returns true if state is changed
			if (_logical_state == state) return false;
			else {
				_logical_state = state;
				if (state) sendMsg("Relay::set", port());
				digitalWrite(port(), controlState());
				return true;
			}
		}
	};

	// *******  Bitwise-mapped relays *******
	
	using RelayFlags = uint8_t;
	/// <summary>
	/// Used to register bit-mapped relay control-state
	/// and write the bit-map to the hardware.
	/// Constructed with the connected relays mask.
	/// Client must provide derived updateRelays() implementation.
	/// </summary>
	class Bitwise_RelayController {
	public:
		void registerRelayChange(I_Relay relay);
		virtual I2C_Talk_ErrorCodes::error_codes updateRelays() = 0;
	protected:
		Bitwise_RelayController(RelayFlags connected_relays) : _connected_relays(connected_relays) {}
		RelayFlags _connected_relays;
		RelayFlags _relayRegister = 0;
	};

	Bitwise_RelayController & relayController(); // to be provided by client

	/// <summary>
	/// Bitwise-mapped relay constructed with its active state and "Port Number".
	/// Port-number[0-64] is the bit-position for a collection of bit-mapped relays.
	/// Changing the relay state is registered with an I_RelayController object
	/// which provides updateRelays() to write to the hardware.
	/// The client must supply relayController() returning an I_RelayController reference.
	/// </summary>
	class Relay_B : public I_Relay { // Bitwise-mapped relays
	public:
		Relay_B(int port, bool activeState) : I_Relay(port, activeState) {}

		bool set(uint8_t state) { // returns true if state is changed
			if (_logical_state == state) return false;
			else {
				_logical_state = state;
				relayController().registerRelayChange(*this);
				return true;
			}
		}
	};

}
