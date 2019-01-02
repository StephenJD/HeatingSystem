#include "I_Keypad.h"
#include "Clock.h"
#include "A__Constants.h"

namespace HardwareInterfaces {
#if defined (ZPSIM)
	int8_t I_Keypad::simKey;
#endif

	I_Keypad::I_Keypad() : _timeSince(0) {
		//Display_Stream & displ
		keyQuePos = -1; // points to last entry in KeyQue
		for (auto & key : keyQue) {
			key = -1;
		}
		//log("I_Keypad Constructor");
		//	DateTime_Run::secondsSinceLastCheck(lastTick); // set lastTick for the keypad to now.
		//log("I_Keypad Base Done");
	}

	bool I_Keypad::isTimeToRefresh() {
		int elapsedTime = Clock::secondsSinceLastCheck(_lastTick);
		if (elapsedTime && _timeSince > 0) {
			_timeSince -= elapsedTime; // timeSince set to DISPLAY_WAKE_TIME whenever a key is pressed
		}
		return (elapsedTime > 0); // refresh every second
	}

	bool I_Keypad::wakeDisplay(bool wake) {
		if (wake) _timeSince = DISPLAY_WAKE_TIME;
		return _timeSince > 0;
	}

	// Functions supporting local interrupt
	namespace { volatile bool keyQueueMutex = false; }

	bool putInKeyQue(volatile int8_t * keyQue, volatile int8_t & keyQuePos, int8_t myKey) {
		if (!keyQueueMutex && keyQuePos < 9 && myKey >= 0) { // 
			++keyQuePos;
			keyQue[keyQuePos] = myKey;
			return true;
		}
		else return false;
	}

	int getFromKeyQue(volatile int8_t * keyQue, volatile int8_t & keyQuePos) {
		// "get" might be interrupted by "put" during an interrupt.
		int retKey = keyQue[0];
		if (keyQuePos >= 0) {
			keyQueueMutex = true;
			if (keyQuePos > 0) { // keyQuePos is index of last valid key
				for (int i = 0; i<keyQuePos; ++i) {
					keyQue[i] = keyQue[i + 1];
				}
			}
			else {
				keyQue[0] = -1;
			}
			--keyQuePos;
			keyQueueMutex = false;
		}
		return retKey;
	}

}
