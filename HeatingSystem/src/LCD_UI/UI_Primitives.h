#pragma once
#include "I_Data_Formatter.h"
#include "I_Streaming_Tool.h"
#include "I_Edit_Hndl.h"
#include "UI_Collection.h"

#ifdef ZPSIM
	#include <iostream>
	#include <iomanip>
#endif

namespace LCD_UI {

	enum wrapTypes { e_WrapString, e_WrapInt };

	///////////////////////////////////////////////////////////////////////////////////////////////
	//               Wrapper classes for Primitive types adding formatting info
	///////////////////////////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// This Object derivative wraps Integer values.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static Int_Interface object via ui().
	/// </summary>
	class IntWrapper : public I_Data_Formatter /* from I_Data_Formatter.h */ {
	public:
		IntWrapper() = default;
		IntWrapper(int32_t val, ValRange valRangeArg) : I_Data_Formatter(val, valRangeArg) {}
		IntWrapper & operator= (const I_Data_Formatter & wrapper) { I_Data_Formatter::operator=(wrapper); return *this; }
		I_Streaming_Tool & ui() override;
	};

	/// <summary>
	/// This Object derivative wraps integer data with string-labels.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static Options_Interface object via ui().
	/// </summary>
	class OptionsWrapper : public I_Data_Formatter {
	public:
		OptionsWrapper() = default;
		OptionsWrapper(int32_t val, ValRange valRangeArg, const char** options) : I_Data_Formatter(val, valRangeArg), _options(options) {}
		OptionsWrapper& operator= (const I_Data_Formatter & wrapper) { I_Data_Formatter::operator=(wrapper); return *this; }

		I_Streaming_Tool& ui() override;
		const char* option(int i) const { return _options[i]; }
	private:
		const char** _options;
	};

	/// <summary>
	/// This Object derivative wraps fixed-point Integer values displayed as decimals.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static Decimal_Interface object via ui().
	/// </summary>
	class DecWrapper : public I_Data_Formatter {
	public:
		DecWrapper() = default;
		DecWrapper(int32_t val, ValRange valRangeArg) : I_Data_Formatter(val, valRangeArg) {}
		DecWrapper & operator= (const I_Data_Formatter & wrapper) { I_Data_Formatter::operator=(wrapper); return *this; }
		I_Streaming_Tool & ui() override;
	};

	/// <summary>
	/// This Object derivative wraps c-strings.
	/// It is constructed with the value and a ValRange formatting object.
	/// It provides streaming and editing by delegation to a file-static String_Interface object via ui().
	/// </summary>
	class StrWrapper : public I_Data_Formatter {
	public:
		StrWrapper() = default;
		StrWrapper(const char* strVal, ValRange valRangeArg);
		StrWrapper & operator= (const char* strVal);
		bool operator== (const char* strVal);

		I_Streaming_Tool & ui() override;
		char * str() { return _str; }
		const char * str() const { return _str; }
	private:
		char _str[15];
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 Permitted Vals Classes
	/////////////////////////////////////////////////////////////////////////////////////////
	class OneVal : public Collection_Hndl, public UI_Object {
	public:
		OneVal(I_SafeCollection * parent) : Collection_Hndl((const UI_Object*)this), _parent(parent) {
#ifdef ZPSIM
			ui_Objects()[this] = "OneVal";
#endif
		};
		void setParent(I_SafeCollection * parent) { _parent = parent; }
		Collection_Hndl * select(Collection_Hndl * from) { return _parent->select(backUI()); }
		Collection_Hndl * on_back() override { return backUI()->on_back(); }
		using UI_Object::behaviour;
		Behaviour & behaviour() override { return _behaviour; }
	private:
		I_SafeCollection * _parent;
		Behaviour _behaviour = {V+V1+R+UD_A};
	};

	/// <summary>
	/// This Collection_Hndl derivative provides the range of permissible values during an edit.
	/// count() and focus() not used.
	/// </summary>
	class Permitted_Vals : public I_SafeCollection {
	public:
		Permitted_Vals() :I_SafeCollection(0, Behaviour{V+S+V1+R+UD_A}), _oneVal(this) {
#ifdef ZPSIM
			ui_Objects()[this] = "Permitted_Vals";
#endif
		}
		Collection_Hndl * select(Collection_Hndl * from) override;

