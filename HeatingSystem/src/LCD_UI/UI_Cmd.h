#pragma once
#include "UI_Collection.h"

namespace LCD_UI {

	class UI_Cmd : public Collection_Hndl, public Custom_Select
	{
	public:
		using Custom_Select::behaviour;
		UI_Cmd(const char * label_text, OnSelectFnctr onSelect, Behaviour behaviour = {V+S+L0+V1});
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override;

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
		UI_Label(const char * label_text, Behaviour behaviour = { V+S0+L });
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override;
		using UI_Object::behaviour;
		Behaviour & behaviour() override { return _behaviour; }
	private:
		const char * text;
		Behaviour _behaviour;
	};
}