#pragma once

#include "UI_LazyCollection.h"

namespace LCD_UI {
	class A_Top_UI;

	class Chapter_Generator {
	public:
		virtual A_Top_UI & operator()(int chapterNo) = 0;
		A_Top_UI & operator()() { return operator()(_chapterNo); }
		void setChapterNo(int chapterNo) { _chapterNo = chapterNo; }
		void backKey();
		uint8_t chapter() { return _chapterNo; }
		uint8_t page();
	private:
		uint8_t _chapterNo = 0;
	};

	// This UI uses _backUI to hold the UI that receives interactions from the user.
	// _backUI is therefore the selected / edited UI.

	class A_Top_UI : public Collection_Hndl {
	public:
		A_Top_UI(const I_SafeCollection & safeCollection, int default_active_index = 0);
		void	rec_left_right(int move); // Moves focus to next selectable item in the receptive object, such as next field
		void	rec_up_down(int move);
		void	rec_prevUI();
		void	rec_select();
		void	rec_edit();
		// These functions are called on the _backUI which is possibly a deeply nested _uiElement
		// Queries

		/// <summary>
		/// Top-most object-handle in which the selection lives.
		/// When not in edit: a page element handle.
		/// </summary>
		const Collection_Hndl * rec_activeUI() const { return const_cast<A_Top_UI*>(this)->rec_activeUI(); }
		UI_DisplayBuffer & stream(UI_DisplayBuffer & buffer) const; // Top-level stream
		
		const Collection_Hndl * selectedPage_h() const { return backUI(); }

		// Modifiers
		Collection_Hndl * selectedPage_h() { return backUI(); }

		/// <summary>
		/// Top-most object-handle in which the selection lives.
		/// When not in edit: a page element handle.
		/// </summary>
		Collection_Hndl * rec_activeUI();
	private:	
		// Modifiers
		void selectPage();
		void set_CursorUI_from(Collection_Hndl * topUI);
		Collection_Hndl * set_leftRightUI_from(Collection_Hndl * topUI, int direction);
		void enter_nested_ViewAll(Collection_Hndl * topUI, int direction);
		void set_UpDownUI_from(Collection_Hndl * topUI);
		void notifyAllOfFocusChange(Collection_Hndl * topUI);

		Collection_Hndl * _leftRightBackUI;
		Collection_Hndl * _upDownUI;
		Collection_Hndl * _cursorUI; // Inner-most Collection_Hndl pointing to the object with the cursor.
	};

	/// <summary>
	/// A Variadic Helper function to make Static Collections of Collection_Hndl.
	/// The elements have focus() and activeUI()
	/// It takes a list of Collectible objects by reference
	/// </summary>
	template<typename... Args>
	auto makeDisplay(Args & ... args) -> Collection<sizeof...(args), Collection_Hndl> {
		return Collection<sizeof...(args), Collection_Hndl>(ArrayWrapper<sizeof...(args), Collection_Hndl>{Collection_Hndl{ args }...}, viewOneUpDnRecycle());
	}
}