		Object_Hndl * item(int index) override {return &_oneVal;} // returns object reference at index.
	private:
		/*static*/ OneVal _oneVal;
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//                 Edit Classes manage a field during edits
	/////////////////////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// This Collection_Hndl derivative provides editing of Integer fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_Int_h : public I_Edit_Hndl {
	public:
		Edit_Int_h();
		int gotFocus(const I_Data_Formatter * data) override; // returns select focus
		bool move_focus_by(int moveBy) override; // move focus to next charater during edit
		I_Data_Formatter & currValue() override { return _currValue; }
		void setInRangeValue() override;
		void setDecade(int focus);
		int cursorFromFocus(int = 0) override; // arg not used. Sets CursorPos

	private:
		IntWrapper _currValue; // copy value for editing
		Permitted_Vals /*&*/ editVal /*= _editVal*/;
		int _decade;
	};

	class String_Interface;
	/// <summary>
	/// This Collection_Hndl derivative provides editing of String fields.
	/// It is the recipient during edits and holds a copy of the data.
	/// It maintains the focus for editing within a field.
	/// The activeUI is the associated PermittedValues object.
	/// </summary>
	class Edit_Char_h : public I_Edit_Hndl {
	public:
		Edit_Char_h();
		int gotFocus(const I_Data_Formatter * data) override; // returns select focus
		int editMemberSelection(const I_Data_Formatter * _data_formatter, int recordID) override; // returns select focus
		int getEditCursorPos() override;  // copy data to edit
		I_Data_Formatter & currValue() override { return _currValue; }
		bool move_focus_by(int moveBy) override; // move focus to next charater during edit
	private:
		friend String_Interface;
		StrWrapper _currValue; // copy of data for editing which is saved when finished
		Permitted_Vals /*&*/ editChar /*= _editVal*/;
		char stream_edited_copy[15]; // copy of _currValue for streaming when in edit - includes |||
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	//                      UI classes for Primitive types
	///////////////////////////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// This LazyCollection derivative is the UI for Integers
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the integer.
	/// </summary>
	class Int_Interface : public I_Streaming_Tool { // Shared stateless UI interface
	public:
#ifdef ZPSIM
		Int_Interface() { ui_Objects()[this] = "Int_Interface"; }
#endif
		using I_Streaming_Tool::editItem;
		void haveMovedTo(int currFocus) override;

		const char * streamData(bool isActiveElement) const override;
		I_Edit_Hndl & editItem() override { return _editItem; }
	protected:
		Edit_Int_h _editItem;
	};	
	
	/// <summary>
	/// This LazyCollection derivative is the UI for Options
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the underlying integer.
	/// </summary>
	class Option_Interface : public I_Streaming_Tool { // Shared stateless UI interface
	public:
#ifdef ZPSIM
		Option_Interface() { ui_Objects()[this] = "Option_Interfacet_Interface"; }
#endif
		using I_Streaming_Tool::editItem;

		const char * streamData(bool isActiveElement) const override;
		I_Edit_Hndl & editItem() override { return _editItem; }
	protected:
		Edit_Int_h _editItem;
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for Fixed-point decimals
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like a LazyCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the value.
	/// </summary>
	class Decimal_Interface : public Int_Interface {
	public:
#ifdef ZPSIM
		Decimal_Interface() { ui_Objects()[this] = "Decimal_Interface"; }
#endif
		const char * streamData(bool isActiveElement) const override;
	};

	/// <summary>
	/// This LazyCollection derivative is the UI for C-Strings
	/// Base-class setWrapper() points the object to the wrapped value.
	/// It behaves like a UI_Object when not in edit and like an I_SafeCollection of field-characters when in edit.
	/// It provides streaming and delegates editing of the string.
	/// </summary>
	class String_Interface : public I_Streaming_Tool {
	public:
#ifdef ZPSIM
		String_Interface() { ui_Objects()[this] = "String_Interface"; }
#endif
		using I_Streaming_Tool::editItem;
		const char * streamData(bool isActiveElement) const override;
		I_Edit_Hndl & editItem() { return _editItem; }
	private:
		Edit_Char_h _editItem;
	};
}