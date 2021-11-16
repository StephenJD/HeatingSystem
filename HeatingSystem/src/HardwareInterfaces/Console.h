#pragma once
#include "..\LCD_UI\UI_DisplayBuffer.h"

namespace LCD_UI {
	class Chapter_Generator;
}

namespace HardwareInterfaces {
	class I_Keypad;

	class Console
	{
	public:
		Console(I_Keypad & keyPad, LCD_Display & lcd_display, LCD_UI::Chapter_Generator & chapterGenerator);
		int consoleMode() const;
		bool processKeys();
		void refreshDisplay();
	private:
		I_Keypad & _keyPad;
		LCD_UI::UI_DisplayBuffer _lcd_UI;
		LCD_UI::Chapter_Generator & _chapterGenerator;
	};

}