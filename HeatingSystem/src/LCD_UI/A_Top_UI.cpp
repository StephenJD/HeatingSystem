#include "A_Top_UI.h"
#include <MemoryFree.h>

#ifdef ZPSIM
	#include <iostream>
	#include <iomanip>
#endif

namespace arduino_logger {
	Logger& debug();
}
using namespace arduino_logger;

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

	Collection_Hndl* A_Top_UI::innerMost(Collection_Hndl * tryInner, bool (*predicate)(Collection_Hndl * tryColl)) {
		/*
		* Must start with a collection.
		* Finds the innermost active element with the required behaviour.
		* It may be inside elements without that behaviour.
		* It may be either a collection or an object.
		*/
		//Collection_Hndl* tryInner = collHdlPtr;
		Collection_Hndl* hasBehaviour = tryInner;
		do {
			tryInner = tryInner->activeUI();
			if (!tryInner) break;
			//debug() << F("\tinnerMost: ") << ui_Objects()[(long)tryInner->get()].c_str() << L_endl;
			if (predicate(tryInner)) hasBehaviour = tryInner;
		} while (tryInner->get()->isCollection());
		
		return hasBehaviour;
	}

	Collection_Hndl* A_Top_UI::uiField(Collection_Hndl* uiElement) {
		/*
		* Must start with a collection.
		* Finds the active UI-element.
		*/
		while (uiElement->get()->cursorMode(0) == HardwareInterfaces::LCD_Display::e_unselectable && uiElement->get()->isCollection()) {
			Collection_Hndl* tryInner = uiElement->activeUI();
			if (!tryInner) break;
			uiElement = tryInner;
			//debug() << F("\tinnerUI: ") << ui_Objects()[(long)uiElement->get()].c_str() << L_endl;
		}
		
		return uiElement;

	}

	void A_Top_UI::setEachPageElementToItsFocus(I_SafeCollection& colln) {
		// Visit each most deeply nested ActiveUI field on the page and set its focusPos.
		//#ifdef ZPSIM
		//		debug() << F("\tSetElToFocus?: ") << ui_Objects()[(long)&colln].c_str() << L_endl;
		//#endif

		for (int i = 0; !colln.atEnd(i); ++i) {
			auto elHdl = static_cast<Collection_Hndl*>(&colln[i]);
			//debug() << F("\tpageItem: ") << ui_Objects()[(long)elHdl->get()].c_str() << L_endl;
			if (elHdl->get()->isCollection()) {
				auto inner = uiField(elHdl);
#ifdef ZPSIM
				debug() << F("\tSetElToFocus: ") << inner->focusIndex() << " for " << ui_Objects()[(long)inner->get()].c_str() << L_endl;
#endif
				inner->move_focus_to(inner->focusIndex());
			}
		}
	}

	void A_Top_UI::notifyAllOfFocusChange(Collection_Hndl* top) {
		/* Visit each UI field on the page and call focusHasChanged() if it's not _upDownUI.
		*  Visit active collection of each V1 and all collections of each VnLR whilst the collection is not a UI. 
		*/
		if (top->get()->cursorMode(top) != HardwareInterfaces::LCD_Display::e_unselectable) {
			if (top != _upDownUI) {
#ifdef ZPSIM
				debug() << F("Notify: ") << ui_Objects()[(long)top->get()].c_str() << L_endl;
#endif
				top->focusHasChanged();
			}
		}
		else if (top->behaviour().is_viewOne()) {
			Collection_Hndl * inner = top->activeUI();
			if (inner && inner->get()->isCollection()) notifyAllOfFocusChange(inner);
		} else {
			for (int i = 0; !top->atEnd(i); ++i) {
				Collection_Hndl* inner = static_cast<Collection_Hndl*>(top->get()->collection()->item(i));
				if (inner->get()->isCollection()) {
					notifyAllOfFocusChange(inner);
				}
			}
		}
	}

	void A_Top_UI::set_CursorUI_from(Collection_Hndl * tryInner) { // required to get cursor into nested collection.
		// Find the most deeply nested ActiveUI;
		if (selectedPage_h() == this) return;

		 _cursorUI = tryInner;
		while (_cursorUI->get()->isCollection()) {
			tryInner = _cursorUI->activeUI();
			if (!tryInner) break;
			//debug() << F("\tinnerMost: ") << ui_Objects()[(long)tryInner->get()].c_str() << L_endl;
			if (tryInner->backUI() == 0) tryInner->setBackUI(_cursorUI);
			_cursorUI = tryInner;
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
#ifdef ZPSIM
			//auto thisFocus = tryLeftRight->focusIndex();
			//tryLeftRight->move_focus_to(thisFocus);
			//auto thisActive_h = tryLeftRight->activeUI();
			//debug() << F("\ttryLeftRightUI: ") << ui_Objects()[(long)(tryLeftRight->get())].c_str()
			//	<< (try_behaviour.is_viewAll_LR() ? " ViewAll_LR" : " ViewActive")
			//	<< " TryActive: " << (thisActive_h ? ui_Objects()[(long)(thisActive_h->get())].c_str() : "Not a collection" ) << L_endl;
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
					}
				}
			}
			auto tryActive_h = tryLeftRight->activeUI();
			if (tryActive_h != 0) {
				tryLeftRight = tryActive_h;
			} 
			else break;
		}
		if (_leftRightBackUI != gotLeftRight) {
			_leftRightBackUI = gotLeftRight;
			_leftRightBackUI->enter_collection(direction);
		}
