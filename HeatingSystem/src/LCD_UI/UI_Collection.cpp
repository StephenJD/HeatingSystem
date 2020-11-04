#include "UI_Collection.h"
#include "UI_Cmd.h"
#include "Logging.h"

#ifdef ZPSIM
	#include <ostream>
	#include <iomanip>
	#include <map>
	#include <string>
	using namespace std;
	map<long, string> & ui_Objects() {
		static map<long, string> _ui_Objects;
		return _ui_Objects;
	};
#endif

namespace LCD_UI {
	using HI_BD = HardwareInterfaces::LCD_Display;
	using ListStatus = UI_DisplayBuffer::ListStatus;

	///////////////////////////////////////////
	// ************   UI_Object **************
	///////////////////////////////////////////

	Collection_Hndl * UI_Object::select(Collection_Hndl * from) {
		return 0;
	}

	HI_BD::CursorMode UI_Object::cursorMode(const Object_Hndl * activeElement) const {
		if (activeElement) {
			if (this == activeElement->get()) 
				return HI_BD::e_selected;
		}
		return HI_BD::e_unselected;
	}

	int UI_Object::cursorOffset(const char * data) const {
		return data ? strlen(data) - 1 : 0;
	}

	bool UI_Object::streamToBuffer(const char * data, UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, ListStatus listStatus) const {
		return buffer.hasAdded
		(data
			, cursorMode(activeElement)
			, cursorOffset(data)
			, endPos
			, listStatus
		);
	}

	///////////////////////////////////////////
	// ************   Collection_Hndl *********
	///////////////////////////////////////////
	Collection_Hndl::Collection_Hndl(const I_SafeCollection & safeCollection) : Collection_Hndl(safeCollection, safeCollection.focusIndex()) {}
	Collection_Hndl::Collection_Hndl(const UI_Object & object) : Object_Hndl(object) {}
	Collection_Hndl::Collection_Hndl(const UI_Cmd & object) : Object_Hndl(object.get()) {}

	Collection_Hndl::Collection_Hndl(const UI_Object * object) : Object_Hndl(object) {}

	Collection_Hndl::Collection_Hndl(const I_SafeCollection & safeCollection, int default_focus)
		: Object_Hndl(safeCollection) {
		if (auto collection = get()->collection()) {
			collection->filter(filter_selectable()); // collection() returns the _objectHndl, cast as a collection.
			auto validFocus = collection->nextActionableIndex(default_focus);
			set_focus(validFocus); // must call on mixed collection of Objects and collections
			move_focus_by(0); // recycle if allowed. 
		}
	}

	CursorMode Collection_Hndl::cursorMode(const Object_Hndl * activeElement) const {
		using HI_BD = HardwareInterfaces::LCD_Display;
		if (activeElement) {
			if (this == reinterpret_cast<const Collection_Hndl*>(activeElement->get())) 
				return HI_BD::e_selected;
		}
		return HI_BD::e_unselected;
	}

	int Collection_Hndl::cursorOffset(const char * data) const {
		return data ? strlen(data) - 1 : 0;
	}

	Collection_Hndl * Collection_Hndl::activeUI() { //  returns validated focus element - index is made in-range
		if (auto collection = get()->collection()) {
			auto validFocus = collection->validIndex(focusIndex());
			auto newObject = collection->item(validFocus);
			if (newObject == 0) {
				newObject = collection->item(0);
				setFocusIndex(collection->objectIndex());
			}
			return static_cast<Collection_Hndl*>(newObject);
		}
		else return this;
	} // must return polymorphicly

	int	Collection_Hndl::set_focus(int index) {
		if (index < 0) index = 0;
		else if (atEnd(index)) index = endIndex();
		setFocusIndex(index);
		return index;
	}

	void Collection_Hndl::focusHasChanged() {
		get()->focusHasChanged(false);
	}

	Collection_Hndl * Collection_Hndl::move_focus_to(int index) {
#ifdef ZPSIM
		logger() << F("\tmove_focus_to on ") << ui_Objects()[(long)get()].c_str() << L_endl;
#endif		
		auto newObject = this;
		auto collection = get()->collection();
		if (!empty() && collection) {
			newObject = collection->move_focus_to(index);
			bool wasSelected = (_backUI == newObject);
			if (wasSelected) {
				_backUI = newObject;
			}
		}
		return newObject;
	}

