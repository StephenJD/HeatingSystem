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
		enum BehaviourFlags : uint8_t { b_NewLine = 1, b_NonRecycle = 2, b_UD_NextActive = 4, b_UD_Edit = 8, b_UD_SaveEdit = b_UD_NextActive + b_UD_Edit, b_LR_Captured = 16, b_ViewOne = 32, b_Selectible = 64, b_Visible = 128 };

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
		bool is_CaptureLR() const{ return _behaviour & b_LR_Captured; }

		// Modifiers
		Behaviour & operator = (int b) { _behaviour = b; return *this; }
		Behaviour make_visible() { _behaviour |= b_Visible; return *this; }
		Behaviour make_hidden() { _behaviour &= ~b_Visible; return *this; }
		Behaviour make_visible(bool show) {return show ? make_visible() : make_hidden(); }
		Behaviour make_newLine() { _behaviour |= b_NewLine; return *this; }
		Behaviour make_newLine(bool newLine) { return newLine ? make_newLine() : make_sameLine(); }
		Behaviour make_viewOne() { _behaviour |= b_ViewOne; return *this; }
		Behaviour make_viewAll() { _behaviour &= ~b_ViewOne; return *this; }
		Behaviour make_noUD() { _behaviour &= ~(b_UD_NextActive | b_UD_Edit); return *this; }
		Behaviour make_noRecycle() { _behaviour |= b_NonRecycle; return *this; }

	private:
		Behaviour make_sameLine() { _behaviour &= ~b_NewLine; return *this; }
		uint8_t _behaviour = 0;
	};

	enum BehaviourAliases : uint8_t {
		H = 0, V = Behaviour::b_Visible
		, L0 = 0, L = Behaviour::b_NewLine
		, S0 = 0, S = Behaviour::b_Selectible
		, Vn = 0, V1 = Behaviour::b_ViewOne
		, R = 0, R0 = Behaviour::b_NonRecycle
		, LR_0 = 0, LR = Behaviour::b_LR_Captured
		, UD_0 = 0, UD_A = Behaviour::b_UD_NextActive, UD_E = Behaviour::b_UD_Edit, UD_S = Behaviour::b_UD_SaveEdit
	};
	
	inline Behaviour::BehaviourFlags operator + (BehaviourAliases lhs, BehaviourAliases rhs) {
		return Behaviour::BehaviourFlags( uint8_t(lhs) | uint8_t(rhs));
	}	
	
	inline Behaviour::BehaviourFlags operator + (Behaviour::BehaviourFlags lhs, BehaviourAliases rhs) {
		return Behaviour::BehaviourFlags( uint8_t(lhs) | uint8_t(rhs) );
	}

	inline Behaviour::BehaviourFlags operator + (Behaviour lhs, BehaviourAliases rhs) {
		return Behaviour::BehaviourFlags( uint8_t(lhs) | uint8_t(rhs) );
	}

	inline Behaviour filter_viewable() { return Behaviour::b_Visible; } // Set flag required for filtering
	inline Behaviour filter_selectable() { return V+S; } // Set flags required for filtering
}