#pragma once
#include "UI_LazyCollection.h"
#include "ValRange.h"
#include <RDB.h>
#include "I_Field_Interface.h"

//#include <string>
//#include <iostream>
//#include <iomanip>

//#define ZPSIM
namespace LCD_UI {
	using namespace RelationalDatabase;
	class ValRange;
	class I_Record_Interface;
	void notifyDataIsEdited(); // global function prototype for notifying anyone who needs to know

	// **********************  Interface to Editable data ******************
	// Objects are constructed with an fieldID to obtain the particular item required from the target data-UI-interface object of type <Dataset_Type>.
	// The UI points to a collection of data belonging to the parent, e.g. all zones for the parent dwelling.
	// To make the data editable, set the bahaviour to editable.
	// streamElement(buffer) returns the data from the target object. 40 byte Objects.
	// Multiple UI_FieldData objects may share the same I_Record_Interface dataset, but selecting a different field.
	// If any field moves to a different record, all fields need to see the new record.
	// Therefore the collection-focus needs to be derived from the record, rather than the record being set by the UI element.

	class UI_FieldData : public LazyCollection {
	public:
		UI_FieldData(I_Record_Interface * dataset, int fieldID, UI_FieldData * parent = 0, int selectFldID = 0
			, Behaviour selectBehaviour = viewOneRecycle(), Behaviour editBehaviour = viewAllNonRecycle()
			, OnSelectFnctr onSelect = 0);

		// Queries
		int	focusIndex() const override;
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex=0) const override;
		HI_BD::CursorMode cursorMode(const Object_Hndl * activeElement) const override;
		int cursorOffset(const char * data) const override;
		I_Record_Interface * data() const { return _data; }
		int nextIndex(int id) const override;
		const Coll_Iterator end() const { return Coll_Iterator( this, endIndex() ); }

		// Modifiers
		Collection_Hndl * item(int elementIndex) override;
		void focusHasChanged(bool hasFocus) override;
		void setFocusIndex(int focus) override;
		void setObjectIndex(int index) const override {;} // done by item()
		void insertNewData();
		void deleteData();
		void set_OnSelFn_TargetUI(Collection_Hndl * obj);
		 
		Collection_Hndl * saveEdit(const I_UI_Wrapper * data);
		Field_Interface_h & getInterface() { return _field_Interface_h; }
	private:
		int fieldID() { return _field_Interface_h.fieldID(); }
		void moveToSavedRecord();
		I_Record_Interface * _data;
		UI_FieldData * _parentFieldData;
		Field_Interface_h _field_Interface_h;
		signed char _selectFieldID;
	};
}
