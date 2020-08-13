#pragma once
#include "Arduino.h"

namespace LCD_UI {
	extern const char NEW_LINE_CHR;

	class Behaviour {
	public:
		//enum BehaviourFlags : uint8_t { b_Hidden, b_NewLine, b_Viewable = 2, b_Selectable = 4, b_ViewAll = 8, b_NextItemOnUpDown = 16, b_EditOnUpDn = 32, b_Recycle = 64, b_NotEditable = 128 };
		enum BehaviourFlags : uint8_t { b_NewLine = 1, b_NonRecycle = 2, b_spare1 = 4, b_spare2 = 8, b_UD_NextActive = 16, b_UD_Edit = 32, b_UD_SaveEdit = b_UD_NextActive + b_UD_Edit, b_LR_ActiveMember = 64, b_ViewOne = 128, b_Hidden = b_ViewOne };
		// Largest value for least often encountered : Selectible == (b_UD_NextActive || b_UD_Edit). Hidden == (b_ViewOne && NotCollection), Viewable == (IsCollection || !b_ViewOne)
		// 110 :view, sel, ViewAll, EditOnNextItem, Recycle
		// 94 : view, sel, ViewAll, UpDn, Recycle
		// 86 : view, sel, ViewOne, UpDn, Recycle
		// 78 : view, sel, ViewAll, Recycle
		// 38 : view, sel, ViewOne, UpDn
		// 30 : view, sel, ViewAll, UpDn
		// 22 : view, sel, ViewOne, UpDn
		// 21 : newLine, sel, UpDown
		Behaviour() = default;
		explicit Behaviour(int b) : _behaviour(b) {}

		// Queries
		explicit operator uint8_t() const { return _behaviour; }
		bool is(Behaviour b) const { return (_behaviour & uint8_t(b)) == uint8_t(b); }

		bool is_viewable() const		{ return _behaviour < b_Hidden; }
		bool is_selectable() const /*has cursor*/ { return _behaviour >= b_UD_NextActive; }
		bool is_viewAll() const			{ return _behaviour < b_ViewOne; }
		bool is_viewOne() const			{ return _behaviour >= b_ViewOne; }
		//bool is_UpDnAble() const		{ return is_selectable(); }
		bool is_next_on_UpDn() const	{ return (_behaviour & b_UD_NextActive) && !(_behaviour & b_UD_Edit); }
		bool is_edit_on_UD() const		{ return _behaviour & b_UD_Edit; }
		bool is_save_on_UD() const		{ return is(Behaviour{ b_UD_NextActive | b_UD_Edit }); }
		bool is_viewOneUpDn_Next() const { return is_viewOne() && is_next_on_UpDn(); }
		bool is_recycle() const			{ return !(_behaviour & b_NonRecycle); }
		bool is_OnNewLine() const		{ return _behaviour & b_NewLine; }
		bool is_NextActive_On_LR() const{ return _behaviour & b_LR_ActiveMember; }
		bool is_MoveFocus_On_LR() const	{ return !is_NextActive_On_LR(); }

		//bool is_Editable() const		{ return (_behaviour & b_NotEditable) == 0; }
		// Modifiers
		Behaviour & operator = (int b) { _behaviour = b; return *this; }
		Behaviour make_hidden() { _behaviour |= b_Hidden; return *this; }
		Behaviour make_visible() { _behaviour &= ~b_Hidden; return *this; }
		Behaviour make_visible(bool show) {return show ? make_visible() : make_hidden(); }
		Behaviour make_newLine() { _behaviour |= b_NewLine; return *this; }
		Behaviour make_newLine(bool newLine) { return newLine ? make_newLine() : make_sameLine(); }
		Behaviour make_sameLine() { _behaviour &= ~b_NewLine; return *this; }
		Behaviour make_viewOne() { _behaviour |= b_ViewOne; return *this; }
		Behaviour make_viewAll() { _behaviour &= ~b_ViewOne; return *this; }
		Behaviour make_editOnUpDn() { _behaviour |= b_UD_Edit; return *this; }
		Behaviour make_saveOnNext() { return addBehaviour(BehaviourFlags(b_UD_NextActive | b_UD_Edit)); }
		//Behaviour make_unEditable() { _behaviour |= b_NotEditable; return *this; }
		Behaviour addBehaviour(BehaviourFlags b) { _behaviour |= b; return *this;  }
		Behaviour removeBehaviour(BehaviourFlags b) { _behaviour &= ~b; return *this;  }
	private:
		uint8_t _behaviour = 0;
	};

	inline Behaviour operator + (Behaviour lhs, Behaviour rhs) {
		return Behaviour{ uint8_t(lhs) + uint8_t(rhs) };
	}
	// Create Individual Behaviours. Default is Visible, View-all, Unselectable, UD-NextActiveMember, LR-MovesCursor, Recycle, Same-Line.  
	inline Behaviour hidden() { return Behaviour{ Behaviour::b_Hidden }; }
	inline Behaviour viewable() { return Behaviour{ 0 }; }
	inline Behaviour viewOneRecycle() { return Behaviour{ Behaviour::b_ViewOne }; }
	inline Behaviour nextActiveMember_onLR() { return Behaviour{ Behaviour::b_LR_ActiveMember }; }
	inline Behaviour nextActiveMember_onUpDn() { return Behaviour{ Behaviour::b_UD_NextActive }; }
	inline Behaviour editActiveMember_onUpDn() { return Behaviour{ Behaviour::b_UD_Edit }; }
	inline Behaviour saveEditActiveMember_onUpDn() { return Behaviour{ Behaviour::b_UD_SaveEdit }; }
	inline Behaviour selectable() { return nextActiveMember_onUpDn(); }
	inline Behaviour nonRecycle() { return Behaviour{ Behaviour::b_NonRecycle }; }
	inline Behaviour newLine() { return Behaviour{ Behaviour::b_NewLine }; }
	// Default combined behaviours
	inline Behaviour viewAllRecycle() { return nextActiveMember_onUpDn(); }
	inline Behaviour viewAllNonRecycle() { return nextActiveMember_onUpDn() + nonRecycle(); }
	//inline Behaviour editNonRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll }; }

	//inline Behaviour viewable() /*no cursor*/ { return Behaviour{ Behaviour::b_Viewable }; }
	//inline Behaviour viewOneRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_NextItemOnUpDown }; }
	//inline Behaviour viewAllUpDn() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll  | Behaviour::b_NextItemOnUpDown}; }
	//inline Behaviour viewAllUpDnRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll | Behaviour::b_NextItemOnUpDown | Behaviour::b_Recycle }; }
	//// Edit Behaviours
	//inline Behaviour editRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll | Behaviour::b_Recycle }; }

}