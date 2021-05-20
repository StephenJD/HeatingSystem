#include "Data_CurrentTime.h"
#include "Conversions.h"
#include <Clock.h>
#include <SD.h>

using namespace arduino_logger;

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
		: I_Data_Formatter(dateVal.asInt(), valRangeArg) {
	}

	I_Streaming_Tool & CurrentTimeWrapper::ui() { return currentTime_UI; }

	bool Edit_CurrentTime_h::upDown(int moveBy, Behaviour ud_behaviour) { // Change character at the cursor. Modify copy held by currValue
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
		// 08:10:00am
		int cursorPos = 0;
		switch (focusIndex) {
		case e_hours:	cursorPos = 1; break;
		case e_10Mins: cursorPos = 3; break;
		case e_mins: cursorPos = 4; break;
		case e_am_pm: cursorPos = 9; break;
		case e_time: cursorPos = 9; break;
		default: cursorPos = 0;
		};

		currValue().valRange._cursorPos = cursorPos;
		return focusIndex;
	}

	int Edit_CurrentTime_h::gotFocus(const I_Data_Formatter * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(e_time); // initial cursorPos when selected (not in edit)
		return e_mins;
	}

	const char * CurrentTime_Interface::streamData(bool isActiveElement) const {
		auto dt = DateTime(getData(isActiveElement));
		//if (editItem().backUI() == 0) dt = DateTime{ clock_() };

		strcpy(scratch, intToString(dt.displayHrs(), 2));
		strcat(scratch, ":");
		strcat(scratch, intToString(dt.mins10(), 1));
		strcat(scratch, intToString(clock_().minUnits(), 1));
		strcat(scratch, ":");
		strcat(scratch, intToString(clock_().seconds(), 2));
		strcat(scratch, (dt.isPM() ? "pm" : "am"));
		return scratch;
	}


	//***************************************************
	//              CurrentDate UI Edit
	//***************************************************

	CurrentDateWrapper::CurrentDateWrapper(DateOnly dateVal, ValRange valRangeArg)
		: I_Data_Formatter(dateVal.asInt(), valRangeArg) {
	}

	I_Streaming_Tool & CurrentDateWrapper::ui() { return currentDate_UI; }

	bool Edit_CurrentDate_h::upDown(int moveBy, Behaviour ud_behaviour) { // Change character at the cursor. Modify copy held by currValue
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

	int Edit_CurrentDate_h::gotFocus(const I_Data_Formatter * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(e_date); // initial cursorPos when selected (not in edit)
		return e_yrs;
	}

	const char * CurrentDate_Interface::streamData(bool isActiveElement) const {
		auto dt = DateOnly(getData(isActiveElement));
		strcpy(scratch, dt.getDayStr());
		strcat(scratch, " ");
		strcat(scratch, intToString(dt.day(), 2));
		strcat(scratch, "/");
		strcat(scratch, dt.getMonthStr());
		strcat(scratch, "/20");
		strcat(scratch, intToString(dt.year(), 2));
		return scratch;
	}

	//***************************************************
	//              Dataset_WithoutQuery Interface
	//***************************************************

	RecInt_CurrDateTime::RecInt_CurrDateTime()
		: _currTime{ clock_().time(), ValRange(e_fixedWidth | e_editAll,0,uint16_t(-1),0,e_time) }
		, _currDate{ clock_().date(), ValRange(e_fixedWidth | e_editAll, 0, uint16_t(-1),0,e_date)}
		, _dst{ clock_().autoDSThours(),ValRange(e_edOneShort,0,2)}
		, _SDCard{"",5}
	{}

	//I_Data_Formatter * Dataset_WithoutQuery::getFieldAt(int fieldID, int elementIndex) {
	//	return getField(fieldID);
	//}

	I_Data_Formatter * RecInt_CurrDateTime::getField(int fieldID) {
		_currTime.val = clock_().time().asInt();
		_currDate.val = clock_().date().asInt();
		_dst.val = clock_().autoDSThours();
		_SDCard = SD.sd_exists(53, 100) ? "SD OK" : "No SD";
		switch (fieldID) {
		case e_currTime: return &_currTime;
		case e_currDate: return &_currDate;
		case e_dst: return &_dst;
		case e_sdcard: return &_SDCard;
		default: return 0;
		}
	}

	bool RecInt_CurrDateTime::setNewValue(int fieldID, const I_Data_Formatter * newValue) {
		switch (fieldID) {
		case e_currTime:
			// Note: minute units are written to clock_() as they are edited.
			_currTime.val = newValue->val;
			logger() << L_time << F("Save new Time: ") << TimeOnly(_currTime.val) << L_endl;
			clock_().setTime(TimeOnly( _currTime.val ));
			break;
		case e_currDate:
			_currDate.val = newValue->val;
			logger() << L_time << F("Save new Date: ") << DateOnly(_currDate.val) << L_endl;
			clock_().setDate(DateOnly(_currDate.val ));
			break;
		case e_dst:
			_dst.val = newValue->val;
			clock_().setAutoDSThours(uint8_t(newValue->val));
			break;
		}
		return false;
	}

}