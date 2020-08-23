#pragma once
#include "I_Edit_Hndl.h"
#include "UI_Collection.h"
#include "I_Data_Formatter.h"

namespace LCD_UI {
	//class I_Data_Formatter;
	class Field_StreamingTool_h;
	class UI_FieldData;
	/// <summary>
	/// Abstract base-class provides streaming and editing for raw data from the database
	/// Derivations specialised for string, int, decimal, time etc.
	/// Class behaves like a UI_Object when not in edit and like an I_SafeCollection of edit-characters when in edit
	/// Derived objects are designed to be static-storage object shared by all fields of that type.
	/// The data to be streamed is provided by setting _data_formatter to the required Dataset member.
	/// </summary>
	class I_Streaming_Tool : public I_SafeCollection { // Shared UI interface between raw data and the visible UI element
	public:
		// New Queries
		const I_Edit_Hndl & editItem() const { return const_cast<I_Streaming_Tool *>(this)->editItem(); }
		
		// Polymorphic Queries
		virtual const char * streamData(bool isActiveElement) const = 0;
		bool isCollection() const override { return behaviour().is_viewAll(); }

		// Polymorphic Modifiers
		Collection_Hndl * item(int elementIndex) override; // used only when in edit
		Collection_Hndl * select(Collection_Hndl * from) override;
		Collection_Hndl * edit(Collection_Hndl * from) override;
		Field_StreamingTool_h * dataSource() { return _dataSource; }

		// New Queries
		const Field_StreamingTool_h * dataSource() const { return _dataSource; }
		int getFieldWidth() const;
		int setInitialCount(Field_StreamingTool_h * parent);
		ValRange getValRange() const;
		decltype(I_Data_Formatter::val) getData(bool isActiveElement) const;
		const I_Data_Formatter * getDataFormatter() const { return _data_formatter; }
		
		// New modifiers
		virtual I_Edit_Hndl & editItem() = 0;
		virtual void haveMovedTo(int currFocus) {}

		void setWrapper(I_Data_Formatter * wrapper);
	protected:
		I_Streaming_Tool() : I_SafeCollection(0, Behaviour{V+S+V1}) {
#ifdef ZPSIM
		logger() << F("\tI_Field_Interface Addr: ") << L_hex << long(this) << L_endl;
#endif
		}
		I_Data_Formatter * _data_formatter;
		Field_StreamingTool_h * _dataSource;
		static char scratch[81]; // Buffer for constructing strings of wrapped values
	};

	/// <summary>
	/// Points to the shared Streaming_Tool-Collection which performs streaming and editing.
	/// Has active-edit-behaviour member to modify the Streaming_Tool-Collection behaviour when in edit:
	/// activeEditBehaviour may be (default first) One(not in edit)/All(during edit), non-Recycle/Recycle, UD-Edit(edit member)/UD-Nothing(no edit)/UD-NextActive(change member).
	/// Can have an alternative Select function set.
	/// It may have a parent field set, from which it obtains its object index.
	/// </summary>
	class Field_StreamingTool_h : public Collection_Hndl { // base-class points to streamingTool
	public:
		Field_StreamingTool_h(I_Streaming_Tool & streamingTool, Behaviour activeEditBehaviour, int fieldID, UI_FieldData * parent, OnSelectFnctr onSelect = 0);
		
		// Polymorphic Queries
		bool streamElement_h(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override;
		CursorMode cursorMode(const Object_Hndl * activeElement) const override;
		int cursorOffset(const char * data) const override { 
			return f_interface().editItem().currValue().valRange._cursorPos;
		}
		const Behaviour behaviour() const override;

		// Polymorphic Modifiers
		bool move_focus_by(int nth) override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
		void setCursorPos() override;

		// New Queries
		CursorMode cursor_Mode() const { return _cursorMode; }
		int fieldID() const { return _fieldID; }
		const UI_FieldData * getData() const { return _parentColln; }
		Behaviour activeEditBehaviour() const { return _activeEditBehaviour; }
		// New modifiers
		UI_FieldData * getData() { return _parentColln; }
		OnSelectFnctr & onSelect() { return _onSelect; }
		Behaviour & activeEditBehaviour() { return _activeEditBehaviour; }
		void setCursorMode(CursorMode mode) { _cursorMode = mode; }
		void set_OnSelFn_TargetUI(Collection_Hndl * obj);
		void setEditFocus(int focus);

		const I_Streaming_Tool & f_interface() const { return static_cast<const I_Streaming_Tool&>(*get()); }
		I_Streaming_Tool & f_interface() { return static_cast<I_Streaming_Tool&>(*get()); }
	private:
		CursorMode _cursorMode = HardwareInterfaces::LCD_Display::e_unselected;
		Behaviour _activeEditBehaviour;
		const uint8_t _fieldID; // placed here to reduce padding
		UI_FieldData * _parentColln; // LazyCollection
		OnSelectFnctr _onSelect;
	};
}
