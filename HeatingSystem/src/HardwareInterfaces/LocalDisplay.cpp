#include "LocalDisplay.h"

namespace HardwareInterfaces {

	LocalDisplay::LocalDisplay() : _lcd(localLCDpins().pins)
	{
		_lcd.begin(20, 4);
		_lcd.clear();
		print("Heating Controller");
		Serial.println("LocalDisplay Constructed");
	}

	void LocalDisplay::sendToDisplay() {
		_lcd.clear();
		_lcd.print(buff());
		_lcd.setCursor(cursorCol(), cursorRow());
		switch (cursorMode()) {
		case e_unselected: _lcd.noCursor(); _lcd.noBlink(); break;
		case e_selected: _lcd.cursor(); _lcd.noBlink(); break;
		case e_inEdit:  _lcd.noCursor(); _lcd.blink(); break;
		}
	}

}