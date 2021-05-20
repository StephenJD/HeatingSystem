#pragma once
#include <Arduino.h>

namespace HardwareInterfaces {
	const uint8_t DISPLAY_WAKE_TIME = 30;

	class I_Keypad
	{
	public:
		virtual int getKey();
		virtual bool isTimeToRefresh();
		virtual bool wakeDisplay(bool wake);
		bool keyIsWaiting() {return keyQuePos != -1;	}
		void clearKeys() { keyQue[0] = -1;  keyQuePos = -1; }
		void putKey(int8_t myKey);

#if defined (ZPSIM)
		int8_t simKey = -1;
#endif
		volatile int8_t keyQue[10] = { -1 };
		volatile int8_t keyQuePos = -1; // points to last entry in KeyQue

	protected:
		int8_t	_secsToKeepAwake = 0;
		uint8_t	_lastSecond = 0;
	};
}

