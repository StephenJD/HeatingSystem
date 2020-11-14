#include "A_Top_UI.h"
#include <MemoryFree.h>

#ifdef ZPSIM
	#include <iostream>
	#include <iomanip>
#endif

namespace LCD_UI {

	void Chapter_Generator::backKey() {
		auto & currentChapter = operator()();
		if (currentChapter.backUI() == &currentChapter)
			setChapterNo(0);
		else
			currentChapter.rec_prevUI();
	}

	uint8_t Chapter_Generator::page() { return operator()(_chapterNo).focusIndex(); }

	A_Top_UI::A_Top_UI(const I_SafeCollection & safeCollection, int default_active_index )
		: Collection_Hndl(safeCollection, default_active_index)
		, _leftRightBackUI(this)
		, _upDownUI(this)
		, _cursorUI(this)
	{	
		setBackUI(this);
		set_CursorUI_from(this);
		notifyAllOfFocusChange(this);
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
#ifdef ZPSIM
			logger() << F("\t_cursorUI: ") << ui_Objects()[(long)(_cursorUI->get())].c_str() << " Active: " << (newActive? ui_Objects()[(long)(newActive->get())].c_str():"") << L_endl;
#endif
			if (newActive == 0) break;
			_cursorUI = newActive;
			if (_cursorUI->backUI() == 0) _cursorUI->setBackUI(parent);
		}
	}

	Collection_Hndl * A_Top_UI::set_leftRightUI_from(Collection_Hndl * topUI, int direction) {
		// LR passes to the innermost ViewAll-collection	
		auto tryLeftRight = topUI;
		auto gotLeftRight = _leftRightBackUI;
		bool tryIsCollection;
		while ((tryIsCollection = tryLeftRight->get()->isCollection()) || tryLeftRight->behaviour().is_viewAll_LR()) {
			if (!tryIsCollection) {
				gotLeftRight = tryLeftRight;
				break;
			}
			auto try_behaviour = tryLeftRight->behaviour();
			auto tryActive_h = tryLeftRight->activeUI();
#ifdef ZPSIM
			logger() << F("\ttryLeftRightUI: ") << ui_Objects()[(long)(tryLeftRight->get())].c_str()
				<< (try_behaviour.is_viewAll_LR() ? " ViewAll_LR" : " ViewActive")
				<< " TryActive: " << (tryActive_h ? ui_Objects()[(long)(tryActive_h->get())].c_str() : "Not a collection" ) << L_endl;
#endif
				if (try_behaviour.is_viewAll_LR()) {
				gotLeftRight = tryLeftRight;
				if (_leftRightBackUI != gotLeftRight) {
					auto & colln = *gotLeftRight->get()->collection();
					if (!colln.indexIsInRange(colln.focusIndex())) {
						gotLeftRight->enter_collection(direction);
					}
					if (colln.iterableObjectIndex() != -1) {
						auto & itActiveColln_h = *colln.activeUI();
						auto & itActiveColln = *itActiveColln_h.get()->collection();
						itActiveColln_h.enter_collection(direction);
						gotLeftRight = &itActiveColln_h;
					}
				}
			}
			tryLeftRight = tryLeftRight->activeUI();
		}
		if (_leftRightBackUI != gotLeftRight) {
			_leftRightBackUI = gotLeftRight;
			_leftRightBackUI->enter_collection(direction);
		}
#ifdef ZPSIM
		logger() << F("\tLeftRightUI is: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
#endif
		return topUI;
	}

	void A_Top_UI::set_UpDownUI_from(Collection_Hndl * this_UI_h) {
		// The innermost handle with UpDown behaviour gets the call.
		_upDownUI = this_UI_h;
		while (this_UI_h && this_UI_h->get()->isCollection()) {
			auto tryUD = this_UI_h->activeUI();
			auto isUpDnAble = false;
			if (tryUD->get()->isCollection()) {
				if (tryUD->behaviour().is_viewOne() || tryUD->get()->collection()->iterableObjectIndex() >= 0) {
					isUpDnAble = true;
				}
			}
			else if (tryUD->behaviour().is_cmd_on_UD()) isUpDnAble = true;
#ifdef ZPSIM
			logger() << F("\ttryUD: ") << ui_Objects()[(long)(tryUD->get())].c_str() << (isUpDnAble ? " HasUD" : " NoUD") << L_endl;
#endif
			if (isUpDnAble) {
				_upDownUI = tryUD;
			}
			this_UI_h = tryUD;
		}
#ifdef ZPSIM
		logger() << F("\tUD is: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
#endif
	}

	void A_Top_UI::selectPage() {
		selectedPage_h()->setBackUI(this);
		set_CursorUI_from(selectedPage_h());
		set_leftRightUI_from(selectedPage_h(),0);
		set_UpDownUI_from(selectedPage_h());
#ifdef ZPSIM 
		logger() << F("\nselectedPage: ") << ui_Objects()[(long)(selectedPage_h()->get())].c_str() << L_endl;
		logger() << F("\tselectedPage focus: ") << selectedPage_h()->focusIndex() << L_endl;
		logger() << F("\t_upDownUI: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
		logger() << F("\t_upDownUI focus: ") << _upDownUI->focusIndex() << L_endl;
		logger() << F("\t_leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
		logger() << F("\t_cursorUI(): ") << ui_Objects()[(long)(_cursorUI->get())].c_str() << L_endl;
		if (_cursorUI->get()) logger() << F("\t_cursorUI() focus: ") << _cursorUI->focusIndex() << L_endl;
#endif
		_cursorUI->setCursorPos();
	}

	bool A_Top_UI::rec_left_right(int move) { // left-right movement
		using HI_BD = HardwareInterfaces::LCD_Display;
		// Lambda
		auto isIteratedUD0 = [this]() -> bool {
			if (_leftRightBackUI->get()->isCollection()) {
				auto & itColl = *_leftRightBackUI->get()->collection();
				//return itColl.iterableObjectIndex() >= 0 && !itColl[itColl.iterableObjectIndex()]->behaviour().is_next_on_UpDn();
				return itColl.iterableObjectIndex() >= 0 && !behaviour().is_next_on_UpDn();
			} else return false;
		};

		// Algorithm
		if (selectedPage_h() == this) {
			setBackUI(activeUI());
			selectPage();
			return true;
		}

		auto hasMoved = _leftRightBackUI->move_focus_by(move);
		bool wasInEdit = false;
		
		if (!hasMoved) {
			wasInEdit = { _leftRightBackUI->cursorMode(_leftRightBackUI) == HI_BD::e_inEdit };
			if (wasInEdit) {
				if (_leftRightBackUI->backUI()->behaviour().is_edit_on_UD()) {
					rec_edit();
					hasMoved = rec_left_right(move);
				}
				else {
					_leftRightBackUI->move_focus_by(0);
					wasInEdit = false;
					hasMoved = true;
				}
			} else if (isIteratedUD0()) { // No more elements in the collection, so move to next iteration and start again. 
				auto& itColl = *_leftRightBackUI->get()->collection();
				auto& iteratedObject = static_cast<Collection_Hndl&>(itColl[itColl.iterableObjectIndex()]);
				hasMoved = iteratedObject.get()->move_focus_by(move, &iteratedObject);
				if (hasMoved) {
					if (move > 0) itColl.setFocusIndex(itColl.nextActionableIndex(0)); else itColl.setFocusIndex(itColl.prevActionableIndex(itColl.endIndex()));
				}
			}
		}
		if (!hasMoved) {
			do {
				do {
					_leftRightBackUI = _leftRightBackUI->backUI();
#ifdef ZPSIM
	logger() << F("\t_leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
#endif
				} while (_leftRightBackUI != this && (_leftRightBackUI->behaviour().is_viewOne()));

				hasMoved = _leftRightBackUI->move_focus_by(move);
			} while (!hasMoved && _leftRightBackUI != this);
		}

		auto outerColl = _leftRightBackUI->backUI();
		set_CursorUI_from(set_leftRightUI_from(outerColl, move));
		set_UpDownUI_from(outerColl);

		_cursorUI->setCursorPos();
		_upDownUI->move_focus_by(0);
		if (wasInEdit)
			rec_edit();

#ifdef ZPSIM
		logger() << F("selectedPage: ") << ui_Objects()[(long)(selectedPage_h()->get())].c_str() << L_endl;
		logger() << F("\t_leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
		logger() << F("\trec_activeUI(): ") << ui_Objects()[(long)(rec_activeUI()->get())].c_str() << L_endl;
		logger() << F("\t_upDownUI: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
		logger() << F("\t_cursorUI: ") << ui_Objects()[(long)(_cursorUI->get())].c_str() << L_endl;
#endif
		return hasMoved;
	}

	void A_Top_UI::rec_up_down(int move) { // up-down movement
		auto haveMoved = false;

		if (_upDownUI->behaviour().is_viewOneUpDn_Next()) {
			if (_upDownUI->backUI()->cursorMode(_upDownUI->backUI()) == HardwareInterfaces::LCD_Display::e_inEdit) {
				move = -move; // reverse up/down when in edit.
			}
		}
		if (!_upDownUI->behaviour().is_edit_on_UD()) haveMoved = _upDownUI->move_focus_by(move);

		if (!haveMoved) {
			if (_upDownUI->get()->upDn_IsSet()) {
				haveMoved = static_cast<Custom_Select*>(_upDownUI->get())->move_focus_by(move,this);
				set_UpDownUI_from(selectedPage_h());
				set_CursorUI_from(selectedPage_h());
			} else if (_upDownUI->behaviour().is_edit_on_UD()) {
				auto isSave = _upDownUI->behaviour().is_save_on_UD();
				rec_edit();
				haveMoved = _upDownUI->move_focus_by(-move); // reverse up/down when in edit.
				if (isSave) {
					rec_select();
				}
			} else {
				_upDownUI->move_focus_by(0);
			}
		}
		
		if (haveMoved) {
			set_CursorUI_from(_upDownUI);
			set_leftRightUI_from(selectedPage_h(),0);
			notifyAllOfFocusChange(_leftRightBackUI);
			_cursorUI->backUI()->activeUI(); // required for setCursor to act on loaded active element
			_cursorUI->setCursorPos();
		}
	}

	void A_Top_UI::notifyAllOfFocusChange(Collection_Hndl * top) {
		top->get()->focusHasChanged(top == _upDownUI);
		auto topCollection = top->get()->collection();
		if (topCollection == 0) return;
		for (int i = topCollection->nextActionableIndex(0); !top->atEnd(i); /*i = topCollection->nextActionableIndex(++i)*/) { // need to check all elements on the page
			auto element_h = static_cast<Collection_Hndl *>(topCollection->item(i));
			if (element_h && element_h->get()->isCollection()) {
				if (element_h != _upDownUI) {
#ifdef ZPSIM
					logger() << F("Notify: ") << ui_Objects()[(long)(element_h->get())].c_str() << L_endl;
#endif
					element_h->focusHasChanged();
				}

				auto inner = element_h->activeUI();
				if (inner && inner->get()->isCollection()) notifyAllOfFocusChange(inner);
			}
			auto next_i = topCollection->nextActionableIndex(++i);
			if (next_i >= i)
				i = next_i; 
			else 
				break;
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
		} else { // take out of edit
			if (newFocus) setBackUI(newFocus);
			_leftRightBackUI = backUI()/*->backUI()*/;
			selectPage();
		}
	}

	UI_DisplayBuffer & A_Top_UI::stream(UI_DisplayBuffer & buffer) const { // Top-level stream
		buffer.reset();
		selectedPage_h()->get()->streamElement(buffer, _cursorUI);
		return buffer;
	}
}