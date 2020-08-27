#include "UI_FieldData.h"
#include "I_Record_Interface.h"
#include "UI_Primitives.h"

#ifdef ZPSIM
#include <iostream>
#include <map>
std::map<long, std::string> & ui_Objects();
using namespace std;
#endif

namespace LCD_UI {
	using namespace RelationalDatabase;

	UI_FieldData::UI_FieldData(I_Record_Interface * dataset, int fieldID
		, Behaviour collectionBehaviour, Behaviour activeEditBehaviour
		, UI_FieldData * parent, int selectFldID, OnSelectFnctr onSelect)
		: LazyCollection(dataset->count(), collectionBehaviour)
		, _data(dataset)
		, _parentFieldData(parent)
		, _field_StreamingTool_h(_data->initialiseRecord(fieldID)->ui(), activeEditBehaviour,fieldID, this, onSelect)
		, _selectFieldID(selectFldID)
	{
		//if (_data->parent())
		//	LazyCollection::setFocusIndex(_data->parentIndex());
		//else 
		//	LazyCollection::setFocusIndex(_data->recordID());
		_data->move_to(focusIndex());
		setObjectIndex(focusIndex());
	} 

	void UI_FieldData::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_field_StreamingTool_h.set_OnSelFn_TargetUI(obj);
	}

	void UI_FieldData::focusHasChanged(bool hasFocus) {
		// parent field might have been changed
		auto objectAtFocus = _data->query()[focusIndex()];
#ifdef ZPSIM
		//logger() << F("\tfocusHasChanged on ") << ui_Objects()[(long)_field_StreamingTool_h.get()].c_str() <<  L_tabs 
		//	<< F("\n\t\tFocusIndex was: ") << focusIndex()
		//	<< F("\n\t\tObjectIndex was: ") << objectIndex()
		//	//<< F("Obj ID was: ") << objectAtFocus.id()
		//	<< F("Parent ID was: ") << (_parentFieldData ? _parentFieldData->data()->record().id() : 0)
		//	<< L_endl;
#endif		
		bool focusWasInRange = objectAtFocus.status() == TB_OK;
	    setCount(_data->resetCount());
		auto newStart = _data->recordID();
		setObjectIndex(newStart);
		if (_data->parent())
			LazyCollection::setFocusIndex(_data->parentIndex());
		objectAtFocus = _data->query()[focusIndex()];
		bool focusStillInRange = objectAtFocus.status() == TB_OK;
		if (hasFocus || (focusWasInRange && !focusStillInRange))
			setFocusIndex(newStart);

//#ifdef ZPSIM
//		logger() <<  L_tabs << F("\tNew FocusIndex: ") << focusIndex()
//		<< F("New Obj ID: ") << objectAtFocus.id() << L_endl;
//#endif
	}

	int UI_FieldData::nextIndex(int id) const {
		_data->move_to(id);
		++id;
		return _data->move_to(id);
	}

	void UI_FieldData::setFocusIndex(int focus) {
		_data->move_to(focus);		
		LazyCollection::setFocusIndex(focus);
	}

	Collection_Hndl * UI_FieldData::item(int elementIndex) { // return 0 if record invalid
		//if (_parentFieldData) { // where a parent points to a single child, this allows the child object to be chosen
			if (/*elementIndex == focusIndex() &&*/ _field_StreamingTool_h.cursorMode(&_field_StreamingTool_h) == HI_BD::e_inEdit) {
				if (_field_StreamingTool_h.activeEditBehaviour().is_next_on_UpDn()) {
					elementIndex = _data->move_to(_field_StreamingTool_h.f_interface().editItem().currValue().val);
				} 
				//else {
				//	_field_StreamingTool_h.f_interface().setDataSource(&_field_StreamingTool_h);
				//	LazyCollection::setObjectIndex(elementIndex);
				//	return &_field_StreamingTool_h;
				//}
			} else {
				//if (_data->parent()) elementIndex = _data->parentIndex();
			}
		//	else {
		//		// get selectionField of parentRecord pointing to its child object
		//		// used by SpellProgram UI which depends upon the Spell RecordInterface.
		//		auto selectedID = _parentFieldData->data()->recordField(_selectFieldID);
		//		elementIndex = _data->move_to(selectedID);
		//	}
		//}
		
		auto wrapper = _data->getFieldAt(fieldID(), elementIndex);
		LazyCollection::setObjectIndex(_data->recordID());
		if (atEnd(objectIndex()) || objectIndex() == -1) return 0;
		_field_StreamingTool_h.f_interface().setDataSource(&_field_StreamingTool_h);
		_field_StreamingTool_h.f_interface().setWrapper(wrapper);
	
		return &_field_StreamingTool_h;
	}

	bool UI_FieldData::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, UI_DisplayBuffer::ListStatus listStatus) const {
		filter(filter_viewable());
		auto hasStreamed = false;
		if (behaviour().is_viewOne()) {
			hasStreamed = _field_StreamingTool_h->streamElement(buffer, activeElement, endPos, listStatus);
		} else {
			auto focus_index = LazyCollection::focusIndex();
			for (auto & element : *this) {
				auto objIndex = objectIndex();
				auto activeEl = activeElement;
				if (objIndex != focus_index) activeEl = 0;
				hasStreamed = _field_StreamingTool_h->streamElement(buffer, activeEl, endPos, listStatus);
			}
		}
		return hasStreamed;
	}

	Collection_Hndl * UI_FieldData::saveEdit(const I_Data_Formatter * data) {
		if (_parentFieldData) {
			auto newID = IntWrapper(objectIndex(),0);
			if (_parentFieldData->data()->setNewValue(_selectFieldID, &newID)) {
				_parentFieldData->setFocusIndex(_parentFieldData->data()->recordID());
			}
		}
		else {
			_data->move_to(focusIndex());
			if (_data->setNewValue(fieldID(), data)) {
				moveToSavedRecord();
				item(focusIndex());
			}
			setCount(_data->resetCount());
		}
		notifyDataIsEdited();
		return 0;
	}

	void UI_FieldData::moveToSavedRecord() {
		auto newFocus = _data->recordID();
		setFocusIndex(newFocus);
		_field_StreamingTool_h.setCursorPos();
	}

	CursorMode UI_FieldData::cursorMode(const Object_Hndl * activeElement) const {
		return _field_StreamingTool_h.cursorMode(activeElement);
	}

	int UI_FieldData::cursorOffset(const char * data) const {
		return _field_StreamingTool_h.cursorOffset(data);
	}

	void UI_FieldData::insertNewData() {
		_data->insertNewData();
	}

	void UI_FieldData::deleteData() {
		_data->deleteData();
		setCount(_data->resetCount());
	}

}