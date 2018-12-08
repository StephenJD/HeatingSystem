#pragma once
#include "I_Edit_Hndl.h"
#include "UI_LazyCollection.h"
#include "ValRange.h"

namespace LCD_UI {
	//class I_UI_Wrapper;
	class Field_Interface_h;
	class UI_FieldData;
	/// <summary>
	/// Abstract base-class provides streaming and editing interface for raw data from the database
	/// Derivations specialised for string, int, decimal, time etc.
	/// Class behaves like a UI_Object when not in edit and like an I_SafeCollection of edit-characters when in edit
	/// Derived objects are designed to be static-storage object shared by all fields of that type.
	/// The data to be streamed is provided by setting _wrapper to the required Dataset member.
	/// </summary>
	class I_Field_Interface : public I_SafeCollection { // Shared UI interface between raw data and the visible UI element
	public:
		// New Queries
		const I_Edit_Hndl & editItem() const { return const_cast<I_Field_Interface *>(this)->editItem(); }
		
		// Polymorphic Queries
		virtual const char * streamData(const Object_Hndl * activeElement) const = 0;
		bool isCollection() const override { return behaviour().is_viewAll(); }

		// Polymorphic Modifiers
		Collection_Hndl * item(int elementIndex) override; // used only when in edit
		Collection_Hndl * select(Collection_Hndl * from) override;
		Collection_Hndl * edit(Collection_Hndl * from) override;

		// New Queries
		Field_Interface_h * parent() const { return _parent; /* const_cast<Field_Interface_h *>(static_cast<const Field_Interface_h *>(editItem().backUI()));*/ }
		int getFieldWidth() const;
		int setInitialCount(Field_Interface_h * parent);
		ValRange getValRange() const;
		decltype(I_UI_Wrapper::val) getData(const Object_Hndl * activeElement) const;
		const I_UI_Wrapper * getWrapper() const { return _wrapper; }
		
		// New modifiers
		virtual I_Edit_Hndl & editItem() = 0;
		virtual void haveMovedTo(int currFocus) {}

		void setWrapper(I_UI_Wrapper * wrapper);
	protected:
		I_Field_Interface() : I_SafeCollection(0,viewAll()) {}
		I_UI_Wrapper * _wrapper;
		Field_Interface_h * _parent;
		static char scratch[81]; // Buffer for constructing strings of wrapped values
	};

	class Field_Interface_h : public Collection_Hndl {
	public:
		Field_Interface_h(I_Field_Interface & fieldInterfaceObj, Behaviour editBehaviour, int fieldID, UI_FieldData * parent, OnSelectFnctr onSelect = 0);
		
		// Polymorphic Queries
		const char * streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex = 0) const override;
		CursorMode cursorMode(const Object_Hndl * activeElement) const override;
		int cursorOffset(const char * data) const override { 
			return f_interface().editItem().currValue().valRange._cursorPos;
		}
		const Behaviour behaviour() const override { 
			bool inEdit = (_cursorMode == HardwareInterfaces::DisplayBuffer_I::e_inEdit);
			return inEdit ? _editBehaviour : Collection_Hndl::behaviour(); 
			//return Collection_Hndl::behaviour(); 
		}

		// Polymorphic Modifiers
		bool move_focus_by(int nth) override;
		Collection_Hndl * on_back() override;
		Collection_Hndl * on_select() override;
		void setCursorPos() override;

		// New Queries
		CursorMode cursor_Mode() const { return _cursorMode; }
		int fieldID() { return _fieldID; }
		Behaviour editBehaviour() const { return _editBehaviour; }
		UI_FieldData * getData() { return _data; }
		OnSelectFnctr & onSelect() { return _onSelect; }
		// New modifiers
		void setCursorMode(CursorMode mode) { _cursorMode = mode; }
		void set_OnSelFn_TargetUI(Collection_Hndl * obj);
		void setEditFocus(int focus);

		const I_Field_Interface & f_interface() const { return static_cast<const I_Field_Interface&>(*get()); }
		I_Field_Interface & f_interface() { return static_cast<I_Field_Interface&>(*get()); }
	private:
		CursorMode _cursorMode = HardwareInterfaces::DisplayBuffer_I::e_unselected;
		Behaviour _editBehaviour;
		const uint8_t _fieldID; // placed here to reduce padding
		UI_FieldData * _data;
		OnSelectFnctr _onSelect;
	};
}
