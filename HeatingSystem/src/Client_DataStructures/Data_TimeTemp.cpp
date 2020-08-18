#include "Data_TimeTemp.h"
#include "..\..\..\DateTime\src\Time_Only.h"
#include <Logging.h>

#ifdef ZPSIM
	#include <ostream>
#endif

namespace client_data_structures {
	using namespace LCD_UI;
	using namespace Date_Time;

	namespace { // restrict to local linkage
		TimeTemp_Interface timeTemp_UI;
	}

	//*************ReqIsTemp_Wrapper****************

	TimeTempWrapper::TimeTempWrapper(int16_t ttVal, ValRange valRangeArg) : I_Data_Formatter(ttVal, valRangeArg) {}
	TimeTempWrapper::TimeTempWrapper(int16_t ttVal) : I_Data_Formatter(ttVal, { e_editAll, 0, uint16_t(-1), 0, Edit_TimeTemp_h::e_NO_OF_EDIT_POS }) {}

	I_Streaming_Tool & TimeTempWrapper::ui() { return timeTemp_UI; }

	//*************Edit_ProfileDays_h****************

	bool Edit_TimeTemp_h::move_focus_by(int moveBy) { // Change character at the cursor. Modify copy held by currValue
		//enum {e_hours, e_10Mins, e_am_pm, e_10Temp, e_temp, e_NO_OF_EDIT_POS };

		if (moveBy == 0) return false;
		TimeTemp tt{ currValue().val };
		auto time = tt.time();
		auto temp = tt.temp();
		switch (focusIndex()) {
		case e_hours:
			time.setHrs(time.hrs() + moveBy);
			break;
		case e_10Mins:
			time.setMins10(time.mins10() + moveBy);
			break;
		case e_am_pm:
			time.addClockTime(moveBy * 12 * 6);
			break;
		case e_10Temp:
			temp += moveBy * 10;
			break;
		case e_temp:
			temp += moveBy;
			break;
		}
		tt.setTemp(temp);
		tt.setTime(time);
		currValue().val = uint16_t(tt);
		setInRangeValue();
		return true;
	}

	int Edit_TimeTemp_h::cursorFromFocus(int focusIndex) {
		// 0810a18
		int cursorPos = 0;
		switch (focusIndex) {
		case e_hours:	cursorPos = 1; break;
		case e_10Mins: cursorPos = 3; break;
		case e_am_pm: cursorPos = 4; break;
		case e_10Temp: cursorPos = 5; break;
		case e_temp: cursorPos = 6; break;
		default: cursorPos = 0;
		};

		currValue().valRange._cursorPos = cursorPos;
		return focusIndex;
	}

	int Edit_TimeTemp_h::gotFocus(const I_Data_Formatter * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(e_temp); // initial cursorPos when selected (not in edit)
		return e_temp;
	}

	//*************ProfileDays_Interface****************

	const char * TimeTemp_Interface::streamData(bool isActiveElement) const {
		TimeTemp tt{ getData(isActiveElement) };
		//logger() << F("TimeTemp : ") << tt << F(" IsActive: ") << isActiveElement << L_endl;
		auto time = tt.time();
		auto temp = tt.temp();
		strcpy(scratch, intToString(time.displayHrs(), 2, '0'));
		strcat(scratch, intToString(time.mins10() * 10, 2, '0'));
		strcat(scratch, time.isPM() ? "p" : "a");
		strcat(scratch, intToString(temp, 2, '0'));
		return scratch;
	}

	//*************Dataset_ProfileDays****************
	Dataset_TimeTemp::Dataset_TimeTemp(Query & query, VolatileData * runtimeData, I_Record_Interface * profile)
		: Record_Interface(query, runtimeData, profile)
		, _timeTemp(0, ValRange(e_editAll, 0, uint16_t(-1), 0, Edit_TimeTemp_h::e_NO_OF_EDIT_POS))
	{}

	I_Data_Formatter * Dataset_TimeTemp::getField(int fieldID) {
		if (recordID() == -1) return 0;
		switch (fieldID) {
		case e_TimeTemp:
			_timeTemp = uint16_t(record().rec().time_temp);
			return &_timeTemp;
		default: return 0;
		}
	}

	bool Dataset_TimeTemp::setNewValue(int fieldID, const I_Data_Formatter * newValue) {
		// Every Profile needs at least one TT
		switch (fieldID) {
		case e_TimeTemp: {
			record().rec().time_temp = (uint16_t)newValue->val;
			auto newRecordID = record().update();			
			if (newRecordID != recordID()) {
				setRecordID(newRecordID);
				return true;
			}
			break;
		}
		default:;
		}
		return false;
	}

	void Dataset_TimeTemp::insertNewData() {
		record() = query().insert(record().rec());
		setRecordID(record().id());
	}

}