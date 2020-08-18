#include "I_Edit_Hndl.h"
#include "I_Data_Formatter.h"
#include "UI_Primitives.h"

namespace LCD_UI {

	// *************** Int_Interface : Class that modifies state of edited value  ******************
	//Permitted_Vals I_Edit_Hndl::_editVal;
	
	I_Edit_Hndl::I_Edit_Hndl(UI_Object * permittedVals) : Collection_Hndl(*permittedVals) {}

	Collection_Hndl * I_Edit_Hndl::on_back() {// function is called on the focus() object to notify it has been cancelled.
		return backUI()->on_back();
	}

	void I_Edit_Hndl::setInRangeValue() {
		if (currValue().val < currValue().valRange.minVal) currValue().val = currValue().valRange.minVal;
		if (currValue().val > currValue().valRange.maxVal) currValue().val = currValue().valRange.maxVal;
	}

	int I_Edit_Hndl::cursorFromFocus(int focusIndex) { // Sets CursorPos
		currValue().valRange._cursorPos = focusIndex;
		return focusIndex;
	}
}