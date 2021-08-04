#pragma once
#include <Arduino.h>
#include <Timer_mS_uS.h>

namespace HardwareInterfaces {
	constexpr uint8_t DISPLAY_WAKE_TIME = 30;
	constexpr uint8_t KEY_QUEUE_LENGTH = 3;

	enum { L_ReadTime, L_ReadAgain, L_Key, L_NoOfFields, L_NoOfLogs = 100 };
	//extern unsigned char log[L_NoOfFields][L_NoOfLogs];
	//extern int logIndex;

	/// <summary>
	/// Debounced with EMI protection with multiple re-reads.
	/// Allows use of ISR or read() to detect pin-change.
	/// When a change is detected, it triggers consecutive reads to establish a correct result.
	/// Reading every 50mS gives instant-feeling response.
	/// </summary>
	class I_Keypad {
	public:
		enum KeyOperation { BAD_KEY = -2, NO_KEY = -1, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SELECT, KEY_BACK, KEY_UP_OR_INFO, KEY_INFO, KEY_WAKEUP };
		enum KeyMode { MODE_UP_DOWN = KEY_DOWN / 2, MODE_LEFT_RIGHT = KEY_RIGHT / 2, MODE_SELECT_BACK = KEY_BACK / 2 };
		static constexpr int IDENTICAL_READ_COUNT = 4;
		static constexpr int RE_READ_PERIOD_mS = 5;
		
		virtual void startRead(); // for interrupt driven keypads
		void readKey(); // for non-interrupt driven keypads
		KeyOperation popKey();
		virtual KeyOperation getKeyCode() = 0;
		bool oneSecondElapsed();
		bool displayIsAwake() { return _secsToKeepAwake > 0; }
		void wakeDisplay() { _secsToKeepAwake = DISPLAY_WAKE_TIME; }

		bool keyIsWaiting() { return keyQueEnd > -1; }
		void clearKeys() { keyQueEnd = -1; }
		
		void putKey(KeyOperation myKey);

#if defined (ZPSIM)
		KeyOperation simKey = NO_KEY;
#endif
		volatile KeyOperation keyQue[KEY_QUEUE_LENGTH] = { NO_KEY };
		volatile int8_t keyQueEnd = -1; // index of last key
		static void _getStableKey();

	protected:
		I_Keypad(int re_read_interval = 50);
	private:
		static I_Keypad* _currentKeypad;
		static KeyOperation _prevKey;
		static uint8_t _readAgain;

		int8_t	_secsToKeepAwake = 10;
		uint8_t	_lastSecond = 0;
		Timer_mS _timeToRead;
	};
}

