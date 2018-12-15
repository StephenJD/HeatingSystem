#include "UI_FieldData.h"
#include "I_Record_Interface.h"
#include "UI_Primitives.h"

namespace LCD_UI {
	using namespace RelationalDatabase;

	UI_FieldData::UI_FieldData(I_Record_Interface * dataset, int fieldID, I_Record_Interface * parent, int selectFldID
		, Behaviour selectBehaviour, Behaviour editBehaviour
		, OnSelectFnctr onSelect)
		: LazyCollection(dataset->count(), selectBehaviour)
		, _data(dataset)
		, _parentRecord(parent)
		, _field_Interface_h(_data->initialiseRecord(fieldID)->ui(), editBehaviour,fieldID, this, onSelect)
		, _selectFieldID(selectFldID)
	{
		setFocusIndex(_data->recordID());
		setObjectIndex(focusIndex());
		std::cout << "LazyCollection UI_FieldData Addr: " << std::hex << long long(this) << std::endl;
		std::cout << "   Field_Interface_h Addr: " << std::hex << long long(&_field_Interface_h) << std::endl << std::endl;
	} 

	void UI_FieldData::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_field_Interface_h.set_OnSelFn_TargetUI(obj);
	}

	void UI_FieldData::focusHasChanged(bool hasFocus) {
		// parent field might have been changed
		auto objectAtFocus = _data->query()[focusIndex()];
		bool focusWasInRange = objectAtFocus.status() == TB_OK;
	    setCount(_data->resetCount());
		setObjectIndex(_data->recordID());
		objectAtFocus = _data->query()[focusIndex()];
		bool focusStillInRange = objectAtFocus.status() == TB_OK;
		if (hasFocus || (focusWasInRange && !focusStillInRange))
			setFocusIndex(objectIndex());
	}

	int UI_FieldData::nextIndex(int id) const {
		_data->move_to(id);
		++id;
		return _data->move_to(id);
	}

	Collection_Hndl * UI_FieldData::item(int elementIndex) { // return 0 if record invalid
		if (_parentRecord) { // where a parent points to a single child, this allows the child object to be chosen
			if (_field_Interface_h.cursorMode(&_field_Interface_h) == HI_BD::e_inEdit) {
				elementIndex = _data->move_to(_field_Interface_h.f_interface().editItem().currValue().val);
			}
			else {
				// get selectionField of parentRecord pointing to its child object
				// used by SpellProgram UI which depends upon the Spell RecordInterface.
				auto selectedID = _parentRecord->recordField(_selectFieldID);
				elementIndex = _data->move_to(selectedID);
			}
		}
		
		auto wrapper = _data->getFieldAt(fieldID(), elementIndex);
		setObjectIndex(_data->recordID());
		if (atEnd(objectIndex()) || objectIndex() == -1) return 0;
		_field_Interface_h.f_interface().setWrapper(wrapper);
		return &_field_Interface_h;
	}

	const char * UI_FieldData::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		// only called for viewall.	
		auto focus_index = focusIndex();
		for (auto & element : *this) {
			auto endVisibleIndex = shortColl->endVisibleItem();
			auto objIndex = objectIndex();
			if (endVisibleIndex) {
				if (objIndex < shortColl->firstVisibleItem()) continue;
				if (objIndex > endVisibleIndex) break;
			}

			auto activeEl = activeElement;
			if (objIndex != focus_index) activeEl = 0;
			_field_Interface_h.streamElement(buffer, activeEl, shortColl, objIndex);
		}
		return buffer.toCStr();
	}

	Collection_Hndl * UI_FieldData::saveEdit(const I_UI_Wrapper * data) {
		if (_parentRecord) {
			auto newID = IntWrapper(objectIndex(),0);
			_parentRecord->setNewValue(_selectFieldID, &newID);
		}
		else {
			_data->move_to(focusIndex());
			if (_data->setNewValue(fieldID(), data)) {
				moveToSavedRecord();
				item(focusIndex());
			}
			setCount(_data->resetCount());
		}
		return 0;
	}

	void UI_FieldData::moveToSavedRecord() {
		auto newFocus = _data->recordID();
		setFocusIndex(newFocus);
		_field_Interface_h.setCursorPos();
	}

	CursorMode UI_FieldData::cursorMode(const Object_Hndl * activeElement) const {
		return _field_Interface_h.cursorMode(activeElement);
	}

	int UI_FieldData::cursorOffset(const char * data) const {
		return _field_Interface_h.cursorOffset(data);
	}

	void UI_FieldData::insertNewData() {
		_data->insertNewData();
	}

	void UI_FieldData::deleteData() {
		_data->deleteData();
		setCount(_data->resetCount());
	}

}