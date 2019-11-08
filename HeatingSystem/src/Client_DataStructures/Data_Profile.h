#pragma once
#include "..\..\..\RDB\src\RDB.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\ValRange.h"

namespace client_data_structures {
	using namespace LCD_UI;
	struct R_Profile {
		R_Profile() = default;
		constexpr R_Profile(RecordID programID, RecordID zoneID, uint8_t days) : programID(programID), zoneID(zoneID), days(days) {}
		// Each Program has a weeks-worth of Profiles for each Zone belonging to the Program's Dwelling
		RecordID programID = -1; // Initialisation inhibits default aggregate initialisation.
		RecordID zoneID = -1;
		uint8_t days = 0;		// Days of the week this profile applies to 
		RecordID field(int fieldIndex) const {
			if (fieldIndex == 0) return programID; else return zoneID;
		}
		bool operator < (R_Profile rhs) const { return days < rhs.days; }
		bool operator == (R_Profile rhs) const { return days == rhs.days; }
	};

	inline Logger & operator << (Logger & stream, const R_Profile & profile) {
		return stream << "Profile for ProgID: " << (int)profile.programID << " ZoneID: " << (int)profile.zoneID << " Days: " << (int)profile.days;
	}

	//***************************************************
	//              ProfileDays UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps ProfileDays values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static Profile_Interface object via ui().
	/// </summary>
	class ProfileDaysWrapper : public I_UI_Wrapper {
	public:
		using I_UI_Wrapper::ui;
		ProfileDaysWrapper() = default;
		ProfileDaysWrapper(int8_t daysVal);
		ProfileDaysWrapper(int8_t daysVal, ValRange valRangeArg);
		ProfileDaysWrapper & operator= (const I_UI_Wrapper & wrapper) { I_UI_Wrapper::operator=(wrapper); return *this; }

		I_Field_Interface & ui() override;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of ProfileDays fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_ProfileDays_h : public Edit_Int_h {
	public:
		using I_Edit_Hndl::currValue;
		int gotFocus(const I_UI_Wrapper * data) override; // returns select focus
		bool move_focus_by(int moveBy) override; // move focus to next charater during edit
		I_UI_Wrapper & currValue() override { return _currValue; }
		int cursorFromFocus(int focusIndex) override;
	private:

		ProfileDaysWrapper _currValue; // copy value for editing
		Permitted_Vals editVal;

	};

	/// <summary>
	/// This LazyCollection derivative is the UI for ProfileDays
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the Days.
	/// </summary>
	class ProfileDays_Interface : public I_Field_Interface {
	public:
		using I_Field_Interface::editItem;
		const char * streamData(bool isActiveElement) const override;
		bool isActionableObjectAt(int index) const override;
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_ProfileDays_h _editItem;
	private:
	};

	//***************************************************
	//              Profile DB Interface
	//***************************************************

	/// <summary>
	/// DB Interface to all Profile Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class Dataset_ProfileDays : public Record_Interface<R_Profile>
	{
	public:
		enum streamable { e_days };
		Dataset_ProfileDays(Query & query, VolatileData * volData, I_Record_Interface * dwellProgs, I_Record_Interface * dwellZone);
		int resetCount() override;
		I_UI_Wrapper * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_UI_Wrapper * val) override;
		static int firstIncludedDay(int days, int * pos = 0);
		static int firstIncludedDayPosition(int days);
		static int firstMissingDay(int days);

	private:
		void addDays(Answer_R<R_Profile> & profile, uint8_t days);
		void addDaysToNextProfile(int daysToAdd);
		void stealFromOtherProfile(int thisProfile, int daysToRemove);
		void promoteOutOfOrderDays();
		void createProfile(uint8_t days);
		void createProfileTT(int profileID);

		ProfileDaysWrapper _days; // size is 32 bits.
		I_Record_Interface * _dwellZone;
	};


}