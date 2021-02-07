#pragma once
#include <Bitfield.h>

namespace HardwareInterfaces {

	/// <summary>
	/// Non-polymorphic baseclass, char-sized object that maintains the current flag state
	/// and is constructed with its active state and "Port Number".
	/// Port-number[0-64] may be a pin-number, flag-bit position, register address etc.
	/// </summary>
	class Flag {
	public:
		Flag(int port = 0, bool activeState = true) { initialise(port, activeState); }
		
		explicit operator uint8_t() const { return _asInt; }
		
		void initialise(uint8_t flagRecord) { _asInt = flagRecord; _logical_state = true;}
		
		void initialise(int port, bool activeState) { // derived classes follow with set(false) to trigger update.
			_port = port;
			_activeState = activeState;
			_logical_state = true;
			//logger() << "flag Port: " << int(_port) << " State: " << int(_logical_state) << L_endl;
		}

		// Queries
		/// <summary>
		/// returns true if flag is logically "Set".
		/// </summary>
		bool logicalState() const { return _logical_state; }
		
		/// <summary>
		/// returns true if physical 1 represents logical "Set".
		/// </summary>
		bool activeState() const { return _activeState; }
		
		/// <summary>
		/// returns physical value for the current logical state.
		/// </summary>
		bool controlState() const { return !(logicalState() ^ activeState()); }
		
		uint8_t port() const { return _port; }

		// Modifiers	
		/// <summary>
		/// Logical-set. Returns true if state changed.
		/// </summary>
		bool set(bool state) {
			if (bool(_logical_state) == state) return false;
			else {
				_logical_state = state;
				return true;
			}
		}

		bool set() { return set(true); }

		bool clear() { return set(false); }

	protected:
		Flag(int port, bool activeState, int pullUpMode) { initialise(port, activeState, pullUpMode); }
		
		void initialise(int port, bool activeState, int pullUpMode) { // derived classes follow with set(false) to trigger update.
			_port = port;
			_activeState = activeState;
			_logical_state = pullUpMode;
		}		
		
		union {
			uint8_t _asInt;
			BitFields::UBitField<uint8_t, 0, 1> _activeState; 
			BitFields::UBitField<uint8_t, 1, 1> _pullUp; // for inputs
			BitFields::UBitField<uint8_t, 1, 1> _logical_state; // for outputs
			BitFields::UBitField<uint8_t, 2, 6> _port;
		};
	};

	// *******  Digital In-Pin  *******
	/// <summary>
	/// Digital Out-Pin constructed with its active state and Pin Number, char-sized.
	/// May require begin() in setup to initialise pin-mode.
	/// </summary>
	class Pin_Watch : public Flag { // Digital In-Pin 
	public:
		Pin_Watch() = default;
		Pin_Watch(int pinNo, bool activeState, int pullUpMode = INPUT_PULLUP);
		void initialise(int pinNo, bool activeState, int pullUpMode );
		void begin();
		
		/// <summary>
		/// Reads pin.
		/// </summary>
		bool logicalState() const;
	};	
	
	// *******  Digital Out-Pin  *******
	/// <summary>
	/// Digital Out-Pin constructed with its active state and Pin Number, char-sized.
	/// May require begin() in setup to initialise pin-mode.
	/// </summary>
	class Pin_Wag : public Flag { // Digital Out-Pin 
	public:
		Pin_Wag(int pinNo, bool activeState, bool startSet = false);
		void initialise(int pinNo, bool activeState, bool startSet = false);
		void begin(bool startSet = false);
		bool set(bool state);
		bool set() { return set(true); }
		bool clear() { return set(false); }
	};
}