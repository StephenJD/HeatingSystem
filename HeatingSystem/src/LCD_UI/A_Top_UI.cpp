#include "A_Top_UI.h"
#include <MemoryFree.h>

#ifdef ZPSIM
	#include <iostream>
	#include <iomanip>
	#include <map>
	extern std::map<long, std::string> ui_Objects;
#endif

namespace LCD_UI {
	using namespace std;

	void Chapter_Generator::backKey() {
		auto & currentChapter = operator()();
		if (currentChapter.backUI() == &currentChapter)
			setChapterNo(0);
		else
			currentChapter.rec_prevUI();
	}

	A_Top_UI::A_Top_UI(const I_SafeCollection & safeCollection, int default_active_index )
		: Collection_Hndl(safeCollection, default_active_index)
		, _leftRightBackUI(this)
		, _upDownUI(this)
		, _cursorUI(this)
	{	
		setBackUI(this);
		set_CursorUI_from(this);
		notifyAllOfFocusChange(this);
#ifdef ZPSIM
		cout << "A_Top_UI is a Collection_Hndl at Addr:" << hex << long long(this) << " pointing at : " << (long long)get() << endl;
#endif
	}

	/// <summary>
	/// Active Page element.
	/// </summary>
	Collection_Hndl * A_Top_UI::rec_activeUI() {
		auto receivingUI = selectedPage_h();
		if (receivingUI) {
			if (receivingUI->get()->isCollection())	
				return receivingUI->activeUI();
			else 
				return receivingUI;
		}
		return this;
	}

	void A_Top_UI::set_CursorUI_from(Collection_Hndl * topUI) { // required to get cursor into nested collection.
		if (selectedPage_h() == this) return;
		_cursorUI = topUI;
		while (_cursorUI->get()->isCollection()) {
			auto parent = _cursorUI;
			auto newActive = _cursorUI->activeUI(); // the focus lies in a nested collection
			if (newActive == 0) break;
			_cursorUI = newActive;
			if (_cursorUI->backUI() == 0) _cursorUI->setBackUI(parent);
		}
	}

	void A_Top_UI::enter_nested_ViewAll(Collection_Hndl * topUI, int direction) {
		if (direction < 0 /*&& topUI->focusIndex() <= topUI->count()*/) {
			topUI->set_focus(topUI->endIndex());
			topUI->move_focus_by(-1);
		}
		else if (direction > 0 && topUI->atEnd(topUI->focusIndex())) { // if we previously left from the right and have cycled round to re-enter from the left, start at 0.
			topUI->set_focus(0);
			topUI->move_focus_by(0);
		}
	}

	Collection_Hndl * A_Top_UI::set_leftRightUI_from(Collection_Hndl * topUI, int direction) {
		if (!topUI->behaviour().is_viewAll()) topUI = topUI->backUI();
		_leftRightBackUI = topUI;
		auto this_UI_h = topUI->activeUI();
		do {
			if (this_UI_h->behaviour().is_viewAll()) {
				_leftRightBackUI = this_UI_h;
				enter_nested_ViewAll(_leftRightBackUI, direction);
			}
			this_UI_h = this_UI_h->activeUI();
			if (this_UI_h == 0) return topUI;
		} while (this_UI_h->get()->isCollection());
		if (this_UI_h->behaviour().is_viewAll()) 
			_leftRightBackUI = this_UI_h;
		return topUI;
	}

	void A_Top_UI::set_UpDownUI_from(Collection_Hndl * this_UI_h) {
		// The innermost handle with UpDown behaviour gets the call.
		if (this_UI_h->behaviour().is_UpDnAble()) _upDownUI = this_UI_h;
		while (this_UI_h && this_UI_h->get()->isCollection()) {
			auto active = this_UI_h->activeUI();
			if (active->behaviour().is_UpDnAble()) {
				_upDownUI = active;
			}
			this_UI_h = active;
		}
	}

