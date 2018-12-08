#include "UI_Cmd.h"
#include "Convertions.h"
#include <iostream>

namespace LCD_UI {
	using namespace std;

	// ********************* Concrete UI_Cmd *******************
	UI_Cmd::UI_Cmd(const char * label_text, OnSelectFnctr onSelect, Behaviour behaviour)
		: Collection_Hndl(this), Custom_Select(onSelect, behaviour), text(label_text) {
		cout << "UI_Cmd " << label_text << " Addr:" << hex << long long(this) << endl;
		//cout << " behaviour: " << dec << _behaviour << endl;
	}

	void UI_Cmd::insertCommandForEdit(Object_Hndl & ui_fieldData_collection) {
		auto ui_fieldData_coll = static_cast<Collection_Hndl*>(&ui_fieldData_collection);
		auto interface_h = ui_fieldData_coll->activeUI();
		interface_h->setBackUI(this);
		setBackUI(ui_fieldData_coll);
		reset(ui_fieldData_coll->get());
	}

	void UI_Cmd::removeCommandForEdit(Object_Hndl & ui_fieldData_collection) {
		auto ui_fieldData_coll = static_cast<Collection_Hndl*>(&ui_fieldData_collection);
		ui_fieldData_coll->activeUI()->setBackUI(backUI());
	}


	const char * UI_Cmd::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		return streamToBuffer(text, buffer, activeElement, shortColl,streamIndex);
	}

	// ********************* Concrete UI_Label *******************

	UI_Label::UI_Label(const char * label_text, Behaviour behaviour)
		: /*UI_Object(behaviour),*/ text(label_text), _behaviour(behaviour) {
		cout << "UI_Label " << label_text << " Addr:" << hex << long long(this) << endl;
	}

	const char * UI_Label::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		return streamToBuffer(text, buffer, activeElement, shortColl,streamIndex);
	}

}
