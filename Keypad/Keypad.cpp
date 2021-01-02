#include "Keypad.h"
#include <Clock.h>

using namespace Date_Time;

namespace HardwareInterfaces {
//#if defined (ZPSIM)
//	int8_t I_Keypad::simKey = -1;
//#endif

	bool I_Keypad::isTimeToRefresh() {
		bool isNewSecond = clock_().isNewSecond(_lastSecond);
		if (isNewSecond && _secsToKeepAwake > 0) {
			--_secsToKeepAwake; // timeSince set to DISPLAY_WAKE_TIME whenever a key is pressed
		}
		return (isNewSecond); // refresh every second
	}

	bool I_Keypad::wakeDisplay(bool wake) {
		if (wake) _secsToKeepAwake = DISPLAY_WAKE_TIME;
		return _secsToKeepAwake > 0;
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
				for (int i = 0; i < keyQuePos; ++i) {
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


