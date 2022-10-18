#pragma once
#include <Bitfield.h>
#include <Timer_mS_uS.h>

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
			_pullUp = pullUpMode == INPUT_PULLUP;
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
	/// Digital In-Pin constructed with its active state and Pin Number, char-sized.
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
	
	// *******  Digital Debounced In-Pin  *******
	/// <summary>
	/// Digital In-Pin constructed with its active state and Pin Number, char-sized.
	/// Debounced with EMI protection with multiple re-reads.
	/// Does NOT use ISR to detect pin-change, as this is inefficient and troublesome.
	/// When a logicalState() or pinChanged() is called, it triggers consecutive reads to establish a correct result.
	/// Reading every 50mS gives instant-feeling response.
	/// May require begin() in setup to initialise pin-mode.
	/// </summary>
	class Pin_Watch_Debounced : public Pin_Watch { // Digital In-Pin 
	public:
		static constexpr int IDENTICAL_READ_COUNT = 4;
		static constexpr int RE_READ_PERIOD_mS = 1;

		using Pin_Watch::Pin_Watch;

		/// <summary>
		/// Non-blocking. Triggers mutiple Reads.
		/// Returns 1 if newly ON, -1 if newly OFF, 0 if no change.
		/// </summary>
		int pinChanged();

		/// <summary>
		/// Non-blocking. Triggers multiple Reads.
		/// Returns the stable state.
		/// </summary>
		bool logicalState();

	private:
		void _debouncePin();
		void _checkState();
		static void startDebounce(Pin_Watch_Debounced * currentPin);
		static void timerFired();
		static void stopDebounce();
		static Pin_Watch_Debounced * _currentPin;
		mutable bool _previousState = false;
		mutable uint8_t _readAgain = IDENTICAL_READ_COUNT;
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
	
	// *******  Digital OpenCollector-Pin  *******
	/// <summary>
	/// Digital OpenCollector-Pin constructed with its active state and Pin Number, char-sized.
	/// When output is HIGH, set as INPUT (simulates open-collector).
	/// May require begin() in setup to initialise pin-mode.
	/// </summary>
	class Pin_OpenCollector : public Flag { // Digital Out-Pin 
	public:
		Pin_OpenCollector(int pinNo, bool activeState, bool startSet = false);
		void initialise(int pinNo, bool activeState, bool startSet = false);
		void begin(bool startSet = false);
		bool set(bool state);
		bool set() { return set(true); }
		bool clear() { return set(false); }
	};
}