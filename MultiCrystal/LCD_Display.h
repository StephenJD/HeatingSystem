#pragma once
#include <Arduino.h>
#include <RDB.h>

namespace HardwareInterfaces {

	class LCD_Display : public Print
	{
	public:
		enum CursorMode : uint8_t { e_unselectable, e_unselected, e_selected, e_inEdit};
		LCD_Display(RelationalDatabase::Query * query = 0) : _query(query) {}
		// Queries
		uint8_t cursorPos() const { return *(buff() - 2); }
		uint8_t cursorCol() const { return cursorPos() % cols(); }
		uint8_t cursorRow() const { return cursorPos() / cols(); }
		CursorMode cursorMode() const { return static_cast<CursorMode>(*(buff() - 1)); }
		operator const char * () const { return buff(); }
		virtual int size() const = 0;
		virtual int cols() const = 0;
		virtual void sendToDisplay() {}
		//int rows() const { return size() / cols(); }
		const char * buff() const { return const_cast<LCD_Display *>(this)->buff(); }
		virtual uint8_t ambientLight() const { return 0; }

		// Modifiers
		virtual void setBackLight(bool wake) {}
		virtual char * buff() = 0;
		void setCursor(int col, int row);
		virtual void blinkCursor(bool isAwake) {}
		size_t write(uint8_t) override;
		using Print::print;
		void print(const char * str, uint32_t val);
		void reset();
		void truncate(int newEnd);
		void setCursorPos(int pos) { *(buff() - 2) = pos; }
		void setCursorMode(CursorMode mode) { *(buff() - 1) = mode; }
	protected:
		RelationalDatabase::Query * _query = 0;
	};

	template<int _cols, int _rows>
	class LCD_Display_Buffer : public LCD_Display {
	public:
		LCD_Display_Buffer(RelationalDatabase::Query * query = 0) : LCD_Display(query) {
			for (auto & character : _buff) character = 0;
		}
		int cols() const override { return _cols; }
		int size() const override { return _cols * _rows; } // no of characters, not including null.
		char * buff() override { return _buff + 2; }
	private:
		char _buff[_cols * _rows + 3] = { 0 }; // [0] == cursorPos, [1] == cursorMode. Last is \0
	};

	struct R_Display {
		char name[5];
		uint8_t addr;
		uint8_t contrast;
		uint8_t backlight_bright;
		uint8_t backlight_dim;
		uint8_t photo_bright;
		uint8_t photo_dim;
		uint8_t timeout;

		bool operator < (R_Display rhs) const { return false; }
		bool operator == (R_Display rhs) const { return true; }
	};
	inline Logger& operator << (Logger& stream, const R_Display & display) {
		return stream << F("Display ") << (int)display.name;
	}
}