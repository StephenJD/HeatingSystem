#pragma once
#include "..\..\..\MultiCrystal\MultiCrystal.h"
#include "LCD_Display.h"

extern byte BRIGHNESS_PWM;
extern byte CONTRAST_PWM;

namespace HardwareInterfaces {


	class LocalDisplay : public LCD_Display_Buffer<20,4>
	{
	public:
		LocalDisplay(RelationalDatabase::Query * query = 0);
		void blinkCursor(bool isAwake) override;
		void sendToDisplay() override;
		void setBackLight(bool wake) override;
		uint8_t ambientLight() const override;

		void changeContrast(int changeBy);
		void changeBacklight(int changeBy);

#if defined (ZPSIM)
		MultiCrystal & lcd() { return _lcd; }
#endif
	private:
		MultiCrystal _lcd;
		bool _cursorOn = true;
		bool _wasBlinking = false;
	};
}

struct LocalLCD_Pinset {
	uint8_t pins[11];
};
LocalLCD_Pinset localLCDpins(); // Function must be provided by client.
