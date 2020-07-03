#include "Data_Profile.h"
#include "Data_Zone.h"
#include "Data_TimeTemp.h"
#include "..\..\..\DateTime\src\Time_Only.h"
#include "..\Assembly\HeatingSystemEnums.h"


namespace client_data_structures {
	using namespace LCD_UI;

	namespace { // restrict to local linkage
		ProfileDays_Interface profileDays_UI;
	}

	//*************ReqIsTemp_Wrapper****************

	ProfileDaysWrapper::ProfileDaysWrapper(int8_t daysVal, ValRange valRangeArg) : I_UI_Wrapper(daysVal, valRangeArg) {}
	ProfileDaysWrapper::ProfileDaysWrapper(int8_t daysVal) : I_UI_Wrapper(daysVal, { e_editAll, 0, 127, 0, 7 }) {}

	I_Field_Interface & ProfileDaysWrapper::ui() { return profileDays_UI; }

	//*************Edit_ProfileDays_h****************

	bool Edit_ProfileDays_h::move_focus_by(int moveBy) { // Change character at the cursor. Modify copy held by currValue
		if (moveBy == 0) return false;
		auto mask = uint32_t(64) >> focusIndex();
		currValue().val = (uint32_t(currValue().val) ^ mask);
		auto daysOnly = currValue().val & 0x7F;
		if (daysOnly == 0x7F) currValue().val = 0xFF;
		else currValue().val = daysOnly;
		setInRangeValue();
		return true;
	}

	int Edit_ProfileDays_h::cursorFromFocus(int focusIndex) {
		currValue().valRange._cursorPos = focusIndex;
		return focusIndex;
	}

	int Edit_ProfileDays_h::gotFocus(const I_UI_Wrapper * data) { // returns initial edit focus
		if (data) currValue() = *data;
		cursorFromFocus(6); // initial cursorPos when selected (not in edit)
		return Dataset_ProfileDays::firstIncludedDayPosition(currValue().val)+1;
	}

	//*************ProfileDays_Interface****************
	
	const char * ProfileDays_Interface::streamData(bool isActiveElement) const {
		constexpr char EDIT_SPC_CHAR[] = "-";
		int myDays = getData(isActiveElement);
		if (myDays & 64) strcpy(scratch, "M"); else strcpy(scratch, EDIT_SPC_CHAR);
		if (myDays & 32) strcat(scratch, "T"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 16) strcat(scratch, "W"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 8) strcat(scratch, "T"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 4) strcat(scratch, "F"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 2) strcat(scratch, "S"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 1) strcat(scratch, "S"); else strcat(scratch, EDIT_SPC_CHAR);
		return scratch;
	}

	bool ProfileDays_Interface::isActionableObjectAt(int index) const {
		auto firstDay = Dataset_ProfileDays::firstIncludedDayPosition(_editItem.currValue().val);
		if (index <= firstDay) return false;
		return true;
	}

	//*************Dataset_ProfileDays****************
	Dataset_ProfileDays::Dataset_ProfileDays(Query & query, VolatileData * runtimeData, I_Record_Interface * dwellProgs, I_Record_Interface * dwellZone)
		: Record_Interface(query, runtimeData, dwellProgs)
		, _days(0, ValRange(e_editAll, 0, 127, 0, 7))
		, _dwellZone(dwellZone)
	{
#ifdef ZPSIM
		ui_Objects()[(long)this] = "Dataset_ProfileDays";
#endif
	}

	void Dataset_ProfileDays::setMatchArgs() {
		// ProfileDays depends on dwellProgs RecordInterface and dwellingZone RecordInterface.
		// Each is selected on the user-interface.
		// The ProfileDays query is a FilterQ, filtering on ProgramID obtained from the parent dwellProgs RecordInterface.
		// But the ProfileDays FilterQ-IncrementQ is itself a FilterQ, 
		// whose MatchArgument is the ZoneID obtained from the dwellingZone RecordInterface
		// The dwellingZone RecordInterface has a Link Query to DwellingZones as its source. Its Record() is a Zone.

		query().setMatchArg(parentIndex()); // parent is ProgramID. Required here
		Answer_R<R_Zone> zone = _dwellZone->record();
		auto zoneID = zone.id();
		auto prevMatchArg = query().incrementQ().matchArg();
		if (zoneID != prevMatchArg) {
			query().incrementQ().setMatchArg(zoneID);
			record() = *(_recSel.begin());
			setRecordID(_recSel.id());
		}
	}

	int Dataset_ProfileDays::resetCount() {
		setMatchArgs();
		return Record_Interface::resetCount();
	}

	I_UI_Wrapper * Dataset_ProfileDays::getField(int fieldID) {
		if (recordID() == -1) return 0;
		setMatchArgs();
		switch (fieldID) {
		case e_days:
			_days = record().rec().days;
			return &_days;
		default: return 0;
		}
	}