	bool Collection_Hndl::move_focus_by(int nth) { // move to next nth selectable item
		auto collection = get()->collection();
	#ifdef ZPSIM
		logger() << F("\tmove_focus_by on ") << ui_Objects()[(long)get()].c_str() << L_endl;
	#endif
		if (collection) {
			move_focus_to(focusIndex());
			const int startFocus = focusIndex(); // might be endIndex()
			auto move_behaviour = behaviour();
			////////////////////////////////////////////////////////////////////
			//************* Lambdas evaluating algorithm *********************//
			////////////////////////////////////////////////////////////////////
			const bool wantToCheckCurrentPosIsOK = { nth == 0 };
			auto wantToMoveForward = [](int nth) {return nth > 0; };
			auto wantToMoveBackwards = [](int nth) {return nth < 0; };
			auto firstValidIndexLookingForwards = [this](int index) {return get()->collection()->nextActionableIndex(index); };
			auto firstValidIndexLookingBackwards = [this](int index) {return get()->collection()->prevActionableIndex(index); };
			const auto weCouldNotMove = startFocus;
			
			auto isIteratedKeepFocus = [collection]() {
				return collection->iterableObjectIndex() >= 0 && (*collection)[collection->iterableObjectIndex()]->behaviour().is_next_on_UpDn();
			};

			auto isInIteratedCollection = [this]() {
				bool back_isIterated = false;
				if (backUI() && backUI()->get()->isCollection()) {
					back_isIterated = backUI()->get()->collection()->iterableObjectIndex() >= 0;
				}
				return back_isIterated;
			};

			auto iterationCanRecycle = [isInIteratedCollection, this]() {
				return isInIteratedCollection() ? !backUI()->behaviour().is_IteratedNoRecycle() : false;
			};
			////////////////////////////////////////////////////////////////////
			//************************  Algorithm ****************************//
			////////////////////////////////////////////////////////////////////
			bool canRecycle = iterationCanRecycle() || !isInIteratedCollection() && move_behaviour.is_recycle();
			bool mustKeepFocus = isIteratedKeepFocus() || (move_behaviour.is_viewOne() && (backUI() ? !backUI()->behaviour().is_IterateOne() : true));

			get()->collection()->filter(filter_selectable());
			if (wantToCheckCurrentPosIsOK) {
				setFocusIndex(firstValidIndexLookingForwards(startFocus));
				if (atEnd(focusIndex()) && canRecycle) setFocusIndex(firstValidIndexLookingForwards(0));
				else if (atEnd(focusIndex()))
					Collection_Hndl::move_focus_by(-1);
			}
			else {
				while (wantToMoveForward(nth)) {
					auto newFocus = focusIndex() + 1;
					newFocus = firstValidIndexLookingForwards(newFocus);
					setFocusIndex(newFocus);
					if (newFocus <= startFocus) break;
					else if (!atEnd(newFocus)) --nth; // We have a valid element
					else if (canRecycle) setFocusIndex(-1);
					else { // at end, can't recycle
						if (mustKeepFocus) setFocusIndex(weCouldNotMove);
						break;
					}
				}
				while (wantToMoveBackwards(nth)) {  // Assume move-back from a valid start-point or focusIndex() of -1 (no actionable elements)
					auto newFocus = focusIndex() - 1;
					newFocus = firstValidIndexLookingBackwards(newFocus);
					setFocusIndex(newFocus); // if no prev, is now -1.
					if (newFocus >= startFocus) break;
					else if (newFocus >= 0) ++nth; // We found one!
					else if (canRecycle) {
						setFocusIndex(endIndex()); // No more previous valid elements, so look from the end.
						move_focus_to(endIndex());
						if (newFocus == 0) break;
					}
					else {
						if (mustKeepFocus || cursorMode(this) == HI_BD::e_inEdit) setFocusIndex(weCouldNotMove); // if can't move out to left-right == view-one or in edit.
						break;
					}
				}
			}
			return isIteratedKeepFocus() || (startFocus != focusIndex() && !atEnd(focusIndex()) && focusIndex() > -1);
		}
		else {
			if (nth) return get()->focusHasChanged(nth > 0);
			else return false;
		}
	}

	///////////////////////////////////////////
	// ************   Coll_Iterator ***********
	///////////////////////////////////////////

	Coll_Iterator & Coll_Iterator::operator++() {
		_index = _collection->nextActionableIndex(_index + 1);
		return *this;
	}

	Coll_Iterator & Coll_Iterator::operator--() {
		_index = _collection->prevActionableIndex(_index - 1);
		return *this;
	}

