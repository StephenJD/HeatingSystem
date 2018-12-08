#pragma once

namespace LCD_UI {
	extern const char NEW_LINE_CHR;

	class Behaviour {
	public:
		enum BehaviourFlags : unsigned char { b_Hidden, b_NewLine, b_Viewable = 2, b_Selectable = 4, b_ViewAll = 8, b_NextItemOnUpDown = 16, b_EditOnUpDn = 32, b_RecycleInList = 64, b_NotEditable = 128 };
		// 110 :view, sel, ViewAll, EditOnNextItem, Recycle
		// 94 : view, sel, ViewAll, UpDn, Recycle
		// 86 : view, sel, ViewOne, UpDn, Recycle
		// 78 : view, sel, ViewOne, Recycle
		// 38 : view, sel, ViewOne, UpDn
		// 30 : view, sel, ViewAll, UpDn
		// 22 : view, sel, ViewOne, UpDn
		// 21 : newLine, sel, UpDown
		Behaviour() = default;
		explicit Behaviour(int b) : _behaviour(b) {}

		// Queries
		explicit operator unsigned char() const { return _behaviour; }
		bool is(Behaviour b) const { return (_behaviour & unsigned char(b)) == unsigned char(b); }

		bool is_viewable() const		{ return is(Behaviour{ b_Viewable }); }
		bool is_UpDnAble() const		{ return (_behaviour & b_Viewable)!=0 && ((_behaviour & b_NextItemOnUpDown) != 0 || (_behaviour & b_EditOnUpDn) != 0); }
		bool is_viewOne() const		    { return (_behaviour & b_Viewable)!=0 && (_behaviour & b_ViewAll) == 0; }
		bool is_viewOneUpDn() const		{ return (_behaviour & b_Viewable)!=0 && (_behaviour & b_ViewAll) == 0 && (_behaviour & b_NextItemOnUpDown)!=0; }
		bool is_viewAll() const			{ return is(Behaviour{ b_Viewable | b_ViewAll }); }
		bool is_selectable() const /*has cursor*/ { return is(Behaviour{ b_Viewable | b_Selectable }); }
		bool is_edit_on_next() const	{ return is(Behaviour{ b_Viewable | b_Selectable | b_EditOnUpDn }); }
		bool is_recycle_on_next() const	{ return is(Behaviour{ b_RecycleInList }); }
		bool is_OnNewLine() const		{ return is(Behaviour{ b_NewLine }); }
		bool is_Editable() const		{ return (_behaviour & b_NotEditable) == 0; }
		// Modifiers
		Behaviour & operator = (int b) { _behaviour = b; return *this; }
		Behaviour make_hidden() { _behaviour &= ~b_Viewable; return *this; }
		Behaviour make_visible() { _behaviour |= b_Viewable; return *this; }
		Behaviour make_visible(bool show) {return show ? make_visible() : make_hidden(); }
		Behaviour make_newLine() { _behaviour |= b_NewLine; return *this; }
		Behaviour make_sameLine() { _behaviour &= ~b_NewLine; return *this; }
		Behaviour make_viewAll() { _behaviour |= b_ViewAll; return *this; }
		Behaviour make_viewOne() { _behaviour &= ~b_ViewAll; return *this; }
		Behaviour make_editOnNext() { _behaviour |= b_EditOnUpDn; return *this; }
		Behaviour make_unEditable() { _behaviour |= b_NotEditable; return *this; }
		Behaviour addBehaviour(BehaviourFlags b) { _behaviour |= b; return *this;  }
		Behaviour removeBehaviour(BehaviourFlags b) { _behaviour &= ~b; return *this;  }
	private:
		unsigned char _behaviour = b_Viewable;
	};
	// Select Behaviours
	inline Behaviour hidden() { return Behaviour{ Behaviour::b_Hidden }; }
	inline Behaviour newLine() { return Behaviour{ Behaviour::b_NewLine | Behaviour::b_Viewable }; }
	inline Behaviour viewable() /*no cursor*/ { return Behaviour{ Behaviour::b_Viewable }; }
	inline Behaviour viewOneSelectable() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable }; }
	inline Behaviour selectable() { return viewOneSelectable(); }
	inline Behaviour viewAll() /*has cursor*/ { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll  }; }
	inline Behaviour viewOneUpDn() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_NextItemOnUpDown }; }
	inline Behaviour viewOneUpDnRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_NextItemOnUpDown | Behaviour::b_RecycleInList }; }
	inline Behaviour viewAllRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll | Behaviour::b_RecycleInList }; }
	inline Behaviour viewAllUpDn() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll  | Behaviour::b_NextItemOnUpDown}; }
	inline Behaviour viewAllUpDnRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll | Behaviour::b_NextItemOnUpDown | Behaviour::b_RecycleInList }; }
	inline Behaviour editOnNextItem() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_EditOnUpDn | Behaviour::b_RecycleInList }; }
	// Edit Behaviours
	inline Behaviour editNonRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll }; }
	inline Behaviour editRecycle() { return Behaviour{ Behaviour::b_Viewable | Behaviour::b_Selectable | Behaviour::b_ViewAll | Behaviour::b_RecycleInList }; }

	inline Behaviour operator + (Behaviour lhs, Behaviour rhs) {
		return Behaviour{ unsigned char(lhs) + unsigned char(rhs) };
	}
}