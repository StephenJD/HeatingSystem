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

	UI_FieldData::UI_FieldData(I_Record_Interface * dataset, int fieldID, UI_FieldData * parent, int selectFldID
		, Behaviour selectBehaviour, Behaviour editBehaviour
		, OnSelectFnctr onSelect)
		: LazyCollection(dataset->count(), selectBehaviour)
		, _data(dataset)
		, _parentFieldData(parent)
		, _field_Interface_h(_data->initialiseRecord(fieldID)->ui(), editBehaviour,fieldID, this, onSelect)
		, _selectFieldID(selectFldID)
	{
		setFocusIndex(_data->recordID());
		setObjectIndex(focusIndex());
#ifdef ZPSIM
		//logger() << F("LazyCollection UI_FieldData Addr: ") << L_hex << long(this) << L_endl;
		//logger() << F("   Field_Interface_h Addr: ") << L_hex << long(&_field_Interface_h) << L_endl << L_endl;
#endif
	} 

	void UI_FieldData::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_field_Interface_h.set_OnSelFn_TargetUI(obj);
	}

	void UI_FieldData::focusHasChanged(bool hasFocus) {
		// parent field might have been changed
		auto objectAtFocus = _data->query()[focusIndex()];
#ifdef ZPSIM
		//logger() << F("\tfocusHasChanged on ") << ui_Objects()[(long)_field_Interface_h.get()].c_str() <<  L_tabs 
		//	<< F("\n\t\tFocusIndex was: ") << focusIndex()
		//	<< F("\n\t\tObjectIndex was: ") << objectIndex()
		//	//<< F("Obj ID was: ") << objectAtFocus.id()
		//	<< F("Parent ID was: ") << (_parentFieldData ? _parentFieldData->data()->record().id() : 0)
		//	<< L_endl;
#endif		
		bool focusWasInRange = objectAtFocus.status() == TB_OK;
	    setCount(_data->resetCount());
		setObjectIndex(_data->recordID());
		objectAtFocus = _data->query()[focusIndex()];
		bool focusStillInRange = objectAtFocus.status() == TB_OK;
		if (hasFocus || (focusWasInRange && !focusStillInRange))
			setFocusIndex(objectIndex());

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

	int	UI_FieldData::focusIndex() const {
		/*
		This function determins which member of a collection is shown.
		For a show-all collection, it needs to come from the LazyCollection because the recordID must be different for each record shown.
		But for a multi-field sub-page collectiom, where each field relates to the same record, a common recordID is required.
		For this, one of the fields must be the parent for the others.
		*/
		return LazyCollection::focusIndex();
	}

	void UI_FieldData::setFocusIndex(int focus) {
		_data->move_to(focus);		
		LazyCollection::setFocusIndex(focus);
	}

	Collection_Hndl * UI_FieldData::item(int elementIndex) { // return 0 if record invalid
		if (_parentFieldData) { // where a parent points to a single child, this allows the child object to be chosen
			if (_field_Interface_h.cursorMode(&_field_Interface_h) == HI_BD::e_inEdit) {
				elementIndex = _data->move_to(_field_Interface_h.f_interface().editItem().currValue().val);
			}
			else {
				// get selectionField of parentRecord pointing to its child object
				// used by SpellProgram UI which depends upon the Spell RecordInterface.
				auto selectedID = _parentFieldData->data()->recordField(_selectFieldID);
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
		//cout << "UI_FieldData::streamElement " << ui_Objects()[(long)this] << endl;
		auto focus_index = LazyCollection::focusIndex();
		for (auto & element : *this) {
			auto endVisibleIndex = shortColl->endVisibleItem();
			auto objIndex = objectIndex();
			if (endVisibleIndex) {
				if (objIndex < shortColl->firstVisibleItem()) continue;
				if (objIndex > shortColl->endVisibleItem()) break;
			}

			auto activeEl = activeElement;
			if (objIndex != focus_index) activeEl = 0;
			_field_Interface_h.streamElement(buffer, activeEl, shortColl, objIndex);
		}
		return buffer.toCStr();
	}

	Collection_Hndl * UI_FieldData::saveEdit(const I_UI_Wrapper * data) {
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