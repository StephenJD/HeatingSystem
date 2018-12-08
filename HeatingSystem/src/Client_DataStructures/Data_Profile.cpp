#include "Data_Profile.h"
#include "Data_Zone.h"
#include <ostream>

namespace Client_DataStructures {
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
		using valType = decltype(currValue().val);
		auto mask = valType(64) >> focusIndex();
		auto newVal = (currValue().val & mask);
		currValue().val = (currValue().val | mask) ^ newVal;
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
		return 0;
	}

	//*************ProfileDays_Interface****************
	
	const char * ProfileDays_Interface::streamData(const Object_Hndl * activeElement) const {
		constexpr char EDIT_SPC_CHAR[] = "-";
		int myDays = getData(activeElement);
		if (myDays & 64) strcpy(scratch, "M"); else strcpy(scratch, EDIT_SPC_CHAR);
		if (myDays & 32) strcat(scratch, "T"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 16) strcat(scratch, "W"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 8) strcat(scratch, "T"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 4) strcat(scratch, "F"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 2) strcat(scratch, "S"); else strcat(scratch, EDIT_SPC_CHAR);
		if (myDays & 1) strcat(scratch, "S"); else strcat(scratch, EDIT_SPC_CHAR);
		return scratch;
	}

	//*************Dataset_ProfileDays****************
	Dataset_ProfileDays::Dataset_ProfileDays(Query & query, VolatileData * runtimeData, I_Record_Interface * dwellProgs, I_Record_Interface * dwellZone)
		: Record_Interface(query, runtimeData, dwellProgs)
		, _days(0, ValRange(e_editAll, 0, 127, 0, 7))
		, _dwellZone(dwellZone)
	{}

	int Dataset_ProfileDays::resetCount() {
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
		return Record_Interface::resetCount();
	}

	I_UI_Wrapper * Dataset_ProfileDays::getField(int fieldID) {
		if (recordID() == -1) return 0;
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
			auto oldDays = unsigned char(_days.val);
			auto newDays = unsigned char(newValue->val);
			auto removedDays = oldDays & ~newDays;
			auto addedDays = newDays & ~oldDays;
			record().rec().days = newDays;
			setRecordID(record().update());
			addDaysToNextProfile(removedDays);
			stealFromOtherProfile(addedDays);
			ensureProfilesCoverWholeWeek();
			break;
			}
		default: ;
		}
		return false;
	}

	void Dataset_ProfileDays::addDaysToNextProfile(int daysToAdd) {
		// Lambdas
		auto addDays = [](unsigned char days, Answer_R<R_Profile> & profile) {
			profile.rec().days |= days;
			profile.update();
		};

		// Algorithm
		if (daysToAdd) {
			Answer_R<R_Profile> profile = *(++_recSel);
			if (profile.status() == TB_OK) {
				addDays(daysToAdd, profile);
			}
		}
	}

	void Dataset_ProfileDays::stealFromOtherProfile(int daysToRemove) {
		// Lambdas
		auto removeDays = [](unsigned char days, Answer_R<R_Profile> & profile) {
			if (days) {
				profile.rec().days &= ~days;
				profile.update();
			}
		};

		// Algorithm
		for (Answer_R<R_Profile> profile : query()) {
			auto duplicateDays = daysToRemove & profile.rec().days;
			if (duplicateDays && profile.id() != record().id()) {
				removeDays(duplicateDays, profile);
				break;
			}
		}
	}

	void Dataset_ProfileDays::ensureProfilesCoverWholeWeek() {
		auto missingDays = findMissingDaysInProfiles();
		createProfile(missingDays);
	}

	unsigned char Dataset_ProfileDays::findMissingDaysInProfiles() {
		// Lambdas
		auto removeDays = [](unsigned char days, Answer_R<R_Profile> & profile) {
			if (days) {
				profile.rec().days &= ~days;
				profile.update();
			}
		};

		// Algorithm
		unsigned char foundDays = 128;
		for (Answer_R<R_Profile> profile : query()) {
			auto duplicateDays = foundDays & profile.rec().days;
			removeDays(duplicateDays, profile);
			auto profileDays = profile.rec().days;
			if (profileDays) foundDays |= profileDays;
			else profile.deleteRecord();
		}
		return ~(foundDays);
	}

	void Dataset_ProfileDays::createProfile(unsigned char days) {
		if (days) {
			R_Profile profile{ record().rec() };
			profile.days = days;
			query().insert(&profile);
			//for (Answer_R<R_Profile> profile : query()) {
			//	std::cout << profile.rec() << std::endl;
			//}
		}
	}

}