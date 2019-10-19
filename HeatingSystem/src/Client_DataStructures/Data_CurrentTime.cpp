#include "Data_CurrentTime.h"
#include "..\..\..\Conversions\Conversions.h"
#include <Clock.h>

namespace client_data_structures {
	using namespace HardwareInterfaces;
	using namespace GP_LIB;

	namespace { // restrict to local linkage
		CurrentTime_Interface currentTime_UI{};
		CurrentDate_Interface currentDate_UI{};
		enum { e_hours, e_10Mins, e_mins, e_am_pm, e_time };
		enum { e_10Days, e_day, e_month, e_10Yrs, e_yrs, e_date };
	}

	//***************************************************
	//              CurrentTime UI Edit
	//***************************************************

	CurrentTimeWrapper::CurrentTimeWrapper(TimeOnly dateVal, ValRange valRangeArg)
		: I_UI_Wrapper(dateVal.asInt(), valRangeArg) {
	}

	I_Field_Interface & CurrentTimeWrapper::ui() { return currentTime_UI; }

	bool Edit_CurrentTime_h::move_focus_by(int moveBy) { // Change character at the cursor. Modify copy held by currValue
		if (moveBy == 0) return false;
		DateTime newDate(TimeOnly(currValue().val));
		switch (focusIndex()) {
		case e_hours:	newDate.addOffset_NoRollover({ hh, moveBy }, DateTime{ 0 });
			break;
		case e_10Mins: newDate.addOffset_NoRollover({ m10,moveBy }, DateTime{ 0 });
			break;
		case e_mins: clock_().setMinUnits(nextIndex(0,clock_().minUnits(),9,moveBy,false ));
			break;
		case e_am_pm: newDate.addOffset_NoRollover({ hh, moveBy * 12 }, DateTime{ 0 });
			break;
		}
		currValue().val = newDate.time().asInt();
		setInRangeValue();
		return true;
	}

	int Edit_CurrentTime_h::cursorFromFocus(int focusIndex) {
		// 08:10:00 am
		int cursorPos = 0;
		switch (focusIndex) {
		case e_hours:	cursorPos = 1; break;
		case e_10Mins: cursorPos = 3; break;
		case e_mins: cursorPos = 4; break;
		case e_am_pm: cursorPos = 9; break;
		case e_time: cursorPos = 10; break;
		default: cursorPos = 0;
		};

		currValue().valRange._cursorPos = cursorPos;
		return focusIndex;
	}

	int Edit_CurrentTime_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(e_time); // initial cursorPos when selected (not in edit)
		return e_mins;
	}

	const char * CurrentTime_Interface::streamData(bool isActiveElement) const {
		clock_().refresh();
		auto dt = DateTime(getData(isActiveElement));
		if (editItem().backUI() == 0) dt = DateTime{ clock_() };

		strcpy(scratch, intToString(dt.displayHrs(), 2));
		strcat(scratch, ":");
		strcat(scratch, intToString(dt.mins10(), 1));
		strcat(scratch, intToString(clock_().minUnits(), 1));
		strcat(scratch, ":");
		strcat(scratch, intToString(clock_().seconds(), 2));
		strcat(scratch, (dt.hrs() < 12) ? " am" : " pm");
		return scratch;
	}


	//***************************************************
	//              CurrentDate UI Edit
	//***************************************************

	CurrentDateWrapper::CurrentDateWrapper(DateOnly dateVal, ValRange valRangeArg)
		: I_UI_Wrapper(dateVal.asInt(), valRangeArg) {
	}

	I_Field_Interface & CurrentDateWrapper::ui() { return currentDate_UI; }

	bool Edit_CurrentDate_h::move_focus_by(int moveBy) { // Change character at the cursor. Modify copy held by currValue
		if (moveBy == 0) return false;
		DateTime newDate(DateOnly( currValue().val ));
		switch (focusIndex()) {
		case e_10Days:	newDate.addOffset_NoRollover({ dd, moveBy * 10 }, DateTime{ 0 });
			break;
		case e_day:	newDate.addOffset_NoRollover({ dd, moveBy }, DateTime{ 0 });
			break;
		case e_month: newDate.addOffset_NoRollover({ mm, moveBy }, DateTime{ 0 });
			break;
		case e_10Yrs: newDate.addOffset_NoRollover({ yy, moveBy * 10 }, DateTime{ 0 });
			break;
		case e_yrs: 
			newDate.addOffset_NoRollover({ yy, moveBy }, DateTime{ 0 });
			break;
		}
		currValue().val = newDate.date().asInt();
		setInRangeValue();
		return true;
	}

	int Edit_CurrentDate_h::cursorFromFocus(int focusIndex) {
		// Wed 12-Feb-2018
		int cursorPos = 0;
		switch (focusIndex) {
		case e_10Days:	cursorPos = 4; break;
		case e_day:	cursorPos = 5; break;
		case e_month: cursorPos = 7; break;
		case e_10Yrs: cursorPos = 13; break;
		case e_yrs: cursorPos = 14; break;
		case e_date: cursorPos = 14; break;
		default: cursorPos = 0;
		};

		currValue().valRange._cursorPos = cursorPos;
		return focusIndex;
	}

	int Edit_CurrentDate_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(e_date); // initial cursorPos when selected (not in edit)
		return e_yrs;
	}

	const char * CurrentDate_Interface::streamData(bool isActiveElement) const {
		auto dt = DateOnly(getData(isActiveElement));
		strcpy(scratch, dt.getDayStr());
		strcat(scratch, " ");
		strcat(scratch, intToString(dt.day(), 2));
		strcat(scratch, "-");
		strcat(scratch, dt.getMonthStr());
		strcat(scratch, "-20");
		strcat(scratch, intToString(dt.year(), 2));
		return scratch;
	}

	//***************************************************
	//              Dataset_WithoutQuery Interface
	//***************************************************

	Dataset_WithoutQuery::Dataset_WithoutQuery()
		: Record_Interface{ nullQuery,0,0 }
		, _currTime{ clock_().time(), ValRange(e_fixedWidth | e_editAll,0,uint16_t(-1),0,e_time) }
		, _currDate{ clock_().date(), ValRange(e_fixedWidth | e_editAll, 0, uint16_t(-1),0,e_date)}
		, _dst{ clock_().autoDSThours(),ValRange(e_edOneShort,0,2)}
	{}

	I_UI_Wrapper * Dataset_WithoutQuery::getFieldAt(int fieldID, int elementIndex) {
		return getField(fieldID);
	}

	I_UI_Wrapper * Dataset_WithoutQuery::getField(int fieldID) {
		switch (fieldID) {
		case e_currTime: return &_currTime;
		case e_currDate: return &_currDate;
		case e_dst: return &_dst;
		default: return 0;
		}
	}

	bool Dataset_WithoutQuery::setNewValue(int fieldID, const I_UI_Wrapper * newValue) {
		switch (fieldID) {
		case e_currTime:
			_currTime.val = newValue->val;
			clock_().setTime(TimeOnly( _currTime.val ));
			break;
		case e_currDate:
			_currDate.val = newValue->val;
			clock_().setDate(DateOnly( _currTime.val ));
			break;
		case e_dst:
			_dst.val = newValue->val;
			clock_().setAutoDSThours(uint8_t(newValue->val));
			break;
		}
		return false;
	}

}