#pragma once
#include "UI_Collection.h"

namespace LCD_UI {
	class I_Data_Formatter;
	class Permitted_Vals;
	class ValRange;
	// New interface classes that know how to edit themselves

	/// <summary>
	/// This Collection_Hndl derivative provides editing of values within a field.
	/// It points to the collection of permitted values.
	/// It is the recipient_activeUI during edits.
	/// The focus is the current permitted value.
	/// </summary>
	class I_Edit_Hndl : public Collection_Hndl { // Shared object for editing any data item.
											// Only one item may be in edit on the entire system
	public:
		I_Edit_Hndl(UI_Object * permittedVals);
		// In the constructor, Permitted_Vals must be taken and passed all the way up by pointer, not reference
		// as the reference will be 0 until after construction,
		// whereas a pointer is correctly assigned.

		// Polymorphic Queries
		int focusIndex() const { return backUI()->focusIndex(); }
		virtual int cursorFromFocus(int focusIndex); // Sets CursorPos

		// Polymorphic Modifiers
		Collection_Hndl * on_back() override; // function is called on the active object to notify it has been cancelled.
		virtual int gotFocus(const I_Data_Formatter * data) = 0; // returns select focus
		virtual int editMemberSelection(const I_Data_Formatter * _data_formatter, int recordID) { return gotFocus(_data_formatter); }; // returns select focus
		virtual int getEditCursorPos() { return cursorFromFocus(focusIndex()); }
		virtual I_Data_Formatter & currValue() = 0;
		virtual void saveEdit() {}

		// Queries
		const I_Data_Formatter & currValue() const { return const_cast<I_Edit_Hndl *>(this)->currValue(); }

		// Modifiers
		virtual void setInRangeValue();
	protected:
		//static Permitted_Vals _editVal;
		// derived classes provide copy wrapper for editing the data
	};
}
