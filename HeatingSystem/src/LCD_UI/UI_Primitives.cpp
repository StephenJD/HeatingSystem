#include "UI_Primitives.h"
#include "Conversions.h"
#include "UI_FieldData.h"
#include "I_Record_Interface.h"

#ifdef ZPSIM
	#include <iostream>
	using namespace std;
#endif

namespace LCD_UI {
	using namespace GP_LIB;

	namespace { // restrict to local linkage
		Int_Interface int_UI;
		Decimal_Interface dec_UI;
		String_Interface string_UI;
	}

	//OneVal Permitted_Vals::_oneVal(0);


	char I_Field_Interface::scratch[81]; // Buffer for constructing strings of wrapped values

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 Permitted Vals 
	/////////////////////////////////////////////////////////////////////////////////////////

	Collection_Hndl * Permitted_Vals::select(Collection_Hndl * from) {
		return from->on_select()->on_select();
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 Edit Ints 
	/////////////////////////////////////////////////////////////////////////////////////////

	Edit_Int_h::Edit_Int_h() : I_Edit_Hndl(&editVal) {
#ifdef ZPSIM
		logger() << F("\tEdit_Int_h Addr: ") << L_hex << long(this) << L_endl;
		logger() << F("\tEdit_Int_h Edit Addr: ") << L_hex << long(&editVal) << L_endl;
		ui_Objects()[(long)get()] = "Edit_Int";
#endif
	}

	int Edit_Int_h::gotFocus(const I_UI_Wrapper * data) { // returns edit focus
		if (data) _currValue = static_cast<const IntWrapper &>(*data);
		_decade = 1;
		cursorFromFocus(); //Sets CursorPos.
		return _currValue.valRange.editablePlaces - 1;
	}

	int Edit_Int_h::cursorFromFocus(int focus) { // focus passed through but not used. Sets CursorPos
		auto & original = _currValue.valRange;
		// Lambdas
		auto focusIsAfterDP = [original](int focus, int spaceForDigits)->int {
			if (original.formatIs(e_editAll)) {
				if (spaceForDigits - focus <= original.noOfDecPlaces) return 1;
			} else if (original.noOfDecPlaces > 0) return 1;
			return 0;
		};

		auto spaceForSign = [original](int val) -> int {
			if (original.formatIs(e_showSign) || val < 0) return 1; else return 0;
		};

		// Algorithm
		int digitPlaces;
		if (original.formatIs(e_fixedWidth)) {
			digitPlaces = original.editablePlaces;
		}
		else {
			digitPlaces = original.spaceForDigits(_currValue.val);
			original.editablePlaces = digitPlaces;
		}

		if (backUI()) {
			focus = focusIndex();
		} else if (original.formatIs(e_editAll)) {
			focus = digitPlaces-1;
		} else focus = 0;
		
		int extras = spaceForSign(_currValue.val) + focusIsAfterDP(focus,digitPlaces);
		
		original._cursorPos = focus + extras;
		return focus;
	}

	void Edit_Int_h::setInRangeValue() {
		I_Edit_Hndl::setInRangeValue();
		// Adjust focus if number of digit-spaces has changed
		auto & original = _currValue.valRange;
		if (original.formatIs(e_edOneShort) || original.formatIs(e_fixedWidth)) return;

		int digitPlaces = original.spaceForDigits(_currValue.val);
		int currentWidth = backUI()->get()->collection()->endIndex();

		int moveFocusBy = digitPlaces - currentWidth;
		if (moveFocusBy != 0) {
			backUI()->get()->collection()->setCount(digitPlaces);
			backUI()->move_focus_by(moveFocusBy);
			cursorFromFocus();
		}
	}

	void Edit_Int_h::setDecade(int focus) {
		int decade = 0;
		decade = decade + _currValue.valRange.spaceForDigits(_currValue.val) - focus - 1;
		_decade = power10(decade);
	}

	bool Edit_Int_h::move_focus_by(int moveBy) { // Change character at the cursor. Modify copy held by Edit_Int_h
		if (moveBy == 0) return false;
		auto & retInt = _currValue.val;
		auto cursorPos = focusIndex();
		auto increment = moveBy * _decade;
		bool negVal = retInt < 0;
		bool negIncrement = increment < 0;
		if (negVal != negIncrement && abs(increment) > abs(retInt)) {
			// This ensures that a sign-change retains the other significant digits as they are
			retInt = increment - retInt;
		}
		else retInt = increment + retInt;
		setInRangeValue();
		return true;
	}

	void Int_Interface::haveMovedTo(int currFocus) {  // move focus to next digit during edit
		static_cast<Edit_Int_h &>(editItem()).setDecade(currFocus);
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 Int & Dec Interface 
	/////////////////////////////////////////////////////////////////////////////////////////

	const char * Int_Interface::streamData(bool isActiveElement) const {
		strcpy(scratch,intToString(getData(isActiveElement), _wrapper->valRange.editablePlaces,'0',_wrapper->valRange.format));
		return scratch;
	}

	const char * Decimal_Interface::streamData(bool isActiveElement) const {
		strcpy(scratch,decToString(getData(isActiveElement),_wrapper->valRange.editablePlaces, _wrapper->valRange.noOfDecPlaces,_wrapper->valRange.format));
		return scratch;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 Edit Char 
	/////////////////////////////////////////////////////////////////////////////////////////

	Edit_Char_h::Edit_Char_h() : I_Edit_Hndl(&editChar) {
#ifdef ZPSIM
		logger() << F("\tEdit_Char_h Addr:") << L_hex << long(this) << L_endl;
		logger() << F("\teditChar Addr:") << L_hex << long(&editChar) << L_endl;
		ui_Objects()[(long)get()] = "Edit_Char";
#endif
	}

	int Edit_Char_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
		const StrWrapper * strWrapper(static_cast<const StrWrapper *>(data));
		if(strWrapper) _currValue = *strWrapper;
		auto strEnd = static_cast<uint8_t>(strlen(_currValue.str())) - 1;
		while (_currValue.str()[strEnd] == ' ') --strEnd;
		_currValue.valRange.noOfDecPlaces = strEnd + 1; // Current Str length
		_currValue.valRange._cursorPos = strEnd; // initial cursorPos when selected (not in edit)
		return 0;
	}

	int Edit_Char_h::editMemberSelection(const I_UI_Wrapper * data, int recordID) { // returns initial edit focus
		const StrWrapper * strWrapper(static_cast<const StrWrapper *>(data));
		if (strWrapper) _currValue = *strWrapper;
		_currValue.val = recordID;
		auto strEnd = static_cast<uint8_t>(strlen(_currValue.str())) - 1;
		while (_currValue.str()[strEnd] == ' ') --strEnd;
		_currValue.valRange.noOfDecPlaces = strEnd + 1; // Current Str length
		_currValue.valRange._cursorPos = strEnd; // initial cursorPos when selected (not in edit)
		return 0;
	}

	int Edit_Char_h::getEditCursorPos() { // copy data to edit
		strcpy(stream_edited_copy, _currValue.str());
		uint8_t currWidth = _currValue.valRange.noOfDecPlaces;
		for (int i = currWidth; i < _currValue.valRange.editablePlaces; ++i) {
			stream_edited_copy[i] = '|';
			_currValue.str()[i] = 0;
		}
		stream_edited_copy[_currValue.valRange.editablePlaces] = 0;
		_currValue.str()[_currValue.valRange.editablePlaces] = 0;
		return cursorFromFocus(focusIndex());
	}

	bool Edit_Char_h::move_focus_by(int move) { // Change character at the cursor. Modify the copy held by Edit_Char_h
		if (move == 0) return false;
		if (backUI()->behaviour().is_Editable()) {
			char * editSr = _currValue.str();
			char * streamStr = stream_edited_copy;
			int cursorPos = focusIndex();
			char _thisChar = streamStr[cursorPos];
			static int lastcursorOffset(0);
			auto & currStrLength = _currValue.valRange.noOfDecPlaces;

			if (move > 0 && (_thisChar >= 'a' && _thisChar <= 'z')) // change to upper case
				_thisChar = _thisChar - ('a' - 'A');
			else if (move < 0 && (_thisChar >= 'A' && _thisChar <= 'Z')) // change to lower case
				_thisChar = _thisChar + ('a' - 'A');
			else if (_thisChar == '|') {
				if (cursorPos != lastcursorOffset) { // start with previous character when just moved to the right
					if (cursorPos > 0) {
						_thisChar = streamStr[cursorPos - 1];
						if (_thisChar == '|') {
							_thisChar = ' ' + move;
						}
					}
					else _thisChar = ' ' + move;
				}
				else {
					_thisChar = _thisChar + move;
					// going past the | character, restore streamStr to previous characters past |
					int i = currStrLength + 1;
					while (streamStr[i] == '|'  && editSr[i] != 0) {
						streamStr[i] = editSr[i];
						++i;
						currStrLength = i;
					}
				}
			}
			else _thisChar = _thisChar + move;
			lastcursorOffset = cursorPos;

			// {-/0123456789| ABCDEFGHIJKLMNOPQRSTUVWXYZ}
			switch (_thisChar) {
			case 31:
			case ':':
				_thisChar = '|';
				break;
			case ',':
				_thisChar = 'z';
				break;
			case '[':
				_thisChar = '-';
				break;
			case '.':
				if (move > 0) _thisChar = '/'; else _thisChar = '-';
				break;
			case '{':
				if (move > 0) _thisChar = '-'; else _thisChar = '9';
				break;
			case '}':
			case 96:
			case '@':
				_thisChar = ' ';
				break;
			case '!':
				_thisChar = 'A';
			}

			if (_thisChar == '|' && cursorPos < currStrLength) {
				// shorten string
				for (int i = cursorPos; i < currStrLength; ++i) {
					streamStr[i] = '|';
				}
				currStrLength = cursorPos;
			}
			else {
				if (currStrLength <= cursorPos) { // may lengthen string
					for (int i = currStrLength; i < cursorPos; ++i) {
						editSr[i] = ' ';
						streamStr[i] = ' ';
					}
					currStrLength = cursorPos + 1;
				}
			}
			editSr[cursorPos] = _thisChar;
			editSr[currStrLength] = 0;
			streamStr[cursorPos] = _thisChar;
		}
		else {
			Field_Interface_h * fldInt_h = static_cast<Field_Interface_h *>(backUI());
			auto status = fldInt_h->getData()->data()->move_by(move);
			if (status != TB_OK && fldInt_h->behaviour().is_recycle_on_next()) {
				if (status == TB_BEFORE_BEGIN) 
					fldInt_h->getData()->data()->last();
				else fldInt_h->getData()->data()->move_to(0);
				//fldInt_h->getData()->data()->setRecordID(fldInt_h->getData()->data()->recordID());
			}
			_currValue.val = fldInt_h->getData()->data()->record().id();
			return fldInt_h->getData()->data()->record().status() == TB_OK;
		}
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 String Interface
	/////////////////////////////////////////////////////////////////////////////////////////
	StrWrapper::StrWrapper(const char* strVal, ValRange valRangeArg)
		: I_UI_Wrapper(0, valRangeArg) {
		strcpy(_str, strVal);
		auto lenStr = static_cast<uint8_t>(strlen(strVal));
		valRange._cursorPos = lenStr - 1;
		for (int i = lenStr; i < valRangeArg.editablePlaces; ++i) {
			_str[i] = ' ';
		}
		_str[valRangeArg.editablePlaces] = 0;
#ifdef ZPSIM
		ui_Objects()[(long)this] = "StrWrapper";
#endif
	}

	StrWrapper & StrWrapper::operator= (const char* strVal) {
		strcpy(_str, strVal);
		auto lenStr = static_cast<uint8_t>(strlen(strVal));
		valRange._cursorPos = lenStr - 1;
		for (int i = lenStr; i < valRange.editablePlaces; ++i) {
			_str[i] = ' ';
		}
		_str[valRange.editablePlaces] = 0;
		return *this;
	}

	const char * String_Interface::streamData(bool isActiveElement) const {
		if (_wrapper == 0) return 0;
		const StrWrapper * strWrapper(static_cast<const StrWrapper *>(_wrapper));

		if (isActiveElement) {
			auto fieldInterface_h = parent();
			if (fieldInterface_h && fieldInterface_h->cursor_Mode() == HardwareInterfaces::LCD_Display::e_inEdit) { // any item may be in edit
				if (fieldInterface_h->editBehaviour().is_Editable()) {
					return _editItem.stream_edited_copy;
				}
			}
		} 
		return strWrapper->str();
	}

	I_Field_Interface & IntWrapper::ui() { return int_UI; }
	I_Field_Interface & DecWrapper::ui() {	return dec_UI; }
	I_Field_Interface & StrWrapper::ui() { return string_UI; }
}