	void A_Top_UI::selectPage() {
		selectedPage_h()->setBackUI(this);
		set_CursorUI_from(selectedPage_h());
		set_leftRightUI_from(selectedPage_h(),0);
		set_UpDownUI_from(selectedPage_h());
		_cursorUI->setCursorPos();
//#ifdef ZPSIM 
//		logger() << "\tselectedPage: " << ui_Objects[(long long)(selectedPage_h()->get())].c_str() << L_endl;
//		logger() << "\t_upDownUI: " << ui_Objects[(long long)(_upDownUI->get())].c_str() << L_endl;
//		logger() << "\t_leftRightBackUI: " << ui_Objects[(long long)(_leftRightBackUI->get())].c_str() << L_endl;
//		logger() << "\trec_activeUI(): " << ui_Objects[(long long)(rec_activeUI()->get())].c_str() << L_endl;
//#endif
	}

	void A_Top_UI::rec_left_right(int move) { // left-right movement
		using HI_BD = HardwareInterfaces::LCD_Display;

		if (selectedPage_h() == this) {
			setBackUI(activeUI());
			selectPage();
			return;
		}

		auto hasMoved = _leftRightBackUI->move_focus_by(move);
		bool wasInEdit = false;
		
		if (!hasMoved) {
			wasInEdit = { _leftRightBackUI->cursorMode(_leftRightBackUI) == HI_BD::e_inEdit };
			if (wasInEdit) {
				if (_leftRightBackUI->backUI()->behaviour().is_edit_on_next()) {
					rec_edit();
					hasMoved = _leftRightBackUI->move_focus_by(move);
				}
				else {
					_leftRightBackUI->move_focus_by(0);
					wasInEdit = false;
					hasMoved = true;
				}
			}
		}
		if (!hasMoved) {
			do {
				do {
					_leftRightBackUI = _leftRightBackUI->backUI();
				} while (_leftRightBackUI != this && !_leftRightBackUI->behaviour().is_viewAll());

				hasMoved = _leftRightBackUI->move_focus_by(move);
			} while (!hasMoved && _leftRightBackUI != this);
		}

		set_CursorUI_from(set_leftRightUI_from(_leftRightBackUI, move));

		set_UpDownUI_from(_leftRightBackUI);
		_cursorUI->setCursorPos();
		_leftRightBackUI->move_focus_by(0);
		if (wasInEdit)
			rec_edit();

//#ifdef ZPSIM
//		logger() << "\tselectedPage: " << ui_Objects[(long long)(selectedPage_h()->get())].c_str() << L_endl;
//		logger() << "\t_upDownUI: " << ui_Objects[(long long)(_upDownUI->get())].c_str() << L_endl;
//		logger() << "\t_leftRightBackUI: " << ui_Objects[(long long)(_leftRightBackUI->get())].c_str() << L_endl;
//		logger() << "\trec_activeUI(): " << ui_Objects[(long long)(rec_activeUI()->get())].c_str() << L_endl;
//#endif
	}

