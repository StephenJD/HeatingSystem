#include "Keypad.h"
#include <Timer_mS_uS.h>
#include <Clock.h>

namespace HardwareInterfaces {
	//#if defined (ZPSIM)
	//	int8_t I_Keypad::simKey = -1;
	//#endif

		// Mutex supporting multiple keypads
	namespace { volatile bool keyQueueMutex = false; }

	bool I_Keypad::oneSecondElapsed() {
		bool isNewSecond = clock_().isNewSecond(_lastSecond);
		if (isNewSecond && _secsToKeepAwake > 0) {
			--_secsToKeepAwake; // timeSince set to DISPLAY_WAKE_TIME whenever a key is pressed
		}
		return isNewSecond;
	}

	void I_Keypad::putKey(int8_t myKey) {
		if (!keyQueueMutex && keyQuePos < 9 && myKey >= 0) { // 
			++keyQuePos;
			keyQue[keyQuePos] = myKey;
		}
	}

	void I_Keypad::readKey(int port) {
		auto myKey = -1;
		if (_timeToRead) {
			myKey = getKeyCode(port);
			putKey(myKey);
			_timeToRead.repeat();
		}
	}

	int I_Keypad::getKey() {
		// "get" might be interrupted by "put" during an interrupt.
		int retKey = keyQue[0];
		if (keyQuePos >= 0) {
			keyQueueMutex = true;
			if (keyQuePos > 0) { // keyQuePos is index of last valid key
				for (int i = 0; i < keyQuePos; ++i) {
					keyQue[i] = keyQue[i + 1];
				}
			} else {
				keyQue[0] = NO_KEY;
			}
			--keyQuePos;
			keyQueueMutex = false;
		}

		if (retKey > NO_KEY) {
			if (!displayIsAwake()) retKey = KEY_WAKEUP;
			wakeDisplay();
		}
		return retKey;
	}
}