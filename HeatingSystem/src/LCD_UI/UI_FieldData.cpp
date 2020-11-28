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

	UI_FieldData::UI_FieldData(Dataset * dataset, int fieldID
		, uint16_t collectionBehaviour
		, int selectFldID, OnSelectFnctr onSelect)
		: LazyCollection(dataset->count(), Behaviour(collectionBehaviour))
		, _data(dataset)
		, _field_StreamingTool_h(_data->initialiseRecord(fieldID)->ui(), collectionBehaviour, fieldID, this, onSelect)
		, _selectFieldID(selectFldID)
	{
		_data->move_to(focusIndex());
		setObjectIndex(focusIndex());
	} 

	void UI_FieldData::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_field_StreamingTool_h.set_OnSelFn_TargetUI(obj);
	}

	bool UI_FieldData::focusHasChanged(bool) {
		// parent field might have been changed
		auto objectAtFocus = _data->query()[focusIndex()];
#ifdef ZPSIM
		logger() << F("\tfocusHasChanged on ") << ui_Objects()[(long)_field_StreamingTool_h.get()].c_str()
			<< F("\n\t\tWas Parent ID: ") << _data->parentIndex()
			<< F(" FocusIndex: ") << focusIndex()
			<< F(" ObjectIndex: ") << objectIndex()
			<< L_endl;
#endif		
		bool focusWasInRange = objectAtFocus.status() == TB_OK;
	    
		setCount(_data->resetCount());
		auto newRS = _data->query().begin();
		auto newStart = newRS.id();
		_data->query().moveTo(newRS, focusIndex());
		_data->query().next(newRS, 0);
		bool focusStillInRange = newRS.status() == TB_OK;
		if (focusWasInRange) {
			if (newRS.id() != focusIndex()) setFocusIndex(newStart);
		}
		_data->setRecordID(focusIndex());

#ifdef ZPSIM
		logger() << F("\t\tNow Parent ID: ") << _data->parentIndex()
			<< F(" FocusIndex: ") << focusIndex()
			<< F(" ObjectIndex: ") << objectIndex()
			<< L_endl;
#endif
		return true;
	}

	int UI_FieldData::nextIndex(int id) const {
		id = _data->move_to(id);
		++id;
		return _data->move_to(id);
	}

	void UI_FieldData::setFocusIndex(int focus) {
		_data->move_to(focus);		
		LazyCollection::setFocusIndex(focus);
	}

	Collection_Hndl * UI_FieldData::item(int elementIndex) { // return 0 if record invalid
		if (_field_StreamingTool_h.cursorMode(&_field_StreamingTool_h) == HI_BD::e_inEdit) {
			if (_field_StreamingTool_h.activeEditBehaviour().is_next_on_UpDn()) {
				auto newRecID = _field_StreamingTool_h.f_interface().editItem().currValue().val; //progID for SpellProg
				elementIndex = _data->move_to(newRecID);
			} 
		} 
		
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

	Collection_Hndl * UI_FieldData::saveEdit(const I_Data_Formatter * newData) {
		if (data()->inParentEditMode()) {
			auto newID = IntWrapper(objectIndex(),0);
			data()->setNewValue(_selectFieldID, &newID);
		}
		else {
			_data->move_to(focusIndex());
			if (_data->setNewValue(fieldID(), newData)) {
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

	bool UI_FieldData::upDown(int moveBy, Collection_Hndl* colln_hndl, Behaviour ud_behaviour) {
		if (ud_behaviour.is_cmd_on_UD()) {
			return !data()->actionOn_UD(fieldID());
		}
		return colln_hndl->move_focus_by(moveBy);
	}


}