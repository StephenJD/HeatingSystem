#include "PinObject.h"
#include <MsTimer2.h>
#include <Logging.h>

using namespace arduino_logger;

//#define debug
#ifdef debug
	constexpr int MAX_COUNT = 20;
	uint32_t bounceTime[MAX_COUNT] = { 0 };
	uint8_t pinVal[MAX_COUNT] = { 0 };
	uint8_t readAgain[MAX_COUNT] = { 0 };
	int count = 0;
#endif

namespace HardwareInterfaces {
	// *******  Digital In-Pin  *******
	Pin_Watch::Pin_Watch(int pinNo, bool activeState, int pullUpMode) : Flag(pinNo, activeState, pullUpMode) {
		pinMode(pinNo, pullUpMode);
	}

	void Pin_Watch::begin() {
		pinMode(port(), 1+_pullUp);
	};

	void Pin_Watch::initialise(int pinNo, bool activeState, int pullUpMode) {
		Flag::initialise(pinNo, activeState, pullUpMode);
		begin();
	}

	bool Pin_Watch::logicalState() {
		return (digitalRead(port()) == activeState());
	}

	// *******  Pin_Watch_Debounced  *******

	Pin_Watch_Debounced* Pin_Watch_Debounced::_currentPin = nullptr;

	int Pin_Watch_Debounced::pinChanged() {
		_checkState();
		bool hasChanged = _readAgain == 0; 
		if (hasChanged) {
			_readAgain = IDENTICAL_READ_COUNT;
			if (_previousState) return 1; else return -1;
		} else return 0;
	}

	void Pin_Watch_Debounced::_checkState() {
#ifdef debug
		auto prevBounce = bounceTime[0];
		for (int i = 0; i < MAX_COUNT; ++i) {
			if (bounceTime[i]) {
				auto interval = bounceTime[i] - prevBounce;
				logger() << L_tabs << interval << readAgain[i] << pinVal[i] << L_endl;
				bounceTime[i] = 0;
			} else break;
		}
		count = 0;
#endif
		if (Pin_Watch::logicalState() != _previousState) {
			startDebounce(this);
		}
	}

	bool Pin_Watch_Debounced::logicalState() { // call this at least every 50mS
		if (_readAgain == 0) _readAgain = IDENTICAL_READ_COUNT;
		_checkState();
		return _previousState;
	}

	void Pin_Watch_Debounced::_debouncePin() {
		--_readAgain;

		auto thisRead = Pin_Watch::logicalState();
		if (thisRead != _previousState) {
			_readAgain = IDENTICAL_READ_COUNT;
			_previousState = thisRead;
		}  else if (!_readAgain) {
			stopDebounce();
		}
#ifdef debug
		bounceTime[count] = millis();
		readAgain[count] = _readAgain;
		pinVal[count] = thisRead;
		if (count < MAX_COUNT-2) ++count;
#endif
	}

	void Pin_Watch_Debounced::startDebounce(Pin_Watch_Debounced* currentPin) {
		// Set timer interrupt to re-read pin if change is detected.
		_currentPin = currentPin;
		MsTimer2::set(RE_READ_PERIOD_mS, timerFired);
		MsTimer2::start();
	}

	void Pin_Watch_Debounced::timerFired() {
		_currentPin->_debouncePin();
	}

	void Pin_Watch_Debounced::stopDebounce() {
		MsTimer2::stop();
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