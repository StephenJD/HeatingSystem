#pragma once
#include "..\LCD_UI\UI_DisplayBuffer.h"

namespace LCD_UI {
	class A_Top_UI;
}

namespace HardwareInterfaces {
	class I_Keypad;

	class Console
	{
	public:
		Console(I_Keypad & keyPad, LCD_Display & lcd_display, LCD_UI::A_Top_UI & pageGenerator);
		bool processKeys();
	private:
		I_Keypad & _keyPad;
		LCD_UI::UI_DisplayBuffer _lcd_UI;
		LCD_UI::A_Top_UI & _pageGenerator;
	};

}