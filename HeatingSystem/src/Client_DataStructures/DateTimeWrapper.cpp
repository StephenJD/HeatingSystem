#include "DateTimeWrapper.h"
#include "Conversions.h"
#include "..\HardwareInterfaces\Clock.h"

#ifdef ZPSIM
	#include <iostream>
#endif


namespace client_data_structures {
	using namespace HardwareInterfaces;
	using namespace GP_LIB;

	namespace { // restrict to local linkage
		DateTime_Interface datetime_UI;
	}

	//***************************************************
	//              DateTime UI Edit
	//***************************************************

	DateTimeWrapper::DateTimeWrapper(DateTime dateVal, ValRange valRangeArg)
		: I_UI_Wrapper(dateVal.asInt(), valRangeArg) {
#ifdef ZPSIM
		std::cout << "DateTimeWrapper at: " << (long long)(this) << std::endl;
#endif
	}

	I_Field_Interface & DateTimeWrapper::ui() { return datetime_UI; }

	Edit_DateTime_h::Edit_DateTime_h() : 
		I_Edit_Hndl(&editVal) { 
#ifdef ZPSIM
		std::cout << "Edit_DateTime_h at: " << std::hex << (long long)this << std::endl; 
		std::cout << "   Edit Copy DateTimeWrapper at: " << (long long)(&_currValue) << std::endl;
		std::cout << "   Edit Copy Permitted_Vals at: " << (long long)(&editVal) << std::endl;
#endif
	}

	bool Edit_DateTime_h::move_focus_by(int moveBy) { // Change character at the cursor. Modify copy held by currValue
		if (moveBy == 0) return false;
		auto now = DateTime{ clock_() };
		DateTime newDate(currValue().val);
		switch (focusIndex()) {
		//enum {e_hours, e_10Mins, e_am_pm, e_10Days, e_day, e_month, e_NO_OF_EDIT_POS };

		case e_hours:	newDate.addOffset_NoRollover({ hh, moveBy}, now);
			break;
		case e_10Mins: newDate.addOffset_NoRollover({ m10,moveBy}, now);
			break;
		case e_am_pm: newDate.addOffset_NoRollover({ hh, moveBy * 12 }, now);
			break;
		case e_10Days:	newDate.addOffset_NoRollover({ dd, moveBy * 10 }, now);
			break;
		case e_day:	newDate.addOffset_NoRollover({ dd, moveBy }, now);
			break;
		case e_month:
				if (currValue().valRange._cursorPos < 10) newDate.addOffset_NoRollover({ dd, moveBy }, now);
				else newDate.addOffset_NoRollover({ mm, moveBy }, now);
				break;
		}
		currValue().val = newDate.asInt();
		setInRangeValue();
		backUI()->set_focus(cursorFromFocus(focusIndex()));
		return true;
	}

	int Edit_DateTime_h::cursorFromFocus(int focusIndex) {
		DateTime editDate = static_cast<const DateTime_Interface &>(_currValue.ui()).valAsDate();
		auto now = DateTime{ clock_() };
		//enum {e_hours, e_10Mins, e_am_pm, e_10Days, e_day, e_month, e_NO_OF_EDIT_POS };

		// debug
		//auto month = now.month();
		//auto day = now.day();
		//auto yr = now.year();
		//auto edmonth = editDate.month();
		//auto edday = editDate.day();
		//auto edyr = editDate.year();
		// end debug

		bool isTodayOrTomorrow = editDate.date() >= now.date() && editDate.date() <= now.date() + 1;
		bool isJudgementDay = editDate.date() == JUDGEMEMT_DAY;
		// 08:10am 31Aug
		// 08:10am Today
		// 08:10am Tomor'w
		// Now
		// Judgement Day
		int cursorPos = 0;
		if (backUI() == 0) { // not in edit
			focusIndex = e_day;
			if (editDate <= now) { cursorPos = 2; focusIndex = e_month; }
			else if (isJudgementDay) { cursorPos = 12; }
			else if (editDate.date() == now.date()) { cursorPos = 12; }
			else if (isTodayOrTomorrow) { cursorPos = 14; }
			else cursorPos = 9;
		}
		else {
			if (editDate <= now || isJudgementDay) {
				cursorPos = 0; focusIndex = e_month;
			}
			else {
				if (isTodayOrTomorrow) {
					if (focusIndex == e_day && currValue().valRange._cursorPos == 8) // moving left
						focusIndex = e_am_pm;
					else if (focusIndex > e_am_pm)
						focusIndex = e_month;
				}
				else if (focusIndex == e_month && currValue().valRange._cursorPos == 8)
					focusIndex = e_day;
				switch (focusIndex) {
				case e_hours:	cursorPos = 1; break;
				case e_10Mins: cursorPos = 3; break;
				case e_am_pm: cursorPos = 5; break;
				case e_10Days:	cursorPos = 8; break;
				case e_day:	cursorPos = isTodayOrTomorrow ? 8 : 9; break;
				case e_month: cursorPos = isTodayOrTomorrow ? 8 : 10; break;
				default: cursorPos = 0;
				};
			}
		}
		currValue().valRange._cursorPos = cursorPos;

		return focusIndex;
	}

	int Edit_DateTime_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(e_month); // initial cursorPos when selected (not in edit)
		return e_day;
	}

	int Edit_DateTime_h::getEditCursorPos() {
		currValue().valRange.minVal = clock_().asInt(); // can't go earlier than now.
		return cursorFromFocus(focusIndex());
	}

	const char * DateTime_Interface::streamData(const Object_Hndl * activeElement) const {
		auto dt = DateTime(getData(activeElement));

		auto now = DateTime{ clock_() };
		if (now >= dt) {
			strcpy(scratch, "Now");
		}
		else {
			strcpy(scratch, intToString(dt.displayHrs(), 2));
			strcat(scratch, ":");
			strcat(scratch, intToString(dt.mins10() * 10, 2));
			strcat(scratch, (dt.hrs() < 12) ? "am" : "pm");
			DateTime gotDate = dt;
			if (gotDate.date() == now) {
				strcat(scratch, " Today");
			}
			else if (gotDate.date() == now + 1) {
				strcat(scratch, " Tomor'w");
			}
			else if (gotDate.date() == JUDGEMEMT_DAY) {
				strcpy(scratch, "Judgement Day");
			}
			else {
				strcat(scratch, " ");
				strcat(scratch, intToString(dt.day(), 2));
				strcat(scratch, dt.getMonthStr());
			}
			if (dt.year() != 127 && dt.year() > now.year()) strcat(scratch, intToString(dt.year(), 2));
		}
		return scratch;
	}
}