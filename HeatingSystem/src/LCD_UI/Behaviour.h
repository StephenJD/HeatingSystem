#pragma once
#include "Arduino.h"

namespace LCD_UI {
	extern const char NEW_LINE_CHR;

	/// <summary>
	/// 244: view, sel, ViewOneLR0, UD-Active, 	Recycle, 	IT-NoRecycle
	/// 238: view, sel, ViewOneLR0, UD-Save, 	No-Recycle
	/// 234: view, sel, ViewOneLR0, UD-Edit, 	No-Recycle
	/// 232: view, sel, ViewOneLR0, UD-Edit, 	Recycle
	/// 228: view, sel, ViewOneLR0, UD-Active, 	Recycle
	/// 224: view, sel, ViewOneLR0, UD_Cmd, 	Recycle
	/// 212: view, sel, ViewAll_LR, UD-Active, 	Recycle,	IT-NoRecycle
	/// 208: view, sel, ViewAll_LR, UD_Cmd, 	Recycle,	IT-NoRecycle
	/// 204: view, sel, ViewAll_LR, UD_Save, 	Recycle
	/// 202: view, sel, ViewAll_LR, UD_Edit, 	No-Recycle
	/// 200: view, sel, ViewAll_LR, UD_Edit, 	Recycle
	/// 198: view, sel, ViewAll_LR, UD-Active, 	No-Recycle
	/// 196: view, sel, ViewAll_LR, UD-Active, 	Recycle
	/// 194: view, sel, ViewAll_LR, UD_Cmd, 	No-Recycle
	/// 192: view, sel, ViewAll_LR, UD_Cmd, 	Recycle
	/// </summary>
	class Behaviour {
	public:
		// Flags must have 1 for filterable attributes (i.e. Visible & Selectable). Otherwise use 0 for default values.
		enum BehaviourFlags : uint16_t { b_NewLine = 1, b_NonRecycle = 2, b_UD_Active = 4, b_UD_Edit = 8, b_UD_SaveEdit = b_UD_Active + b_UD_Edit, b_IteratedNoRecycle = 16, b_ViewOne = 32, b_Selectable = 64, b_Visible = 128, b_EditNoRecycle = 256, b_Edit_Active = 512 };

		Behaviour() = default;
		Behaviour(BehaviourFlags b) : _behaviour(b) {}
		explicit Behaviour(int b) : _behaviour(b) {}

		// Queries
		explicit operator uint16_t() const { return _behaviour; }
		explicit operator uint8_t() const { return _behaviour; }
		bool is(uint16_t b) const		{ return (_behaviour & b) == b; }
		bool is(Behaviour b) const		{ return (_behaviour & uint8_t(b)) == uint8_t(b); }

		bool is_viewable() const		{ return _behaviour & b_Visible; }
		bool is_selectable() const		{ return is(b_Selectable + b_Visible); } /*has cursor*/
		bool is_viewOne() const			{ return is(b_Visible + b_ViewOne); }
		bool is_viewAll_LR() const		{ return is_viewable() && !is_viewOne(); }
		//bool captureUD() const		{ return (_behaviour & b_UD_Active) || (_behaviour & b_UD_Edit); }
		bool captureUD() const			{ return is_selectable() /*&& (is_viewOne() || (is_viewAll_LR() && is_edit_on_UD()))*/; }
		bool is_next_on_UpDn() const	{ return (_behaviour & b_UD_Active) && !(_behaviour & b_UD_Edit); }
		bool is_edit_on_UD() const		{ return _behaviour & b_UD_Edit; }
		bool is_save_on_UD() const		{ return is(b_UD_Active + b_UD_Edit); }
		bool is_cmd_on_UD() const		{ return !(_behaviour & (b_UD_Active | b_UD_Edit)); }
		//bool is_viewOneUpDn_Next() const { return is_viewOne() && is_next_on_UpDn(); }
		bool is_viewOneUpDn_Next() const { return is_viewOne() && !is_edit_on_UD(); }
		bool is_recycle() const			{ return !(_behaviour & b_NonRecycle); }
		bool is_OnNewLine() const		{ return _behaviour & b_NewLine; }
		bool is_CaptureLR() const		{ return is_viewAll_LR(); }
		bool is_EditNoRecycle() const	{ return _behaviour & b_EditNoRecycle; }
		bool is_IterateOne() const		{ return is_viewAll_LR() && is_next_on_UpDn(); }
		bool is_IteratedRecycle() const	{ return !(_behaviour & b_IteratedNoRecycle); }

