#include "I_Streaming_Tool.h"
#include "I_Record_Interface.h"
#include "UI_Primitives.h"
#include "UI_FieldData.h"
#include "Conversions.h"
#include <Logging.h>

#ifdef ZPSIM
#include <map>
using namespace std;
map<long, string> & ui_Objects();
#endif

namespace LCD_UI {
	using namespace GP_LIB;
	using HI_BD = HardwareInterfaces::LCD_Display;
	// *********************************************
	//             I_Streaming_Tool 
	// *********************************************

	void I_Streaming_Tool::setWrapper(I_Data_Formatter * wrapper) {
		_data_formatter = wrapper;
	}

	Collection_Hndl * I_Streaming_Tool::select(Collection_Hndl * from) { // start edit
		auto field_streamingTool_h = static_cast<Field_StreamingTool_h&>(*from);
		if (field_streamingTool_h.onSelect().targetIsSet()) {
			field_streamingTool_h.onSelect()();
		} 
		else {
			edit(from);
		}
		return 0;
	}

	Collection_Hndl * I_Streaming_Tool::edit(Collection_Hndl * from) {
		// Save handled by Field_StreamingTool_h::on_select() at the end of this file.
		// Cancel edit handled by Field_StreamingTool_h::on_back() at the end of this file.
		auto field_streamingTool_h = static_cast<Field_StreamingTool_h*>(from);
		editItem().setBackUI(from);
#ifdef ZPSIM
		cout << F("Edit Interface: ") << ui_Objects()[(long)this] << endl;
		cout << F("\tedit->back ") << ui_Objects()[(long)field_streamingTool_h->backUI()->get()] << endl; // Collection with field to be edited
		cout << F("\tedit->back->focus ") << field_streamingTool_h->backUI()->focusIndex() << endl;
#endif	
		auto sourceCollection = field_streamingTool_h->backUI()->get()->collection();
		if (sourceCollection->objectIndex() != sourceCollection->focusIndex()) { // get data loaded for iterated collections
			field_streamingTool_h->backUI()->move_focus_by(0); 
			editItem().gotFocus(getDataFormatter()); 
		}
		field_streamingTool_h->set_focus(editItem().getEditCursorPos()); // copy data to edit
		field_streamingTool_h->setCursorMode(HI_BD::e_inEdit);
		
		field_streamingTool_h->activeEditBehaviour().make_viewAll(); // Allows UP/DOWN to go to edit collection
		behaviour() = field_streamingTool_h->activeEditBehaviour();
		if (behaviour().is_next_on_UpDn()) {
			editItem().currValue().val = field_streamingTool_h->getData()->data()->record().id();
			field_streamingTool_h->getData()->data()->setEditMode(true);
		}
		return 0;
	}


	int I_Streaming_Tool::setInitialCount(Field_StreamingTool_h * parent) {
		_dataSource = parent;
		int initialEditFocus;
		initialEditFocus = editItem().gotFocus(_data_formatter); // gotFocus copies data to currValue
		setCount(editItem().currValue().valRange.editablePlaces);
		return initialEditFocus;
	}

	int I_Streaming_Tool::getFieldWidth() const {
		return _data_formatter->valRange.editablePlaces;
	}

	Collection_Hndl * I_Streaming_Tool::item(int) {
		return &editItem();
	}

	ValRange I_Streaming_Tool::getValRange() const { 
		if (_data_formatter == 0) return 0; 
		return _data_formatter->valRange;
	}

	decltype(I_Data_Formatter::val) I_Streaming_Tool::getData(bool isActiveElement) const {
		if (_data_formatter == 0) return 0;
		auto streamVal = _data_formatter->val;
		if (isActiveElement) {
			auto field_streamingTool_h = dataSource();
			if (field_streamingTool_h->cursor_Mode() == HardwareInterfaces::LCD_Display::e_inEdit) { // any item may be in edit
				streamVal = editItem().currValue().val;
			}
		}
		return streamVal;
	}

