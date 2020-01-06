#include "I_Field_Interface.h"
#include "I_Record_Interface.h"
#include "UI_Primitives.h"
#include "UI_FieldData.h"
#include "Conversions.h"
#include <Logging.h>

#ifdef ZPSIM
#include <map>
extern std::map<long, std::string> ui_Objects;
#endif

namespace LCD_UI {
	using namespace GP_LIB;
	using HI_BD = HardwareInterfaces::LCD_Display;
	// *********************************************
	//             I_Field_Interface 
	// *********************************************

	void I_Field_Interface::setWrapper(I_UI_Wrapper * wrapper) {
		_wrapper = wrapper;
	}

	Collection_Hndl * I_Field_Interface::select(Collection_Hndl * from) { // start edit
		auto fieldInterface_h = static_cast<Field_Interface_h&>(*from);
		if (fieldInterface_h.onSelect().targetIsSet()) {
			fieldInterface_h.onSelect()();
		} 
		else {
			edit(from);
		}
		return 0;
	}

	Collection_Hndl * I_Field_Interface::edit(Collection_Hndl * from) {
		editItem().setBackUI(from);
		parent()->set_focus(editItem().getEditCursorPos());
		addBehaviour(Behaviour::b_ViewAll);
		parent()->setCursorMode(HI_BD::e_inEdit);
		return 0;
	}


	int I_Field_Interface::setInitialCount(Field_Interface_h * parent) {
		_parent = parent;
		int initialFocus;
		if (parent->editBehaviour().is_Editable()) {
			initialFocus = editItem().gotFocus(_wrapper); // gotFocus copies data to currValue
		}
		else {
			UI_FieldData * fieldData = parent->getData();
			initialFocus = editItem().editMemberSelection(_wrapper, fieldData->data()->recordID());
		}
		setCount(editItem().currValue().valRange.editablePlaces);
		return initialFocus; // return focus for initial edit
	}

	int I_Field_Interface::getFieldWidth() const {
		return _wrapper->valRange.editablePlaces;
	}

	Collection_Hndl * I_Field_Interface::item(int elementIndex) {
		return &editItem();
	}

	ValRange I_Field_Interface::getValRange() const { 
		if (_wrapper == 0) return 0; 
		return _wrapper->valRange;
	}

	decltype(I_UI_Wrapper::val) I_Field_Interface::getData(bool isActiveElement) const {
		if (_wrapper == 0) return 0;
		auto streamVal = _wrapper->val;
		if (isActiveElement) {
			auto fieldInterface_h = parent();
			if (fieldInterface_h && fieldInterface_h->cursor_Mode() == HardwareInterfaces::LCD_Display::e_inEdit) { // any item may be in edit
				streamVal = editItem().currValue().val;
				//logger() << "edit value", streamVal);
			}
			else const_cast<I_Edit_Hndl&>(editItem()).currValue().val = streamVal;
		}
		return streamVal;
	}

	// *********************************************
	//             Field_Interface_h 
	// *********************************************

	Field_Interface_h::Field_Interface_h(I_Field_Interface & fieldInterfaceObj, Behaviour editBehaviour, int fieldID, UI_FieldData * parent, OnSelectFnctr onSelect)
		: Collection_Hndl(fieldInterfaceObj), _editBehaviour(editBehaviour), _fieldID(fieldID), _data(parent), _onSelect(onSelect)
	{
		fieldInterfaceObj.behaviour() = viewOneSelectable();
	}

	void Field_Interface_h::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_onSelect.setTarget(obj);
	}

	const char * Field_Interface_h::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		auto cursor_mode = cursorMode(activeElement);
		if (activeElement && !cursor_mode) activeElement = 0;

		const char * dataStream = f_interface().streamData(activeElement);
#ifdef ZPSIM
		//logger() << "\tstreamElement " << dataStream << L_tabs
			//<< "\n\t\tObjectIndex: " << f_interface().objectIndex() << L_endl;
#endif		
		const UI_Object * streamObject = backUI() ? backUI()->get() : get();
		return streamObject->streamToBuffer(dataStream, buffer, activeElement, shortColl, streamIndex);
	}

	bool Field_Interface_h::move_focus_by(int nth) {
		bool hasMoved = Collection_Hndl::move_focus_by(nth);
		f_interface().haveMovedTo(focusIndex()); // required even if not moved for editInts
		if (hasMoved) {
			set_focus(f_interface().editItem().cursorFromFocus(focusIndex()));
		}
		return hasMoved;
	}

	CursorMode Field_Interface_h::cursorMode(const Object_Hndl * activeElement) const {
		if (!activeElement) return HI_BD::e_unselected;
		if (this == activeElement) {
			if (_cursorMode == HI_BD::e_inEdit) return HI_BD::e_inEdit;
			else return HI_BD::e_selected;
		} else return _cursorMode;
	}

	void Field_Interface_h::setCursorPos() { 
		// only called when it gets the focus - i.e selected, not in edit
		auto initialFocus = f_interface().setInitialCount(this); // Sets endIndex() for number of edit-positions and returns initial edit-focus 
		set_focus(initialFocus);
	}

	void Field_Interface_h::setEditFocus(int focus) {
		set_focus(f_interface().editItem().cursorFromFocus(focus));
	}

	Collection_Hndl * Field_Interface_h::on_select() {
		backUI()->getItem(backUI()->focusIndex());
		auto fieldData = backUI()->get()->collection();
		static_cast<UI_FieldData*>(fieldData)->saveEdit(&f_interface().editItem().currValue());
		backUI()->on_select();
		return on_back();
	}

	Collection_Hndl * Field_Interface_h::on_back() {// function is called on the focus() object to notify it has been cancelled.
		Collection_Hndl * retVal = this;
		if (_cursorMode == HI_BD::e_inEdit) {
			setCursorMode(HI_BD::e_unselected);
			get()->removeBehaviour(Behaviour::b_ViewAll);
			f_interface().editItem().setBackUI(0);
			f_interface().setCount(0);
			retVal = 0;
			backUI()->on_back();
			f_interface().item(focusIndex());
		}
		setBackUI(0);
		return retVal;
	}

}