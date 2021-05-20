#include "Keypad.h"
#include <Clock.h>
#include <Logging.h>
using namespace arduino_logger;

using namespace Date_Time;

namespace HardwareInterfaces {
//#if defined (ZPSIM)
//	int8_t I_Keypad::simKey = -1;
//#endif

	// Mutex supporting multiple keypads
	namespace { volatile bool keyQueueMutex = false; }

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

	void I_Keypad::putKey(int8_t myKey) {
		if (!keyQueueMutex && keyQuePos < 9 && myKey >= 0) { // 
			++keyQuePos;
			keyQue[keyQuePos] = myKey;
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


