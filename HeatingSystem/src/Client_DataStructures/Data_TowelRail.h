#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\ValRange.h"
#include "..\..\..\RDB\src\RDB.h"
#include "..\..\..\DateTime\src\Date_Time.h"
#include "..\HardwareInterfaces\TowelRail.h"

namespace client_data_structures {
	using namespace LCD_UI;
	//***************************************************
	//              TowelRail UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps TowelRail values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static TowelRail_Interface object via ui().
	/// </summary>
	class TowelRail_Wrapper : public I_UI_Wrapper {
	public:
		using I_UI_Wrapper::ui;
		TowelRail_Wrapper() = default;
		TowelRail_Wrapper(ValRange valRangeArg);
		I_Field_Interface & ui() override;

		char name[5];
		uint8_t onTemp;
		uint8_t minutesOn;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of TowelRail fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_TowelRail_h : public Edit_Int_h {
	public:
		using I_Edit_Hndl::currValue;
		int gotFocus(const I_UI_Wrapper * data) override;  // returns initial edit focus
		int cursorFromFocus(int focusIndex) override;
		void setInRangeValue() override;
	private:
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for TowelRail
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the ReqIsTemp.
	/// </summary>
	class TowelRail_Interface : public I_Field_Interface {
	public:
		using I_Field_Interface::editItem;
		const char * streamData(bool isActiveElement) const override;
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_Int_h _editItem;
	};

	//***************************************************
	//              Zone RDB Tables
	//***************************************************

	struct R_TowelRail {
		char name[7];
		RecordID callTempSens;
		RecordID callRelay;
		RecordID mixValve;
		uint8_t onTemp;
		uint8_t minutes_on;
		bool operator < (R_TowelRail rhs) const { return false; }
		bool operator == (R_TowelRail rhs) const { return true; }
	};

	inline Logger & operator << (Logger & stream, const R_TowelRail & towelRail) {
		return stream << "TowelRail: " << towelRail.name << " On for " << towelRail.minutes_on << " minutes";
	}

	//***************************************************
	//              TowelRail DB Interface
	//***************************************************

	/// <summary>
	/// DB Interface to all TowelRail Data
	/// Provides streamable fields which may be populated by a database or the runtime-data.
	/// Initialised with the Query, and a pointer to any run-time data, held by the base-class
	/// A single object may be used to stream and edit any of the fields via getFieldAt
	/// </summary>
	class Dataset_TowelRail : public Record_Interface<R_TowelRail> {
	public:
		enum streamable { e_name, e_onTemp, e_minutesOn, e_secondsToGo };
		Dataset_TowelRail(Query & query, VolatileData * runtimeData, I_Record_Interface * parent);
		I_UI_Wrapper * getField(int _fieldID) override;
		bool setNewValue(int _fieldID, const I_UI_Wrapper * val) override { return true; }
		HardwareInterfaces::TowelRail & towelRail(int index) { return static_cast<HardwareInterfaces::TowelRail*>(runTimeData())[index]; }
	private:
		StrWrapper _name;
		IntWrapper _onTemp;
		IntWrapper _minutesOn;
		IntWrapper _secondsToGo;
	};
}