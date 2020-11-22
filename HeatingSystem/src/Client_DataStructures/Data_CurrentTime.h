#pragma once
#include "..\LCD_UI\I_Record_Interface.h"
#include "..\LCD_UI\UI_Primitives.h"
#include "..\LCD_UI\I_Data_Formatter.h"
#include "..\..\..\RDB\src\RDB.h"
#include "..\..\..\DateTime\src\Date_Time.h"

namespace client_data_structures {
	using namespace Date_Time;
	using namespace LCD_UI;

	//***************************************************
	//              CurrentTime UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps CurrentTime values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static CurrentTime_Interface object via ui().
	/// The streamed value is held in the base-class as .val
	/// </summary>
	class CurrentTimeWrapper : public I_Data_Formatter {
	public:
		using I_Data_Formatter::ui;
		CurrentTimeWrapper() = default;
		CurrentTimeWrapper(TimeOnly dateVal, ValRange valRangeArg);
		I_Streaming_Tool & ui() override;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of CurrentTime fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_CurrentTime_h : public I_Edit_Hndl {
	public:
		using I_Edit_Hndl::currValue;
		Edit_CurrentTime_h() : I_Edit_Hndl(&editVal) {
		}
		int gotFocus(const I_Data_Formatter * data) override; // returns select focus
		bool upDown(int moveBy, Behaviour ud_behaviour) override;
		//bool move_focus_by(int moveBy) override; // move focus to next charater during edit
		I_Data_Formatter & currValue() override { return _currValue; }
		int cursorFromFocus(int focusIndex) override;
	private:
		CurrentTimeWrapper _currValue; // copy value for editing
		Permitted_Vals editVal;
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for CurrentTime fields
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the CurrentTime.
	/// </summary>
	class CurrentTime_Interface : public I_Streaming_Tool {
	public:
		using  I_Streaming_Tool::editItem;
#ifdef ZPSIM
		CurrentTime_Interface() { ui_Objects()[(long)this] = "CurrentTime_Interface"; }
#endif
		// Queries

		const char * streamData(bool isActiveElement) const override;
		DateTime valAsDate() const { return DateTime(_editItem.currValue().val); }
		// Modifiers
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_CurrentTime_h _editItem;
	private:
	};

	//***************************************************
	//              CurrentDate UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps CurrentDate values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static CurrentDate_Interface object via ui().
	/// </summary>
	class CurrentDateWrapper : public I_Data_Formatter {
	public:
		using I_Data_Formatter::ui;
		CurrentDateWrapper() = default;
		CurrentDateWrapper(DateOnly dateVal, ValRange valRangeArg);
		I_Streaming_Tool & ui() override;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of CurrentDate fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_CurrentDate_h : public I_Edit_Hndl {
	public:
		using I_Edit_Hndl::currValue;
		Edit_CurrentDate_h() : I_Edit_Hndl(&editVal) {
		}
		int gotFocus(const I_Data_Formatter * data) override; // returns select focus
		
		bool upDown(int moveBy, Behaviour ud_behaviour) override; // move focus to next charater during edit
		I_Data_Formatter & currValue() override { return _currValue; }
		int cursorFromFocus(int focusIndex) override;
	private:
		CurrentDateWrapper _currValue; // copy value for editing
		Permitted_Vals editVal;
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for CurrentDate fields
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the CurrentDate.
	/// </summary>
	class CurrentDate_Interface : public I_Streaming_Tool {
	public:
		using  I_Streaming_Tool::editItem;
#ifdef ZPSIM
		CurrentDate_Interface() { ui_Objects()[(long)this] = "CurrentDate_Interface"; }
#endif
		// Queries
		const char * streamData(bool isActiveElement) const override;
		DateTime valAsDate() const { return DateTime(_editItem.currValue().val); }
		// Modifiers
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_CurrentDate_h _editItem;
	private:
	};


	//***************************************************
	//              RecInt_CurrDateTime
	//***************************************************

	/// <summary>
	/// DB Interface to Data outside the RDB.
	/// Provides streamable fields to special date.
	/// A single object may be used to stream and edit any of the fields via getField
	/// </summary>
	class RecInt_CurrDateTime : public I_Record_Interface
	{
	public:
		enum {e_currTime, e_currDate, e_dst, e_sdcard};
		RecInt_CurrDateTime();
		I_Data_Formatter * getField(int fieldID) override;
		bool setNewValue(int fieldID, const I_Data_Formatter * val) override;
	private:
		CurrentTimeWrapper _currTime; // size is 32 bits.
		CurrentDateWrapper _currDate; // size is 32 bits.
		IntWrapper _dst;
		StrWrapper _SDCard;
	};
}