	I_SafeCollection & Coll_Iterator::operator*() { // index must be in-range
	//UI_Object & Coll_Iterator::operator*() { // index must be in-range
		//auto & collectibleHndl = _collection->at(_index); // returns the element as a collectibleHndl
		//auto & collection_Proxy = static_cast<const Collection_Hndl&>(collectibleHndl); // Interpret it as a Proxy
		//auto & safeCollection = collection_Proxy.get()->collection(); // return its pointer as a safeCollection reference
		auto object = static_cast<Collection_Hndl&>(*_collection->item(_index)).get();
		auto collection = object->collection();
		if (collection) return *collection; else return static_cast<I_SafeCollection &>(*object);
	}

	///////////////////////////////////////////
	// ************   Custom_Select ***********
	///////////////////////////////////////////

	Custom_Select::Custom_Select(OnSelectFnctr onSelect, Behaviour behaviour)
		: _behaviour(behaviour)
		, _onSelectFn(onSelect)
	{}

	void Custom_Select::set_OnSelFn_TargetUI(Collection_Hndl * obj) {
		_onSelectFn.setTarget(obj);
	}

	void Custom_Select::set_UpDn_Target(Collection_Hndl * obj) {
		_upDownTarget = obj;
		behaviour().make_UD();
	}

	Collection_Hndl * Custom_Select::select(Collection_Hndl *) {
		if (_onSelectFn.targetIsSet()) {
			_onSelectFn();
		}
		return 0;
	}

	bool Custom_Select::move_focus_by(int moveBy) {
		if (_upDownTarget) {
			return _upDownTarget->move_focus_by(moveBy);
		}
		else return false;
	}

	///////////////////////////////////////////
	// **********   I_SafeCollection **********
	///////////////////////////////////////////
	Behaviour I_SafeCollection::_filter = filter_viewable();

	I_SafeCollection::I_SafeCollection(int count, Behaviour behaviour) :_count(count), _behaviour(behaviour) {}

	bool I_SafeCollection::isActionableObjectAt(int index) const {
		auto object = item(index);
		if (object) return object->get()->behaviour().is(_filter);
		else return false;
	}

	int I_SafeCollection::nextActionableIndex(int index) const { // index must be in valid range including endIndex()
		// returns endIndex() if none found
		if (objectIndex() > index) {// Current position must be <= requested index to get correct answer
			item(0);
		}
		while (!atEnd(index) && !isActionableObjectAt(index)) { //Database query moves record to next valid ID		
			index = nextIndex(index);
		}
		if (atEnd(index)) {
			I_SafeCollection::setObjectIndex(endIndex());
		} else {
			setObjectIndex(index); // does nothing for database queries.
		}
		return objectIndex();
	}

	int I_SafeCollection::prevActionableIndex(int index) const { // index must in valid range including endIndex()
		// Returns -1 of not found
		// Lambdas
		auto tryAnotherMatch = [index, this](int index, bool match) {return !(objectIndex() <= index && match) && objectIndex() >= 0; };
		auto leastOf = [](int index, int objectIndex) { return index < objectIndex ? index : objectIndex; };
		// Algorithm
		if (objectIndex() < index) { // Current position must be >= requested index to get correct answer
			item(endIndex());
		}		
		setObjectIndex(index);
		auto match = isActionableObjectAt(index);
		index = leastOf(index, objectIndex());
		while (index >= 0 && tryAnotherMatch(index, match)) {
			--index;
			match = isActionableObjectAt(index);
			index = leastOf(index, objectIndex());
		}
		return index;
	}

	Coll_Iterator I_SafeCollection::begin() {
		nextActionableIndex(0);
		return { this, this->objectIndex() };
	}

