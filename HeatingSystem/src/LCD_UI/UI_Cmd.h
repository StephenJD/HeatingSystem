#pragma once
#include "UI_Collection.h"

namespace LCD_UI {

	class UI_Cmd : public Collection_Hndl, public Custom_Select
	{
	public:
		using Custom_Select::behaviour;
		UI_Cmd(const char * label_text, OnSelectFnctr onSelect, Behaviour behaviour = selectable());
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex = 0) const override;

		void insertCommandForEdit(Object_Hndl & ui_fieldData_collection);
		void removeCommandForEdit(Object_Hndl & ui_fieldData_collection);

	private:
		const char * text;
	};

	// ********************* Fld_Label *******************
	// Single Responsibility: Concrete class limiting the Field_Cmd for Label fields.
	class UI_Label : public UI_Object
	{
	public:
		UI_Label(const char * label_text, Behaviour behaviour = newLine());
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex = 0) const override;
		using UI_Object::behaviour;
		Behaviour & behaviour() override { return _behaviour; }
	private:
		const char * text;
		Behaviour _behaviour;
	};
}