	bool Dataset_ProfileDays::setNewValue(int fieldID, const I_UI_Wrapper * newValue) {
		// Every Profile needs at least one TT
		// The collection of profiles for this Program/Zone must include all days just once
		// The first missing day up to that profile must be the first included day in the next profile.
		// To keep profiles in day-order:
		//	a) don't allow edit cursor on first included day
		//  b) If a day is stolen from a later profile the next missing day must be found and moved to the next profile.
		// Empty profiles must delete their TT's
		// New profiles must be given a copy of it's parent TTs.
		switch (fieldID) {
		case e_days: {
			uint8_t oldDays = _days.val & 0x7F;
			uint8_t newDays = uint8_t(newValue->val);
			uint8_t removedDays = oldDays & ~newDays;
			uint8_t addedDays = newDays & ~oldDays;
			record().rec().days = newDays;
			setRecordID(record().update());
			addDaysToNextProfile(removedDays);
			stealFromOtherProfile(record().id(), addedDays);
			promoteOutOfOrderDays();
			break;
			}
		default: ;
		}
		return false;
	}

	void Dataset_ProfileDays::addDays(Answer_R<R_Profile> & profile, uint8_t days) {
		profile.rec().days |= days;
		profile.update();
	};

	void Dataset_ProfileDays::addDaysToNextProfile(int daysToAdd) {
		// Algorithm
		if (daysToAdd) {
			Answer_R<R_Profile> profile = *(++_recSel);
			if (profile.status() == TB_OK) {
				addDays(profile, daysToAdd );
			}
			else {
				createProfile(daysToAdd);
			}
		}
	}

	void Dataset_ProfileDays::stealFromOtherProfile(int thisProfile, int daysToRemove) {
		// Lambdas
		auto removeDays = [](uint8_t days, Answer_R<R_Profile> & profile) {
			if (days) {
				profile.rec().days &= ~days;
				profile.update();
			}
		};

		// Algorithm
		for (Answer_R<R_Profile> profile : query()) {
			auto duplicateDays = daysToRemove & profile.rec().days;
			if (duplicateDays && profile.id() != thisProfile) {
				removeDays(duplicateDays, profile);
				break;
			}
		}
	}

	int Dataset_ProfileDays::firstIncludedDayPosition(int days) {
		int pos = 0;
		firstIncludedDay(days, &pos);
		return pos;
	}

	int Dataset_ProfileDays::firstIncludedDay(int days, int * pos) {
		auto _firstIncludedDay = 64;
		if (days == 0) return 0;
		while (!(days & _firstIncludedDay)) {
			_firstIncludedDay >>= 1;
			if (pos) ++(*pos);
		}
		return _firstIncludedDay;
	}

	int Dataset_ProfileDays::firstMissingDay(int days) {
		auto _firstMissingDay = 64;
		if (days == 127) return 0;
		while (!(~days & _firstMissingDay)) {
			_firstMissingDay >>= 1;
		}
		return _firstMissingDay;
	}

	void Dataset_ProfileDays::promoteOutOfOrderDays() {
		auto _firstMissingDay = 64;
		auto _foundDays = 0;
		// Algorithm
		for (Answer_R<R_Profile> profile : query()) {
			auto _firstIncludedDay = firstIncludedDay(profile.rec().days);
			if (_firstIncludedDay != _firstMissingDay) {
				addDays(profile, _firstMissingDay);
				stealFromOtherProfile(profile.id(), _firstMissingDay);
			}
			auto days = profile.rec().days;
			if (days == 0) profile.deleteRecord();
			else _foundDays |= days;
			_firstMissingDay = firstMissingDay(_foundDays);
		}
	}

	void Dataset_ProfileDays::createProfile(uint8_t days) {
		if (days) {
			auto profile = record().rec();
			profile.days = days;
			auto newProfile = query().insert(profile);
			createProfileTT(newProfile.id());
		}
	}

	void Dataset_ProfileDays::createProfileTT(int profileID) {
		auto rdb_b = query().getDB();
		auto _db = static_cast<RDB<Assembly::TB_NoOfTables>*>(rdb_b);
		auto tt_tableQ = _db->tableQuery(Assembly::TB_TimeTemp);
		auto tt_FilterQ = QueryF_T<R_TimeTemp>{ tt_tableQ , 0 };
		tt_FilterQ.setMatchArg(profileID);
		auto gotTT = tt_FilterQ.begin();
		if (gotTT.status() != TB_OK) {
			auto newTime = TimeTemp{ {7,0},18 };
			tt_tableQ.insert(R_TimeTemp { RecordID(profileID), newTime });
		}
	}

}