	bool I_Streaming_Tool::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, UI_DisplayBuffer::ListStatus listStatus) const {
		auto cursor_mode = dataSource()->cursorMode(activeElement);
		if (activeElement && cursor_mode == HI_BD::e_unselected) activeElement = 0;
		const char * dataStream = streamData(activeElement);	
		const UI_Object * streamObject = dataSource()->backUI() ? dataSource()->backUI()->get() : this;
		return streamObject->streamToBuffer(dataStream, buffer, activeElement, endPos, listStatus);
	}

	// *********************************************
	//             Field_StreamingTool_h 
	// *********************************************

	/// <summary>
	/// Points to the shared Streaming_Tool which performs streaming and editing.
	/// Has edit-behaviour member to determin the Streaming_Tool behaviour when in edit:
	/// activeEditBehaviour may be (default first) One(not in edit)/All(during edit), non-Recycle/Recycle, UD-Edit(edit member)/UD-Nothing(no edit)/UD-NextActive(change member).
	/// Can have an alternative Select function set.
	/// It may have a parent field set, from which it obtains its object index.
	/// </summary>
	Field_StreamingTool_h::Field_StreamingTool_h(I_Streaming_Tool & streamingTool, uint16_t activeEditBehaviour, int fieldID, UI_FieldData * parent, OnSelectFnctr onSelect)
		: Collection_Hndl(streamingTool), _activeEditBehaviour(V+S+V1+(activeEditBehaviour & EA ? UD_C : UD_E)+(activeEditBehaviour & ER0 ? R0 : R)), _fieldID(fieldID), _parentColln(parent), _onSelect(onSelect)
	{}

	const Behaviour Field_StreamingTool_h::behaviour() const {
		bool inEdit = (_cursorMode == HardwareInterfaces::LCD_Display::e_inEdit);
		auto non_UD_capturing_activeBehaviour = Behaviour(_activeEditBehaviour).make_UD_Cmd().make_viewOne();
		if (inEdit) {
			return _activeEditBehaviour; // .is_next_on_UpDn() ? Behaviour(_activeEditBehaviour).make_UD_Cmd() : Behaviour(_activeEditBehaviour).make_UD_NextActive();
		} else return non_UD_capturing_activeBehaviour;
	}

	void Field_StreamingTool_h::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_onSelect.setTarget(obj);
	}

	bool Field_StreamingTool_h::move_focus_by(int nth) {
		bool hasMoved = get()->move_focus_by(nth, this);
		f_interface().haveMovedTo(focusIndex()); // required even if not moved for editInts
		if (hasMoved) {
			set_focus(f_interface().editItem().cursorFromFocus(focusIndex()));
		}
		return hasMoved;
	}

	CursorMode Field_StreamingTool_h::cursorMode(const Object_Hndl * activeElement) const {
		if (!activeElement) return HI_BD::e_unselected;
		if (this == activeElement) {
			if (_cursorMode == HI_BD::e_inEdit) return HI_BD::e_inEdit;
			else return HI_BD::e_selected;
		} else return _cursorMode;
	}

	void Field_StreamingTool_h::setCursorPos() { 
		// Called directly by A_TopUI. Only called when it gets the focus - i.e selected, not in edit
		getData()->item(getData()->focusIndex());
		auto initialEditFocus = f_interface().setInitialCount(this); // Copies data to wrapper.currentValue, Sets endIndex() for number of edit-positions and returns initial edit-focus 
		set_focus(initialEditFocus);
	}

	void Field_StreamingTool_h::setEditFocus(int focus) {
		set_focus(f_interface().editItem().cursorFromFocus(focus));
	}

	Collection_Hndl * Field_StreamingTool_h::on_select() { // Saves edit
#ifdef ZPSIM
		cout << F("on_select ") << ui_Objects()[(long)backUI()->get()] << endl;
#endif	
		backUI()->getItem(backUI()->focusIndex());
		auto fieldData = backUI()->get()->collection();		
		static_cast<UI_FieldData*>(fieldData)->saveEdit(&f_interface().editItem().currValue());
		backUI()->on_select();
		return on_back();
	}

	Collection_Hndl * Field_StreamingTool_h::on_back() {// function is called on the focus() object to notify it has been cancelled.
		Collection_Hndl * retVal = this;
		if (_cursorMode == HI_BD::e_inEdit) {
			setCursorMode(HI_BD::e_unselected);
			get()->behaviour().make_viewOne(); // Prevents LR going to the SteamingTool
			getData()->data()->setEditMode(false); 
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