#include "Keypad.h"
#include <Timer_mS_uS.h>
#include <Flag_Enum.h>

#include <Logging.h>
namespace arduino_logger {
	Logger& logger();
}
using namespace arduino_logger;

using namespace flag_enum;

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
	
	enum I2C_Flags { F_ENABLE_KEYBOARD, F_DATA_CHANGED, F_I2C_NOW, _NO_OF_FLAGS, NO_REG_OFFSET_SET = 255 };

	namespace { volatile bool keyQueueLocked = false; }
	I_Keypad* I_Keypad::_currentKeypad;
	I_Keypad::KeyOperation I_Keypad::_prevKey = NO_KEY;
	uint8_t I_Keypad::_readAgain = IDENTICAL_READ_COUNT;

	I_Keypad::I_Keypad(int re_read_interval_mS, int wakeTime_S)
		: _keepAwake_mS{ wakeTime_S * 1000UL }
		, _wakeTime{uint8_t(wakeTime_S / 4)}
		{ 
			FE_Ref<I2C_Flags, 4>(_wakeTime).set(F_ENABLE_KEYBOARD);
			_lapFlags.setValue(re_read_interval_mS);
		}

	bool I_Keypad::oneSecondElapsed() { // for refreshing display / registers.
		//logger() << (long)this << L_tabs  << "oneSecondElapsed..." << "Millis:" << millis()<< L_endl;
		bool prevLapFlag = _lapFlags.is(LAP_EVEN_SECOND);
		bool isNewSecond = hasLapped(1000, prevLapFlag);
		if (isNewSecond) {
			//logger() << L_tabs << "oneSecondElapsed_Now:" << !prevLapFlag << L_endl;
			_lapFlags.set(LAP_EVEN_SECOND,!prevLapFlag); // alternate even-flag to detect new second
		}
		return isNewSecond;
	}

	void I_Keypad::readKey() {
		//logger() << (long) this << L_tabs  << F(" readKey...\tMillis:") << millis() << L_endl;
		bool prevLapFlag = _lapFlags.is(LAP_READ_KEY);
		bool isTimeToRead = hasLapped(_lapFlags.value(), prevLapFlag); // _lapFlags.value() is the key-read interval
		if (isTimeToRead) {
			//logger() << L_tabs << F("readKey_Now: ") << !prevLapFlag << L_endl;
			_lapFlags.set(LAP_READ_KEY, !prevLapFlag); // timeSince set to _wakeTime whenever a key is pressed
			startRead();
		}
	}

	void I_Keypad::wakeDisplay() {
		auto consoleMode = FE_Ref<I2C_Flags,4>(_wakeTime);
		_keepAwake_mS = consoleMode.is(F_ENABLE_KEYBOARD) ? consoleMode.value()*4000 : 0;
		//logger() << (long)this << L_tabs << "wakeDisplay_Millis:" << millis() << L_endl;
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