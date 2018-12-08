#pragma once
#include "..\HardwareInterfaces\DisplayBuffer.h"

namespace LCD_UI {

	using CursorMode = HardwareInterfaces::DisplayBuffer_I::CursorMode;

	class UI_DisplayBuffer { // cannot derive from DisplayBuffer_I, because it needs to be a polymorphic pointer
	public:
		enum ListStatus { e_showingAll, e_notShowingStart, e_not_showingEnd = e_notShowingStart * 2, e_this_not_showing = e_not_showingEnd * 2 };
		
		UI_DisplayBuffer(HardwareInterfaces::DisplayBuffer_I & displayBuffer) : _lcd(&displayBuffer) {}
		
		//Queries
		const char * toCStr() const { return _lcd->buff(); }
		CursorMode cursorMode() { return _lcd->cursorMode(); }
		int cursorPos() { return _lcd->cursorPos(); }
		// Modifiers
		bool hasAdded(const char * stream, CursorMode cursorMode, int cursorOffset, int endPos, ListStatus listStatus);
		void newLine();
		void reset() {_lcd->reset();}
		HardwareInterfaces::DisplayBuffer_I * _lcd;
	};
}