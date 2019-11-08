#pragma once
#include "..\..\..\RDB\src\RDB.h"
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\ValRange.h"
#include "..\..\..\DateTime\src\Time_Only.h"
#include <Logging.h>

namespace client_data_structures {
	using namespace LCD_UI;

	class TimeTemp {
	public:
		constexpr TimeTemp(uint16_t tt) : _time_temp(tt) {}
		constexpr TimeTemp(int32_t tt) : _time_temp(uint16_t(tt)) {}
		constexpr TimeTemp(Date_Time::TimeOnly time, int8_t temp) : _time_temp((uint16_t(time.asInt()) << 8) + temp + 10) {}
		TimeTemp() = default;
		Date_Time::TimeOnly time() const { return Date_Time::TimeOnly{ _time_temp >> 8 }; }
		int8_t temp() const { return (_time_temp & 0xff) - 10; }
		void setTime(Date_Time::TimeOnly time) { _time_temp = (_time_temp & 0xff) + (uint16_t(time.asInt()) << 8); }
		void setTemp(int temp) { _time_temp = (_time_temp & 0xff00) + temp + 10; }
		bool operator < (TimeTemp rhs) const { return _time_temp < rhs._time_temp; }
		bool operator == (TimeTemp rhs) const { return _time_temp == rhs._time_temp; }
		explicit operator uint16_t() const { return _time_temp; }
	private:
		uint16_t _time_temp = 0;
	};

	//***************************************************
	//              TimeTemp RDB Table
	//***************************************************

	struct R_TimeTemp {
		// Each Profile has one or more TimeTemps defining the temperature for times throughout a day
		constexpr R_TimeTemp(RecordID id, TimeTemp tt) : profileID(id), time_temp(tt) {}
		R_TimeTemp() = default;
		RecordID profileID = 0;
		TimeTemp time_temp;
		RecordID field(int fieldIndex) const { return profileID; }
		Date_Time::TimeOnly time() const { return time_temp.time(); }
		int8_t temp() const { return time_temp.temp(); }
		void setTime(Date_Time::TimeOnly time) { time_temp.setTime(time); }
		void setTemp(int temp) { time_temp.setTemp(temp); }

		bool operator < (R_TimeTemp rhs) const { return time_temp < rhs.time_temp; }
		bool operator == (R_TimeTemp rhs) const { return time_temp == rhs.time_temp; }
	};

	inline Logger & operator << (Logger & stream, const R_TimeTemp & timeTemp) {
		using namespace Date_Time;
		return stream << "TimeTemp for ProfileID: " << (int)timeTemp.profileID << " time: " 
			<< intToString(timeTemp.time_temp.time().hrs(), 2, '0') << (int)timeTemp.time_temp.time().mins10() << "0  Temp: " << (int)timeTemp.time_temp.temp();
	}

	//***************************************************
	//              TimeTemp UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps TimeTemp values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static TimeTemp_Interface object via ui().
	/// </summary>
	class TimeTempWrapper : public I_UI_Wrapper {
	public:
		using I_UI_Wrapper::ui;
		TimeTempWrapper() = default;
		TimeTempWrapper(int16_t daysVal);
		TimeTempWrapper(int16_t daysVal, ValRange valRangeArg);
		TimeTempWrapper & operator= (const I_UI_Wrapper & wrapper) { I_UI_Wrapper::operator=(wrapper); return *this; }

		I_Field_Interface & ui() override;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of TimeTemp fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_TimeTemp_h : public Edit_Int_h {
	public:
		enum { e_hours, e_10Mins, e_am_pm, e_10Temp, e_temp, e_NO_OF_EDIT_POS };
		using I_Edit_Hndl::currValue;
		int gotFocus(const I_UI_Wrapper * data) override; // returns select focus
		bool move_focus_by(int moveBy) override; // move focus to next charater during edit
		I_UI_Wrapper & currValue() override { return _currValue; }
		int cursorFromFocus(int focusIndex) override;
	private:
		TimeTempWrapper _currValue; // copy value for editing
		Permitted_Vals editVal;

	};

	/// <summary>
	/// This LazyCollection derivative is the UI for TimeTemp
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the TimeTemp.
	/// </summary>
	class TimeTemp_Interface : public I_Field_Interface {
	public:
		using I_Field_Interface::editItem;
		const char * streamData(bool isActiveElement) const override;
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_TimeTemp_h _editItem;
	};

	//***************************************************
	//              TimeTemp DB Interface
	//***************************************************

	/// <summary>
	/// DB Interface to all TimeTemp Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class Dataset_TimeTemp : public Record_Interface<R_TimeTemp>
	{
	public:
		enum streamable { e_TimeTemp, e_profileID };
		Dataset_TimeTemp(Query & query, VolatileData * volData, I_Record_Interface * parent);
		I_UI_Wrapper * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_UI_Wrapper * val) override;
		void insertNewData() override;
		int recordField(int selectFieldID) const override {
			return record().rec().field(selectFieldID);
		}
	private:
		TimeTempWrapper _timeTemp; // size is 32 bits.
	};
}