#ifdef ZPSIM
		//debug() << F("\tLeftRightUI is: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
#endif
		return topUI;
	}

	void A_Top_UI::set_UpDownUI_from(Collection_Hndl * this_UI_h) {
		// The innermost handle with UpDown behaviour gets the call.
		auto isUpDnAble = [](Collection_Hndl * tryColl) -> bool {
			if (tryColl->get()->isCollection()) {
				if (tryColl->behaviour().is_viewOne() || tryColl->get()->collection()->iterableObjectIndex() >= 0) {
					return true;
				}
			} else if (tryColl->behaviour().is_cmd_on_UD()) return true;
			return false;
		};

		_upDownUI = innerMost(this_UI_h, isUpDnAble);

#ifdef ZPSIM
		//debug() << F("\tUD is: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
#endif
	}

	void A_Top_UI::selectPage() {
		selectedPage_h()->setBackUI(this);
		set_CursorUI_from(selectedPage_h());
		set_leftRightUI_from(selectedPage_h(),0);
		set_UpDownUI_from(selectedPage_h());
#ifdef ZPSIM 
		debug() << F("\nselectedPage: ") << ui_Objects()[(long)(selectedPage_h()->get())].c_str() << L_endl;
		debug() << F("\tselectedPage focus: ") << selectedPage_h()->focusIndex() << L_endl;
		debug() << F("\t_upDownUI: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
		debug() << F("\t_upDownUI focus: ") << _upDownUI->focusIndex() << L_endl;
		debug() << F("\t_leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
		debug() << F("\t_cursorUI(): ") << ui_Objects()[(long)(_cursorUI->get())].c_str() << L_endl;
		if (_cursorUI->get()) debug() << F("\t_cursorUI() focus: ") << _cursorUI->focusIndex() << L_endl;
#endif
		_cursorUI->setCursorPos();
	}

	bool A_Top_UI::rec_left_right(int move) { // left-right movement
		using HI_BD = HardwareInterfaces::LCD_Display;
#ifdef ZPSIM
		debug() << F("LR on _leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
#endif
		setEachPageElementToItsFocus(*activeUI()->get()->collection());

		// Lambda
		auto isIteratedUD0 = [this]() -> bool {
			if (_leftRightBackUI->get()->isCollection()) {
				auto & itColl = *_leftRightBackUI->get()->collection();
				return itColl.iterableObjectIndex() >= 0 && !itColl.behaviour().is_next_on_UpDn();
			} else return false;
		};

		auto iteratedBehaviour = [this]() {
			auto& itColl = *_upDownUI->backUI()->get()->collection();
			if (itColl.iterableObjectIndex() >= 0) return itColl.behaviour();
			return _upDownUI->behaviour();
		};

		// Algorithm
		if (selectedPage_h() == this) {
			setBackUI(activeUI());
			selectPage();
			return true;
		}

		auto lr_behaviour = iteratedBehaviour();
		auto hasMoved = _leftRightBackUI->leftRight(move, lr_behaviour);
		bool wasInEdit = false;
		
		if (!hasMoved) {
			wasInEdit = { _leftRightBackUI->cursorMode(_leftRightBackUI) == HI_BD::e_inEdit };
			if (wasInEdit) {
				if (_leftRightBackUI->backUI()->behaviour().is_edit_on_UD()) {
					rec_edit();
					hasMoved = rec_left_right(move);
				}
				else {
					_leftRightBackUI->leftRight(0, lr_behaviour);
					wasInEdit = false;
					hasMoved = true;
				}
			} else if (isIteratedUD0()) { // No more elements in the collection, so move to next iteration and start again. 
				auto& itColl = *_leftRightBackUI->get()->collection();
				auto& iteratedObject = static_cast<Collection_Hndl&>(itColl[itColl.iterableObjectIndex()]);
				hasMoved = iteratedObject.get()->leftRight(move, &iteratedObject,lr_behaviour);
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
	debug() << F("\t_leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
#endif
				} while (_leftRightBackUI != this && (_leftRightBackUI->behaviour().is_viewOne()));

				hasMoved = _leftRightBackUI->leftRight(move, lr_behaviour);
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
		debug() << F("selectedPage: ") << ui_Objects()[(long)(selectedPage_h()->get())].c_str() << L_endl;
		debug() << F("\t_leftRightBackUI: ") << ui_Objects()[(long)(_leftRightBackUI->get())].c_str() << L_endl;
		debug() << F("\trec_activeUI(): ") << ui_Objects()[(long)(rec_activeUI()->get())].c_str() << L_endl;
		debug() << F("\t_upDownUI: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
		debug() << F("\t_cursorUI: ") << ui_Objects()[(long)(_cursorUI->get())].c_str() << L_endl;
#endif
		return hasMoved;
	}

	void A_Top_UI::rec_up_down(int move) { // up-down movement
		auto haveMoved = false;
#ifdef ZPSIM
		debug() << F("UD on _upDownUI: ") << ui_Objects()[(long)(_upDownUI->get())].c_str() << L_endl;
#endif
		setEachPageElementToItsFocus(*activeUI()->get()->collection());

		// Lambda
		auto iteratedBehaviour = [this]() {
			auto& itColl = *_upDownUI->backUI()->get()->collection();
			if (itColl.iterableObjectIndex() >= 0) return itColl.behaviour();
			return _upDownUI->behaviour();
		};

		// Algorithm
		auto ud_behaviour = iteratedBehaviour();

		if (ud_behaviour.is_viewOneUpDn_Next()) {
			if (_upDownUI->backUI()->cursorMode(_upDownUI->backUI()) == HardwareInterfaces::LCD_Display::e_inEdit) {
				move = -move; // reverse up/down when in edit.
			}
		}
		if (!ud_behaviour.is_edit_on_UD()) haveMoved = _upDownUI->upDown(move, ud_behaviour);

		if (!haveMoved) {
			if (ud_behaviour.is_edit_on_UD()) {
				auto isSave = ud_behaviour.is_save_on_UD();
				rec_edit();
				haveMoved = _upDownUI->upDown(-move, ud_behaviour); // reverse up/down when in edit.
				if (isSave) {
					rec_select();
				}
			} else if (!ud_behaviour.is_IteratedRecycle()) {
				_upDownUI->upDown(0, ud_behaviour);
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

	void A_Top_UI::rec_select() { 
		setEachPageElementToItsFocus(*activeUI()->get()->collection());
		if (selectedPage_h() == this) {
			setBackUI(activeUI());
		} else {
			auto jumpTo = _cursorUI->get()->select(_cursorUI);
			if (jumpTo) {// change page
				setBackUI(jumpTo);
			}
		}
		setEachPageElementToItsFocus(*activeUI()->get()->collection());
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