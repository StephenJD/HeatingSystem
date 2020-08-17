#pragma once
#include "Arduino.h"

namespace LCD_UI {
	extern const char NEW_LINE_CHR;

	/// <summary>
	/// 232: view, sel, ViewOne, LR-MoveFocus, UD-Edit, Recycle
	/// 228: view, sel, ViewOne, LR-MoveFocus, UD-NextActive, Recycle
	/// 226: view, sel, ViewOne, LR-MoveFocus, No-UD, Recycle
	/// 224: view, sel, ViewOne, LR-MoveFocus, No-UD, Recycle
	/// 210: view, sel, ViewAll, LR-NextActive, No-UD, No-recycle
	/// 208: view, sel, ViewAll, LR-NextActive, No-UD, Recycle
	/// 198: view, sel, ViewAll, LR-MoveFocus, UD-NextActive, No-Recycle
	/// 196: view, sel, ViewAll, LR-MoveFocus, UD-NextActive, Recycle
	/// 194: view, sel, ViewAll, LR-MoveFocus, No-UD, No-Recycle
	/// 192: view, sel, ViewAll, LR-MoveFocus, No-UD, Recycle
	/// </summary>
	class Behaviour {
	public:
		// Flags must have 1 for filterable attributes (i.e. Visible & Selectible). Otherwise use 0 for default values.
		enum BehaviourFlags : uint8_t { b_NewLine = 1, b_NonRecycle = 2, b_UD_NextActive = 4, b_UD_Edit = 8, b_UD_SaveEdit = b_UD_NextActive + b_UD_Edit, b_LR_ActiveMember = 16, b_ViewOne = 32, b_Selectible = 64, b_Visible = 128 };
		Behaviour() = default;
		Behaviour(BehaviourFlags b) : _behaviour(b) {}
		explicit Behaviour(int b) : _behaviour(b) {}

		// Queries
		explicit operator uint8_t() const { return _behaviour; }
		bool is(Behaviour b) const { return (_behaviour & uint8_t(b)) == uint8_t(b); }

		bool is_viewable() const		{ return _behaviour >= b_Visible; }
		bool is_selectable() const		{ return _behaviour >= b_Visible + b_Selectible; } /*has cursor*/ 
		bool is_viewOne() const			{ return _behaviour & b_ViewOne; }
		bool is_viewAll() const			{ return !is_viewOne(); }
		bool is_UpDnAble() const		{ return (_behaviour & b_UD_NextActive) || (_behaviour & b_UD_Edit); }
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
		Behaviour make_visible() { _behaviour |= b_Visible; return *this; }
		Behaviour make_hidden() { _behaviour &= ~b_Visible; return *this; }
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
		return Behaviour{ uint8_t(lhs) | uint8_t(rhs) };
	}

	inline Behaviour operator + (Behaviour::BehaviourFlags lhs, Behaviour::BehaviourFlags rhs) {
		return Behaviour{ uint8_t(lhs) | uint8_t(rhs) };
	}	
	
	// Create Individual Behaviours. Default(0) is Hidden, View-all, Unselectable, UD-NextActiveMember, LR-MovesCursor, Recycle, Same-Line.  
	inline Behaviour hidden() { return Behaviour{ 0 }; }
	inline Behaviour viewable() { return Behaviour::b_Visible; } // Set flag required for filtering
	// Default combined behaviours
	inline Behaviour selectable() { return viewable() + Behaviour::b_Selectible; } // Set flags required for filtering
	inline Behaviour nextActiveMember_onLR() { return selectable() + Behaviour::b_LR_ActiveMember + Behaviour::b_NonRecycle; } // For iterated collection
	inline Behaviour nextActiveMember_onUpDn() { return selectable() + Behaviour::b_UD_NextActive; }
	inline Behaviour editActiveMember_onUpDn() { return selectable() + Behaviour::b_UD_Edit; }
	inline Behaviour saveEditActiveMember_onUpDn() { return selectable() + Behaviour::b_UD_SaveEdit; }
	inline Behaviour nonRecycle() { return selectable() + Behaviour::b_NonRecycle; }
	inline Behaviour newLine() { return viewable() + Behaviour::b_NewLine; }
	inline Behaviour viewOne() { return viewable() + Behaviour::b_ViewOne; }
	inline Behaviour viewOneRecycle() { return selectable() + nextActiveMember_onUpDn() + viewOne(); }
	inline Behaviour viewAllRecycle() { return selectable() + nextActiveMember_onUpDn(); }
	inline Behaviour viewAllNonRecycle() { return selectable() + nextActiveMember_onUpDn() + nonRecycle(); }
	inline Behaviour viewOneNonRecycle() { return selectable() + viewOneRecycle() + nonRecycle(); }
}