	bool I_SafeCollection::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, ListStatus listStatus) const {
		filter(filter_viewable());
		auto hasStreamed = false;
#ifdef ZPSIM
//		cout << "\nI_SafeC_Stream each element of " << ui_Objects()[(long)this] << (activeElement ? " HasActive" : " Inactive") << endl;
#endif
		if (behaviour().is_viewOne()) {
			hasStreamed = activeUI()->get()->streamElement(buffer, activeElement,endPos , listStatus);
		} else {
			for (auto & element : *this) {
				auto i = objectIndex();
#ifdef ZPSIM
//				cout << "\t[" << i << "]: " << ui_Objects()[(long)&element] << endl;
#endif
				if (element.behaviour().is_OnNewLine()) buffer.newLine();
				if (element.collection() && element.behaviour().is_viewOne()) {
					auto thisActiveObj = element.activeUI();
					if (thisActiveObj) {
						auto & object = *thisActiveObj->get();
#ifdef ZPSIM
//						cout << F("\t\tView Object: ") << ui_Objects()[(long)&thisActiveObj] << endl << endl;
#endif
						hasStreamed = object.streamElement(buffer, activeElement, endPos, listStatus);
					} 
#ifdef ZPSIM
					else {
						cout << F("\n\n!!!!!!!!!!!! ERROR: View Object empty !!!!!!!!!! \n\n");
					}
#endif
				} else {
					auto thisActiveObj = activeElement;
					if (focusIndex() != i) {
						thisActiveObj = 0;
					}
#ifdef ZPSIM
//					cout << F("\t\tViewAll. Active: ") << (thisActiveObj ? ui_Objects()[(long)thisActiveObj->get()] : "") << endl;
#endif
					hasStreamed = element.streamElement(buffer, thisActiveObj,endPos , listStatus);
				}
			}
		}
		return hasStreamed;
	}

	Collection_Hndl * I_SafeCollection::move_focus_to(int index) {
		auto obj = static_cast<Collection_Hndl *>(item(index));
		if (obj && obj->behaviour().is_selectable()) {
			setFocusIndex(index);
		}
		return obj;
	}

	Collection_Hndl * I_SafeCollection::move_to_object(int index) {
		auto obj = static_cast<Collection_Hndl *>(item(index));
		return obj;
	}

	void Collection_Hndl::enter_collection(int direction) {
		if (get()->isCollection()) {
			if (direction < 0) {
				set_focus(endIndex());
				move_focus_by(-1);
			} else if (direction > 0) {
				auto focus = focusIndex();
				if (atEnd(focus) || focus == -1) { // if we previously left from the right and have cycled round to re-enter from the left, start at 0.
					set_focus(0);
					move_focus_by(0);
				}
			}
		}
	}

	///////////////////////////////////////////
	// **********   UI_IteratedCollection **********
	///////////////////////////////////////////

	bool UI_IteratedCollection_Hoist::h_streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, ListStatus listStatus) const {
		// Set the activeUI to object-index 0 and Stream all the elements
		// Then move the activeUI to the next object-index and re-stream all the elements
		// Keep going till all members of the activeUI have been streamed
		///		Coll-Focus (moved by LR)
		///			v
		///	{L1, Active[0], Slave[0]}
		///	{L1, Active[1], Slave[1]} <- Active-focus (moved by LR if UD_E)
		///	{L1, Active[2], Slave[2]}

		auto coll_focus = activeElement ? iterated_collection()->I_SafeCollection::focusIndex() : -1;
		// lambdas
		auto endOfBufferSoFar = [&buffer]() {return strlen(buffer.toCStr());};
		auto thisElementIsOnAnewLine = [&buffer](size_t bufferStart) {return buffer.toCStr()[bufferStart -1] == '~';};
		auto removeNewLineSymbol = [](size_t & bufferStart) {--bufferStart; };
		auto collectionHasfocus = [coll_focus, this]() {return (coll_focus >= 0 && coll_focus < iterated_collection()->I_SafeCollection::endIndex()) ? true : false; };
		auto startThisField = [&buffer](size_t bufferStart, bool newLine) {
			buffer.truncate(bufferStart);
			if (newLine) buffer.newLine();
		};

		// algorithm
		auto hasStreamed = false;
		auto & iteratedActiveUI_h = *const_cast<I_SafeCollection*>(iterated_collection())->move_to_object(_iteratedMemberIndex); // This must be the parent field.
		auto & activeMember_h = *const_cast<I_SafeCollection*>(iterated_collection())->activeUI();
		// Initialise for single iteration of remote displays, based on activeMember focus.
		auto numberOfIterations = 1;
		auto itIndex = iteratedActiveUI_h.get()->isCollection() ? iteratedActiveUI_h->focusIndex() : 0;
		auto activeFocus = itIndex;
		if (iterated_collection()->behaviour().is_viewAll_LR() && iteratedActiveUI_h.get()->isCollection()) {
			numberOfIterations = iteratedActiveUI_h->endIndex();
			itIndex = iteratedActiveUI_h->nextActionableIndex(0); // required!
			activeFocus = iteratedActiveUI_h->validIndex(iteratedActiveUI_h->focusIndex());
		}
		_beginIndex = itIndex;

		auto bufferStart = endOfBufferSoFar();
		auto mustStartNewLine = thisElementIsOnAnewLine(bufferStart);
		if (mustStartNewLine) removeNewLineSymbol(bufferStart);
		startThisField(bufferStart, mustStartNewLine);
		iterated_collection()->filter(filter_viewable());
		do {
			auto streamActive = activeElement;
			if (itIndex != activeFocus){
				streamActive = 0;
			}			
			for (auto & object : *iterated_collection()) {
#ifdef ZPSIM
//				cout << F("Iteration-Streaming [") << itIndex << "] " << ui_Objects()[(long)&object] << F(" Iteration-ActiveField ") << ui_Objects()[(long)iteratedActiveUI_h.get()] << " ActiveInd: " << activeFocus << endl;
#endif
				auto collHasfocus = collectionHasfocus();
				auto firstVisIndex = h_firstVisibleItem();
				if (itIndex < firstVisIndex)
					break;
				if(object.isCollection()) object.item(itIndex);
				auto originalBehaviour = object.behaviour();
				object.behaviour().make_viewOne();
				hasStreamed = object.streamElement(buffer, streamActive, _endPos, h_listStatus(itIndex));
				object.behaviour() = originalBehaviour;
				h_endVisibleItem(hasStreamed, itIndex);

				if (_beginShow == -1) {
					_beginShow = itIndex;
				}
				if (collHasfocus && _beginShow != _endShow && (activeFocus < _beginShow || activeFocus >= _endShow)) {
					startThisField(bufferStart, mustStartNewLine);
					itIndex = _beginIndex;
					break;
				}
			}
			++itIndex;
		} while (itIndex < numberOfIterations && (itIndex = iteratedActiveUI_h->nextActionableIndex(itIndex)) < numberOfIterations);
#ifdef ZPSIM
//		logger() << L_endl;
#endif
		if (iteratedActiveUI_h.get()->isCollection()) iteratedActiveUI_h->move_to_object(iteratedActiveUI_h->focusIndex());
		return hasStreamed;
	}

	Collection_Hndl * UI_IteratedCollection_Hoist::h_item(int newIndex) {// return member without moving focus
		auto active = iterated_collection()->activeUI();
		active->get()->collection()->move_to_object(newIndex);
#ifdef ZPSIM
		//cout << F("\tUI_IteratedCollection: ") << ui_Objects()[(long)iterated_collection()] << endl;
		//cout << F("\t\tactive item: ") << ui_Objects()[(long)active->get()->collection()];
		//cout << " NewIndex : " << newIndex << " SubCollection obj: " << iterated_collection()->objectIndex() << " SubCollection Focus: " << iterated_collection()->focusIndex() << endl;
#endif
		return active;
	}

	bool UI_IteratedCollection_Hoist::h_focusHasChanged() {
		auto & coll = *iterated_collection();
		auto & iteratedActiveUI_h = *coll.move_to_object(_iteratedMemberIndex); // This must be the parent field.
		_endShow = 1; _beginIndex = 0; _beginShow = 0;
		if (iteratedActiveUI_h.get()->isCollection()) {
			auto & iteratedActiveColl = *iteratedActiveUI_h.get()->collection();
			_beginIndex = iteratedActiveColl.nextActionableIndex(0);
			_endShow = iteratedActiveColl.endIndex();
			_beginShow = _beginIndex;
			if (_beginShow >= _endShow) {
				_beginShow = iteratedActiveColl.prevActionableIndex(_endShow);
			}
		}
		return true;
	}

	int UI_IteratedCollection_Hoist::h_firstVisibleItem() const {
		auto & iteratedActiveObject = *iterated_collection()->activeUI()->get();
		if (iteratedActiveObject.isCollection()) {
			auto & coll = *iteratedActiveObject.collection();
			auto focus = coll.focusIndex();
			if (focus == coll.endIndex()) focus = _beginShow;
			if (focus >= 0 && _beginShow > focus)
				_beginShow = focus; // move back through the list
			if (_endShow > 0 && _endShow <= focus) { // move forward through the list
				_beginShow = coll.nextIndex(_beginShow);
				_endShow = coll.endIndex();
			}
		}
		return _beginShow;
	}

	ListStatus UI_IteratedCollection_Hoist::h_listStatus(int streamIndex) const {
		auto thisListStatus = ListStatus::e_showingAll;
		if (streamIndex == _beginShow && _beginShow > _beginIndex) thisListStatus = ListStatus::e_notShowingStart;
		auto activeObj = iterated_collection()->activeUI();
		if (activeObj->get()->isCollection() && _endShow < activeObj->endIndex()) {
			thisListStatus = ListStatus(thisListStatus | ListStatus::e_not_showingEnd);
			if (streamIndex > _endShow) thisListStatus = ListStatus(thisListStatus | ListStatus::e_this_not_showing);
		}
		return thisListStatus;
	}

	void UI_IteratedCollection_Hoist::h_endVisibleItem(bool thisWasShown, int streamIndex) const {
		// _endShow is streamIndex after the last visible 
		if (thisWasShown && streamIndex >= _endShow) _endShow = streamIndex + 1;
		if (!thisWasShown && streamIndex < _endShow) _endShow = streamIndex;
	}
}