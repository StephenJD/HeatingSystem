#pragma once
#include <Arduino.h>
#include <Timer_mS_uS.h>

namespace HardwareInterfaces {
	const uint8_t DISPLAY_WAKE_TIME = 30;

	class I_Keypad {
	public:
		enum KeyOperation { BAD_KEY = -2, NO_KEY = -1, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SELECT, KEY_BACK, KEY_INFO, KEY_WAKEUP };
		enum KeyMode { MODE_UP_DOWN = KEY_DOWN / 2, MODE_LEFT_RIGHT = KEY_RIGHT / 2, MODE_SELECT_BACK = KEY_BACK / 2 };
		virtual int getKey();
		virtual int getKeyCode(int port) = 0;
		bool oneSecondElapsed();
		bool displayIsAwake() { return _secsToKeepAwake > 0; }
		void wakeDisplay() { _secsToKeepAwake = DISPLAY_WAKE_TIME; }
		void readKey(int port);

		bool keyIsWaiting() { return keyQuePos != -1; }
		void clearKeys() { keyQue[0] = -1;  keyQuePos = -1; }

#if defined (ZPSIM)
		int8_t simKey = -1;
#endif
		volatile int8_t keyQue[10] = { -1 };
		volatile int8_t keyQuePos = -1; // points to last entry in KeyQue

	protected:
		I_Keypad(int re_read_interval = 50) : _timeToRead{ re_read_interval } {}
		int8_t	_secsToKeepAwake = 10;
		uint8_t	_lastSecond = 0;
		Timer_mS _timeToRead;
	};

	// Functions supporting local interrupt
	int getFromKeyQue(volatile int8_t* keyQue, volatile int8_t& keyQuePos);
	bool putInKeyQue(volatile int8_t* keyQue, volatile int8_t& keyQuePos, int8_t myKey);
}