		// Modifiers
		Behaviour& operator = (int b)	{ _behaviour = b; return *this; }
		Behaviour& make_visible()		{ _behaviour |= b_Visible; return *this; }
		Behaviour& make_hidden()		{ _behaviour &= ~b_Visible; return *this; }
		Behaviour& make_unselectable()	{ _behaviour &= ~b_Selectable; return *this; }
		Behaviour& make_visible(bool show) { return show ? make_visible() : make_hidden(); }
		Behaviour& make_newLine()		{ _behaviour |= b_NewLine; return *this; }
		Behaviour& make_newLine(bool newLine) { return newLine ? make_newLine() : make_sameLine(); }
		Behaviour& make_viewOne()		{ _behaviour |= b_ViewOne; return *this; }
		Behaviour& make_viewAll()		{ _behaviour &= ~b_ViewOne; return *this; }
		Behaviour& make_UD_NextActive()	{ _behaviour |= b_UD_Active; _behaviour &= ~(b_UD_Edit); return *this; }
		Behaviour& make_EditUD()		{ _behaviour |= b_UD_Edit; return *this; }
		Behaviour& make_UD_Cmd()		{ _behaviour &= ~(b_UD_Active | b_UD_Edit); return *this; }
		Behaviour& make_noRecycle()		{ _behaviour |= b_NonRecycle; return *this; }
		Behaviour& copyUD(Behaviour ud)	{ make_UD_Cmd(); _behaviour |= (uint8_t(ud) & (b_UD_Active | b_UD_Edit)); return *this; }

	private:
		Behaviour& make_sameLine()		{ _behaviour &= ~b_NewLine; return *this; }
		uint8_t _behaviour = 0;
	};

	enum BehaviourAliases : uint16_t {
		H = 0, V = Behaviour::b_Visible
		, L0 = 0, L = Behaviour::b_NewLine
		, S0 = 0, S = Behaviour::b_Selectable
		, VnLR = 0, V1 = Behaviour::b_ViewOne
		, R = 0, R0 = Behaviour::b_NonRecycle
		, UD_C = 0, UD_A = Behaviour::b_UD_Active, UD_E = Behaviour::b_UD_Edit, UD_S = Behaviour::b_UD_SaveEdit
		, IR = 0, IR0 = Behaviour::b_IteratedNoRecycle
		, ER = 0, ER0 = Behaviour::b_EditNoRecycle
		, EA = Behaviour::b_Edit_Active
	};

	//using BehaviourFlags = Behaviour::BehaviourFlags;

	inline Behaviour::BehaviourFlags operator + (BehaviourAliases lhs, BehaviourAliases rhs) {
		return Behaviour::BehaviourFlags(uint16_t(lhs) | uint16_t(rhs));
	}

	inline Behaviour::BehaviourFlags operator + (Behaviour::BehaviourFlags lhs, BehaviourAliases rhs) {
		return Behaviour::BehaviourFlags(uint16_t(lhs) | uint16_t(rhs));
	}

	inline Behaviour::BehaviourFlags operator + (Behaviour lhs, BehaviourAliases rhs) {
		return Behaviour::BehaviourFlags(uint16_t(lhs) | uint16_t(rhs));
	}

	inline Behaviour filter_viewable() { return Behaviour::b_Visible; } // Set flag required for filtering
	inline Behaviour filter_selectable() { return V + S; } // Set flags required for filtering
}