#include <LCD_Display.h>

using namespace RelationalDatabase;

namespace HardwareInterfaces {

	void LCD_Display::setCursor(int col, int row) {
		setCursorPos(col + row * cols());
		for (int i = strlen(buff()); i < cursorPos(); ++i) {
			buff()[i] = ' ';
		}
	}

	size_t LCD_Display::write(uint8_t character) {
		buff()[cursorPos()] = character;
		setCursorPos(cursorPos() + 1);
		return 1;
	}

	void LCD_Display::print(const char * str, uint32_t val) {
		print(str);
		print(val);
	}

	void LCD_Display::reset() {
		buff()[0] = 0;
		setCursorPos(0);
		setCursorMode(e_unselected);
	}

	void LCD_Display::truncate(int newEnd) {
		if (newEnd <= size()) buff()[newEnd] = 0;
	}
}


