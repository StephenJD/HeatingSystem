#pragma once
#include <Date_Time.h>
#include "..\LCD_UI\ValRange.h"
#include "..\LCD_UI\UI_Primitives.h"

namespace client_data_structures {
	using namespace Date_Time;
	using namespace LCD_UI;
	enum {e_hours, e_10Mins, e_am_pm, e_10Days, e_day, e_month, e_NO_OF_DT_EDIT_POS};
	
	//***************************************************
	//              DateTime UI Edit
	//***************************************************

	/// <summary>
	/// This Object derivative wraps DateTime values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static DateTime_Interface object via ui().
	/// </summary>
	class DateTimeWrapper : public I_UI_Wrapper {
	public:
		using I_UI_Wrapper::ui;
		DateTimeWrapper() = default;
		DateTimeWrapper(DateTime dateVal, ValRange valRangeArg);
		I_Field_Interface & ui() override;
	};

	/// <summary>
	/// This Collection_Hndl derivative provides editing of DateTime fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_DateTime_h : public I_Edit_Hndl {
	public:
		using I_Edit_Hndl::currValue;
		Edit_DateTime_h();
		int gotFocus(const I_UI_Wrapper * data) override; // returns select focus
		int getEditCursorPos() override;
		bool move_focus_by(int moveBy) override; // move focus to next charater during edit
		I_UI_Wrapper & currValue() override { return _currValue; }
		int cursorFromFocus(int focusIndex) override;
	private:
		DateTimeWrapper _currValue; // copy value for editing
		Permitted_Vals editVal;
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for DateTime fields
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the DateTime.
	/// </summary>
	class DateTime_Interface : public I_Field_Interface {
	public:
		using  I_Field_Interface::editItem;
		// Queries
		const char * streamData(const Object_Hndl * activeElement) const override;
		DateTime valAsDate() const { return DateTime(_editItem.currValue().val); }
		// Modifiers
		I_Edit_Hndl & editItem() { return _editItem; }
	protected:
		Edit_DateTime_h _editItem;
	private:
	};
}