	void A_Top_UI::rec_up_down(int move) { // up-down movement
		//logger() << "rec_up_down start for " << long(this) << " Mem: " << freeMemory() << L_endl;
		auto haveMoved = false;
		auto freeMem = freeMemory();

		if (_upDownUI->get()->isCollection() && _upDownUI->behaviour().is_viewOneUpDn()) {
			if (_leftRightBackUI->cursorMode(_leftRightBackUI) == HardwareInterfaces::LCD_Display::e_inEdit) {
				move = -move; // reverse up/down when in edit.
			}
			haveMoved = _upDownUI->move_focus_by(move);
			changeInFreeMemory(freeMem, "\trec_up_down move_focus_by");

			//logger() << "rec_up_down move_focus_by for " << long(this) << " Mem: " << freeMemory() << L_endl;

		}
		if (!haveMoved) {
			if (_upDownUI->get()->upDn_IsSet()) {
				haveMoved = static_cast<Custom_Select*>(_upDownUI->get())->move_focus_by(move);
				changeInFreeMemory(freeMem, "\trec_up_down not-moved move_focus_by");

				//logger() << "rec_up_down !move_focus_by for " << long(this) << " Mem: " << freeMemory() << L_endl;
				set_UpDownUI_from(selectedPage_h());
				set_CursorUI_from(selectedPage_h());
			} else if (_upDownUI->behaviour().is_edit_on_next()) {
				rec_edit();
				changeInFreeMemory(freeMem, "\trec_up_down rec_edit");
				haveMoved = _upDownUI->move_focus_by(-move); // reverse up/down when in edit.
				changeInFreeMemory(freeMem, "\trec_up_down edit move_focus_by");
			} 
		}
		
		if (haveMoved) {
			set_CursorUI_from(_upDownUI);
			set_leftRightUI_from(selectedPage_h(),0);
			changeInFreeMemory(freeMem, "\trec_up_down set_leftRightUI_from");
			notifyAllOfFocusChange(_leftRightBackUI);
			changeInFreeMemory(freeMem, "\trec_up_down notifyAllOfFocusChange");
			//notifyAllOfFocusChange(selectedPage_h());
			_cursorUI->backUI()->activeUI(); // required for setCursor to act on loaded active element
			_cursorUI->setCursorPos();
			changeInFreeMemory(freeMem, "\trec_up_down setCursorPos");
			//logger() << "rec_up_down notifyAllOfFocusChange for " << long(this) << " Mem: " << freeMemory() << L_endl;
		}
	}

	void A_Top_UI::notifyAllOfFocusChange(Collection_Hndl * top) {
		top->get()->focusHasChanged(top == _upDownUI);
		for (int i = top->get()->collection()->nextActionableIndex(0); !top->atEnd(i); i = top->get()->collection()->nextActionableIndex(++i)) { // need to check all elements on the page
			auto element_h = static_cast<Collection_Hndl *>(top->get()->collection()->item(i));
			if (element_h->get()->isCollection()) {
//#ifdef ZPSIM
//				logger() << "Notify: " << ui_Objects[(long long)(element_h->get())].c_str() << L_tabs << L_hex << (long long)(element_h->get()) << L_endl;
//#endif
				element_h->focusHasChanged(element_h == _upDownUI);
				auto inner = element_h->activeUI();
				if (inner && inner->get()->isCollection()) notifyAllOfFocusChange(inner);
			}
		}
	}

	void A_Top_UI::rec_select() { 
		if (selectedPage_h() == this) {
			setBackUI(activeUI());
		} else {
			auto jumpTo = _cursorUI->get()->select(_cursorUI);
			if (jumpTo) {// change page
				setBackUI(jumpTo);
			}
		}
		selectPage();
	}

	void A_Top_UI::rec_edit() {
		if (selectedPage_h() == this) {
			setBackUI(activeUI());
		} else {
			auto jumpTo = _cursorUI->get()->edit(_cursorUI);
			if (jumpTo) {// change page
				setBackUI(jumpTo);
			}
		}
		selectPage();
	}


	void A_Top_UI::rec_prevUI() {
		decltype(backUI()) newFocus = 0;
		if (_cursorUI->get()->back() && _cursorUI->on_back()) {// else returning from cancelled edit
			newFocus = selectedPage_h()->backUI();
		}
		if (newFocus == this) { // deselect page
			selectedPage_h()->setBackUI(0);
			setBackUI(this);
			_cursorUI = this;
			_leftRightBackUI = this;
			_upDownUI = this;
		} else {
			if (newFocus) setBackUI(newFocus);
			selectPage();
		}
	}

	UI_DisplayBuffer & A_Top_UI::stream(UI_DisplayBuffer & buffer) const { // Top-level stream
		buffer.reset();
		//std::cout << "\n... Streaming Started ...\n";
		selectedPage_h()->streamElement(buffer, _cursorUI);
		return buffer;
	}
}