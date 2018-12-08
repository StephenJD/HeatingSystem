#pragma once
#include <MultiCrystal.h>
#include "DisplayBuffer.h"

namespace HardwareInterfaces {


	class LocalDisplay : public DisplayBuffer<20,4>
	{
	public:
		LocalDisplay();
		void sendToDisplay() override;
#if defined (ZPSIM)
		MultiCrystal & lcd() { return _lcd; }
#endif
	private:
		MultiCrystal _lcd;
	};
}

struct LocalLCD_Pinset {
	uint8_t pins[11];
};
LocalLCD_Pinset localLCDpins(); // Function must be provided by client.
