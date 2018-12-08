#pragma once
#include <Arduino.h>

namespace HardwareInterfaces {

	class DisplayBuffer_I : public Print
	{
	public:
		enum CursorMode : unsigned char { e_unselected, e_selected, e_inEdit};
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
		const char * buff() const { return const_cast<DisplayBuffer_I *>(this)->buff(); }

		// Modifiers
		void setCursor(int col, int row);
		size_t write(uint8_t) override;
		using Print::print;
		void print(const char * str, uint32_t val);
		void reset();
		virtual char * buff() = 0;
		void setCursorPos(int pos) { *(buff() - 2) = pos; }
		void setCursorMode(CursorMode mode) { *(buff() - 1) = mode; }
	protected:
		// Modifiers
	};

	template<int _cols, int _rows>
	class DisplayBuffer : public DisplayBuffer_I {
	public:
		DisplayBuffer() {
			for (auto & character : _buff) character = 0;
		}
		int cols() const override { return _cols; }
		int size() const override { return _cols * _rows; } // no of characters, not including null.
		char * buff() override { return _buff + 2; }
	private:
		char _buff[_cols * _rows + 3] = { 0 }; // [0] == cursorPos, [1] == cursorMode. Last is \0
	};

}