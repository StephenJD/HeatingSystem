#pragma once
#include <LCD_Display.h>

namespace LCD_UI {

	using CursorMode = HardwareInterfaces::LCD_Display::CursorMode;

	class UI_DisplayBuffer { // cannot derive from LCD_Display, because it needs to be a polymorphic pointer
	public:
		enum ListStatus { e_showingAll, e_notShowingStart, e_not_showingEnd = e_notShowingStart * 2, e_this_not_showing = e_not_showingEnd * 2 };
		
		UI_DisplayBuffer(HardwareInterfaces::LCD_Display & displayBuffer) : _lcd(&displayBuffer) {}
		
		//Queries
		const char * toCStr() const { return _lcd->buff(); }
		CursorMode cursorMode() { return _lcd->cursorMode(); }
		int cursorPos() { return _lcd->cursorPos(); }
		size_t end() { return _lcd->end(); }
		// Modifiers
		
		/// <summary>	
		/// '`' before or after a field inhibits space separating fields.
		/// '~' before a field starts it on a new line.
		/// </summary>
		bool hasAdded(const char * stream, CursorMode cursorMode, int cursorOffset, int endPos, ListStatus listStatus);
		void newLine();
		void reset() {_lcd->reset();}
		void truncate(int newEnd) { _lcd->setEnd(newEnd); }
		HardwareInterfaces::LCD_Display * _lcd;
	};
}