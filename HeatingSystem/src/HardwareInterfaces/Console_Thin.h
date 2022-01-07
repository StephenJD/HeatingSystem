#pragma once
#include "..\LCD_UI\UI_DisplayBuffer.h"

namespace LCD_UI {
	class Chapter_Generator;
}

namespace HardwareInterfaces {
	class I_Keypad;

	class Console_Thin
	{
	public:
		Console_Thin(I_Keypad & keyPad, LCD_Display & lcd_display, LCD_UI::Chapter_Generator & chapterGenerator);
		uint8_t& consoleMode();
		bool processKeys();
		void refreshDisplay();
	private:
		I_Keypad & _keyPad;
		LCD_UI::UI_DisplayBuffer _lcd_UI;
		LCD_UI::Chapter_Generator & _chapterGenerator;
	};

}