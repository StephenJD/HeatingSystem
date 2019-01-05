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
		void sendToDisplay() override;
		void setBackLight(bool wake) override;
		uint8_t ambientLight() const override;

		void saveContrast(int contrast);
		void saveBrightBacklight(int backlight);
		void saveDimBacklight(int backlight);

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
