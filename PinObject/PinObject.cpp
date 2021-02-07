#include "PinObject.h"

namespace HardwareInterfaces {
	// *******  Digital In-Pin  *******
	Pin_Watch::Pin_Watch(int pinNo, bool activeState, int pullUpMode) : Flag(pinNo, activeState, pullUpMode) {
		pinMode(pinNo, pullUpMode);
	}

	void Pin_Watch::begin() {
		pinMode(port(), _pullUp);
	};

	void Pin_Watch::initialise(int pinNo, bool activeState, int pullUpMode) {
		Flag::initialise(pinNo, activeState, pullUpMode);
		begin();
	}

	bool Pin_Watch::logicalState() const {
		return (digitalRead(port()) == activeState());
	}


	// *******  Digital Out-Pin  *******

	Pin_Wag::Pin_Wag(int pinNo, bool activeState, bool startSet) : Flag(pinNo, activeState) {
		begin(startSet);
	}

	void Pin_Wag::begin(bool startSet) {
		_logical_state = !startSet;
		set(startSet); // Pre-write so that when set to OUTPUT it retains its state, otherwise you get a glitch.
		pinMode(port(), OUTPUT);
	};

	void Pin_Wag::initialise(int pinNo, bool activeState, bool startSet) {
		Flag::initialise(pinNo, activeState);
		begin(startSet);
	}

	bool Pin_Wag::set(bool state) { // returns true if state is changed
		//logger() << "Pin_Wag set. Port: " << int(_port) << " State: " << int(_logical_state) << " NewState: " << state << L_endl;
		if (Flag::set(state)) {
			digitalWrite(port(), controlState());
			//logger() << "Pin_Wag PinWrite. Port: " << int(_port) << " to: " << controlState() << L_endl;
			return true;
		}
		else return false;
	};
}