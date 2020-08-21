#pragma once
#include "UI_Collection.h"
#include "I_Data_Formatter.h"
#include <RDB.h>
#include "I_Streaming_Tool.h"

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
	/// <summary>
	/// Objects are constructed with the target record-interface and a fieldID to specify the record-field required.
	/// The UI may obtain its recordID from a given field of a parent UI_FieldData.
	/// collectionBehaviour may be (default first) One/All, non-Recycle/Recycle, NotNewline/Newline, LR-MoveCursor/LR-NextMember, UD-Nothing/UD-NextActive/UD-Edit/UD-Save, Selectible/Unselectible, Visible/Hidden.
	/// activeBehaviour may be (default first) One(not in edit)/All(during edit), LR-nonRecycle/Recycle, UD-Edit(edit member)/UD-Nothing(no edit)/UD-NextActive(change member).
	/// </summary>
	class UI_FieldData : public LazyCollection {
	public:
		UI_FieldData(I_Record_Interface * dataset, int fieldID, UI_FieldData * parent = 0, int selectFldID = 0
			, Behaviour collectionBehaviour = { V + S + V1 + R0 + UD_A }, Behaviour activeBehaviour = {UD_E+R0}
			, OnSelectFnctr onSelect = 0);

		// Queries
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
		 
		Collection_Hndl * saveEdit(const I_Data_Formatter * data);
		Field_StreamingTool_h & getStreamingTool() { return _field_StreamingTool_h; }
	private:
		int fieldID() { return _field_StreamingTool_h.fieldID(); }
		void moveToSavedRecord();
		I_Record_Interface * _data;
		UI_FieldData * _parentFieldData;
		Field_StreamingTool_h _field_StreamingTool_h;
		signed char _selectFieldID;
	};
}
