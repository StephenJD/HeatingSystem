#include <LCD_Display.h>

using namespace RelationalDatabase;

namespace HardwareInterfaces {

	void LCD_Display::setCursor(int col, int row) {
		auto cursorPos = col + row * cols();
		setCursorPos(cursorPos);
		for (int i = end(); i < cursorPos; ++i) {
			buff()[i] = ' ';
		}
		if (cursorPos > end()) setEnd(cursorPos);
	}

	size_t LCD_Display::write(uint8_t character) {
		auto cursorAt = cursorPos();
		buff()[cursorAt] = character;
		++cursorAt;
		setCursorPos(cursorAt);
		if (cursorAt > end()) setEnd(cursorAt);
		return 1;
	}

	void LCD_Display::print(const char * str, uint32_t val) {
		print(str);
		print(val);
	}

	void LCD_Display::reset() {
		setEnd(0);
		setCursorPos(0);
		setCursorMode(e_unselected);
	}

	void LCD_Display::clearFromEnd() {
		for (int i = end(); i <= size(); ++i) {
			buff()[i] = ' ';
		}
		setEnd(size());
	}
}


