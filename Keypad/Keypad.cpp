#include "Keypad.h"
#include <Timer_mS_uS.h>
#include <Clock.h>

#if defined(__SAM3X8E__)
	#include <DueTimer.h>
	auto dueTimer = Timer.getAvailable();
	
	void startTimer() { 
		//Serial.println("st");
		dueTimer.attachInterrupt(HardwareInterfaces::I_Keypad::_getStableKey);
		dueTimer.setPeriod(HardwareInterfaces::I_Keypad::RE_READ_PERIOD_mS * 1000);
		dueTimer.start();
	}
	
	void stopTimer() { dueTimer.stop();/* Serial.println("Stop");*/ }
#else
	#include <MsTimer2.h>
	void startTimer() { 
		MsTimer2::set(HardwareInterfaces::I_Keypad::RE_READ_PERIOD_mS, HardwareInterfaces::I_Keypad::_getStableKey);
		MsTimer2::start(); 
	}
	void stopTimer() { MsTimer2::stop(); }
#endif

namespace HardwareInterfaces {
	//enum { L_ReadTime, L_ReadAgain, L_Key, L_NoOfFields, L_NoOfLogs = 100 };
	//unsigned char keyLog[L_NoOfFields][L_NoOfLogs] = { 0 };
	//int logIndex = 0;

	namespace { volatile bool keyQueueLocked = false; }
	I_Keypad* I_Keypad::_currentKeypad;
	I_Keypad::KeyOperation I_Keypad::_prevKey = NO_KEY;
	uint8_t I_Keypad::_readAgain = IDENTICAL_READ_COUNT;

	I_Keypad::I_Keypad(int re_read_interval_mS, int wakeTime_S)
		: _timeToRead{ re_read_interval_mS }
		, _wakeTime{ uint8_t(wakeTime_S)}
		{}

	bool I_Keypad::oneSecondElapsed() {
		bool isNewSecond = clock_().isNewSecond(_lastSecond);
		if (isNewSecond && _secsToKeepAwake > 0) {
			--_secsToKeepAwake; // timeSince set to _wakeTime whenever a key is pressed
		}
		return isNewSecond;
	}

	void I_Keypad::startRead() {
		auto key = getKeyCode();
		if (key != _prevKey) {
			_prevKey = key;
			_readAgain = IDENTICAL_READ_COUNT;
			_currentKeypad = this;
			startTimer();
		}
	}

	void I_Keypad::_getStableKey() {
		--_readAgain;
		auto key = _currentKeypad->getKeyCode();
		//if (logIndex < L_NoOfLogs - 1) ++logIndex;
		//keyLog[L_ReadTime][logIndex] = (unsigned char)millis();
		//keyLog[L_ReadAgain][logIndex] = _readAgain;
		//keyLog[L_Key][logIndex] = key;
		if (key != _prevKey) {
			_readAgain = IDENTICAL_READ_COUNT;
			_prevKey = key;
		} else if (!_readAgain) {
			stopTimer();
			static auto upKeyCombination = NO_KEY;
			if (key == KEY_UP_OR_INFO) { // wait for second key or key release
				if (upKeyCombination == NO_KEY) upKeyCombination = KEY_UP_OR_INFO;
				_readAgain = IDENTICAL_READ_COUNT;
				//Serial.println("UpInfo");
				startTimer();
				return;
			}
			// We have a non-KEY_UP_OR_INFO key, or NO_KEY.
			if (key == KEY_INFO) upKeyCombination = KEY_INFO;
			else {
				if (upKeyCombination == KEY_UP_OR_INFO) key = KEY_UP;
				upKeyCombination = NO_KEY;
			}
			_currentKeypad->putKey(key);
		}
	}

	void I_Keypad::putKey(KeyOperation myKey) { // could be called by interrupt during a getKey
		if (!keyQueueLocked && keyQueEnd < KEY_QUEUE_LENGTH - 1 && myKey > NO_KEY) { // 
			keyQueueLocked = true;
			++keyQueEnd;
			keyQue[keyQueEnd] = myKey;
			keyQueueLocked = false;

			//Serial.println(F("\nTime, ReadAgain, Key"));
			//for (int i = 0; i <= logIndex; ++i) {
			//	Serial.print(keyLog[L_ReadTime][i]); Serial.print(F("\t"));
			//	Serial.print(keyLog[L_ReadAgain][i]); Serial.print(F("\t"));
			//	Serial.println((int)keyLog[L_Key][i]);
			//}
		}
		//logIndex = -1;
	}

	void I_Keypad::readKey() {
		if (_timeToRead) {
			startRead();
			_timeToRead.repeat();
		}
	}

	I_Keypad::KeyOperation I_Keypad::popKey() { // could be interrupted by a putKey
		// Lambda
		auto popKeyFromQueue = [this]() { 
			auto retKey = NO_KEY;
			if (keyQueEnd >= 0 && !keyQueueLocked) { // keyQueEnd is pos of last valid key
				keyQueueLocked = true;
				retKey = keyQue[0];
				for (int i = 0; i < keyQueEnd; ++i) {
					keyQue[i] = keyQue[i + 1];
				}
				--keyQueEnd;
				keyQueueLocked = false;
			}
			return retKey;
		};

		// Algorithm
		auto retKey = popKeyFromQueue();
		if (retKey == NO_KEY) { 
			readKey(); // trigger read on keyboards not read on interrupts
			retKey = popKeyFromQueue();
		}

		if (retKey > NO_KEY) {
			//Serial.print("Popped: "); Serial.println((int)retKey);
			if (!displayIsAwake()) retKey = KEY_WAKEUP;
			wakeDisplay();
		}
		return retKey;
	}
}