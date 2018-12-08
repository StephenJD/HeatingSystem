#include "UI_DisplayBuffer.h"
//#include <cstring>

namespace LCD_UI {

	void UI_DisplayBuffer::newLine() {
		strcat(_lcd->buff(), "~");
	}

	bool UI_DisplayBuffer::hasAdded(const char * stream
		, CursorMode cursorMode, int cursorOffset, int endPos, ListStatus listStatus) {
		// stream points to a single field
		// spaces are inserted where necessary to avoid fields splitting across lines.
		// check line ends, and max length. Insert < > if required.
		if (stream == 0) return true;
		if (endPos == 0) endPos = _lcd->size(); else endPos;
		auto buffer = _lcd->buff();
		int originalEnd = strlen(buffer) -1; // index of current null
		bool onNewLine = (buffer[originalEnd] == '~');
		bool hasShortListChar = (buffer[originalEnd] == '>');
		if (onNewLine) 
			--originalEnd;
		int appendPos = originalEnd + 1;
		int addLen = strlen(stream);
		////////////////////////////////////////////////////////////////////
		//************* Lambdas evaluating algorithm *********************//
		////////////////////////////////////////////////////////////////////
		auto insertSpaces = [buffer](auto noOf , auto appendPos) -> decltype(appendPos)  {
			while (noOf > 0) { buffer[appendPos] = ' '; ++appendPos; --noOf; }
			return appendPos;
		};
		auto notAtStartOfLine = [this](auto appendPos) -> bool { return appendPos % _lcd->cols() != 0; };
		auto spaceFor_char = [endPos](auto appendPos) -> bool {return appendPos < endPos; };
		auto inserted_a_fieldSeparator = [onNewLine,buffer,addLen,this,insertSpaces] (auto appendPos) -> decltype(appendPos)  {
			buffer[appendPos] = ' ';
			auto spacesToEndOfLine = _lcd->cols() - (appendPos % _lcd->cols()) -1;
			++appendPos;
			if (onNewLine) {
				appendPos = insertSpaces(spacesToEndOfLine, appendPos);
			}
			else if (spacesToEndOfLine < addLen) {
				appendPos = insertSpaces(spacesToEndOfLine, appendPos);
			}
			return appendPos;
		};

		auto inserted_a_list_start_char = [buffer](auto appendPos) -> decltype(appendPos) {
			buffer[appendPos] = '<';
			++appendPos;
			return appendPos;
		};

		auto nextFieldFits = [listStatus, addLen, endPos](auto appendPos) -> bool {
			return (appendPos + addLen  + (listStatus & e_not_showingEnd ? 1 : 0)) <= endPos; 
		};

		auto setCursor = [cursorMode, cursorOffset, this](auto appendPos) -> void {
			_lcd->setCursorMode(cursorMode);
			_lcd->setCursorPos(appendPos + cursorOffset); 
		};

		auto appendField = [stream, buffer](auto appendPos) -> decltype(appendPos) {
			int i = 0;
			while (stream[i] != 0) {
				buffer[appendPos] = stream[i];
				++appendPos;
				++i;
			}
			buffer[appendPos] = 0;
			return appendPos;
		};

		auto terminate_with_EndList_char_at = [buffer](int charPos) -> void {
			buffer[charPos] = '>';
			buffer[charPos + 1] = 0;
		};

		////////////////////////////////////////////////////////////////////
		//************************  Algorithm ****************************//
		////////////////////////////////////////////////////////////////////
		if (notAtStartOfLine(appendPos) && spaceFor_char(appendPos)) {
			appendPos = inserted_a_fieldSeparator(appendPos);
		}
		if (listStatus & e_notShowingStart && spaceFor_char(appendPos)) {
			appendPos = inserted_a_list_start_char(appendPos);
		}
		if (nextFieldFits(appendPos)) {
			if (cursorMode != CursorMode::e_unselected) {
				setCursor(appendPos);
			}
			appendPos = appendField(appendPos);
			if (listStatus & e_this_not_showing) {
				terminate_with_EndList_char_at(appendPos);
			}
		}
		else {
			if (!(listStatus & e_this_not_showing) && spaceFor_char(originalEnd + 1)) {
				terminate_with_EndList_char_at(originalEnd + 1);
			} else buffer[originalEnd + 1] = 0;
			return false;
		}
		return true;
	}
}