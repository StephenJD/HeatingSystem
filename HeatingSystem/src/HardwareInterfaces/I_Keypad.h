#pragma once
#include <Arduino.h>

namespace HardwareInterfaces {

	class I_Keypad
	{
	public:
		virtual int getKey() = 0;
		virtual	int readKey() = 0;
		virtual bool isTimeToRefresh();
#if defined (ZPSIM)
		static	int8_t simKey;
#endif
		volatile int8_t keyQue[10];
		volatile int8_t keyQuePos; // points to last entry in KeyQue
		bool wakeDisplay(bool wake);

	protected:
		I_Keypad();
		int8_t		_timeSince;
		uint32_t	_lastTick;
	};


}

	// Functions supporting local interrupt
	int getFromKeyQue(volatile int8_t * keyQue, volatile int8_t & keyQuePos);
	bool putInKeyQue(volatile int8_t * keyQue, volatile int8_t & keyQuePos, int8_t myKey);
