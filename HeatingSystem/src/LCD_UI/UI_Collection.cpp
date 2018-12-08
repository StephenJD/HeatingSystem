#include "UI_Collection.h"
#include "UI_LazyCollection.h"

// For debugging
#include <ostream>
//#include <iomanip>

namespace LCD_UI {
	using namespace std;
	using HI_BD = HardwareInterfaces::DisplayBuffer_I;
	using ListStatus = UI_DisplayBuffer::ListStatus;

	///////////////////////////////////////////
	// ************   UI_Object **************
	///////////////////////////////////////////

	Collection_Hndl * UI_Object::select(Collection_Hndl * from) {
		return 0;
	}

	HI_BD::CursorMode UI_Object::cursorMode(const Object_Hndl * activeElement) const {
		if (activeElement) {
			if (this == activeElement->get()) return HI_BD::e_selected;
		}
		return HI_BD::e_unselected;
	}

	int UI_Object::cursorOffset(const char * data) const {
		return data ? strlen(data) - 1 : 0;
	}

	const char * UI_Object::streamToBuffer(const char * data, UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		auto hasAdded = buffer.hasAdded
		(data
			, cursorMode(activeElement)
			, cursorOffset(data)
			, shortColl ? shortColl->fieldEndPos() : 0
			, shortColl ? shortColl->listStatus(streamIndex) : UI_DisplayBuffer::e_showingAll
		);

		if (shortColl) shortColl->endVisibleItem(hasAdded, streamIndex); // requires index of streamed object
		return buffer.toCStr();
	}

	///////////////////////////////////////////
	// ************   Object_Hndl *************
	///////////////////////////////////////////

	const char * Object_Hndl::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		return get()->streamElement(buffer, activeElement, shortColl, streamIndex);
	}


	///////////////////////////////////////////
	// ************   Collection_Hndl *********
	///////////////////////////////////////////
	Collection_Hndl::Collection_Hndl(const I_SafeCollection & safeCollection) : Collection_Hndl(safeCollection, safeCollection.focusIndex()) {}
	Collection_Hndl::Collection_Hndl(const UI_Object & object) : Object_Hndl(object) {}
	Collection_Hndl::Collection_Hndl(const UI_Object * object) : Object_Hndl(object) {}

	I_SafeCollection & Collection_Hndl::getCollection() {
		// Called when dereferencing an iterator.
		// cast the pointer as a safe collection
		return *get()->collection();
		//return *static_cast<I_SafeCollection *>(const_cast<UI_Object *>(_objectHndl));
	}

	Collection_Hndl::Collection_Hndl(const UI_ShortCollection & shortList_Hndl, int default_focus)
		: Object_Hndl(shortList_Hndl) {
		getCollection().filter(selectable()); // getCollection() returns the _objectHndl, cast as a collection.
		set_focus(getCollection().nextActionableIndex(default_focus)); // must call on mixed collection of Objects and collections
		move_focus_by(0); // recycle if allowed. 
	}

	Collection_Hndl::Collection_Hndl(const I_SafeCollection & safeCollection, int default_focus)
		: Object_Hndl(safeCollection) {
		if (auto collection = get()->collection()) {
			collection->filter(selectable()); // getCollection() returns the _objectHndl, cast as a collection.
			set_focus(collection->nextActionableIndex(default_focus)); // must call on mixed collection of Objects and collections
			move_focus_by(0); // recycle if allowed. 
		}
	}

	CursorMode Collection_Hndl::cursorMode(const Object_Hndl * activeElement) const {
		using HI_BD = HardwareInterfaces::DisplayBuffer_I;
		if (activeElement) {
			if (this == reinterpret_cast<const Collection_Hndl*>(activeElement->get())) return HI_BD::e_selected;
		}
		return HI_BD::e_unselected;
	}

	int Collection_Hndl::cursorOffset(const char * data) const {
		return data ? strlen(data) - 1 : 0;
	}

	Collection_Hndl * Collection_Hndl::activeUI() {
		if (auto collection = get()->collection()) {
			return static_cast<Collection_Hndl*>(collection->item(collection->validIndex(focusIndex())));;
		}
		else return this;
	} // must return polymorphicly

	int	Collection_Hndl::set_focus(int index) {
		if (index < 0) index = 0;
		else if (atEnd(index)) index = endIndex();
		setFocusIndex(index);
		return index;
	}

	void Collection_Hndl::focusHasChanged(bool hasFocus) {
		get()->focusHasChanged(hasFocus);
		if (hasFocus) {
			move_focus_by(0);
		}
	}

	Collection_Hndl * Collection_Hndl::move_focus_to(int index) {
		auto newObject = this;
		if (!empty()) {
			newObject = getCollection().move_focus_to(index);
			bool wasSelected = (_backUI == activeUI());
			newObject = activeUI();
			if (wasSelected) {
				_backUI = newObject;
			}
		}
		return newObject;
	}

	bool Collection_Hndl::move_focus_by(int nth) { // move to next nth selectable item
		move_focus_to(focusIndex());
		const int startFocus = focusIndex(); // might be endIndex()
		////////////////////////////////////////////////////////////////////
		//************* Lambdas evaluating algorithm *********************//
		////////////////////////////////////////////////////////////////////
		const bool wantToCheckCurrentPosIsOK = { nth == 0 };
		auto wantToMoveForward = [](auto nth) {return nth > 0; };
		auto wantToMoveBackwards = [](auto nth) {return nth < 0; };
		auto firstValidIndexLookingForwards = [this](auto index) {return getCollection().nextActionableIndex(index); };
		auto firstValidIndexLookingBackwards = [this](auto index) {return getCollection().prevActionableIndex(index); };
		auto needToRecycle = [this](auto index) {return atEnd(index) && behaviour().is_recycle_on_next(); };
		auto tryMovingForwardByOne = [this](auto index) {/*if (!atEnd(index)) */++index; return index; };
		auto tryMovingBackwardByOne = [this](auto index) {if (index > -1)  --index; return index; };
		auto weHaveMoved = [startFocus](auto index) {return startFocus != index; };
		const auto weCouldNotMove = startFocus;
		////////////////////////////////////////////////////////////////////
		//************************  Algorithm ****************************//
		////////////////////////////////////////////////////////////////////
		getCollection().filter(selectable());
		if (wantToCheckCurrentPosIsOK) {
			setFocusIndex(firstValidIndexLookingForwards(startFocus));
			if (needToRecycle(focusIndex())) setFocusIndex(firstValidIndexLookingForwards(0));
			else if (atEnd(focusIndex()))
				Collection_Hndl::move_focus_by(-1);
		}
		else {
			while (wantToMoveForward(nth)) {
				setFocusIndex(tryMovingForwardByOne(focusIndex()));
				setFocusIndex(firstValidIndexLookingForwards(focusIndex()));
				if (!weHaveMoved(focusIndex()))  break;
				else if (!atEnd(focusIndex())) --nth; // We have a valid element
				else if (behaviour().is_recycle_on_next()) setFocusIndex(-1);
				else { // at end, can't recycle
					if (behaviour().is_viewOneUpDn()) setFocusIndex(weCouldNotMove);
					break;
				}
			}
			while (wantToMoveBackwards(nth)) {  // Assume move-back from a valid start-point or focusIndex() of -1 (no actionable elements)
				setFocusIndex(tryMovingBackwardByOne(focusIndex()));
				setFocusIndex(firstValidIndexLookingBackwards(focusIndex())); // if no prev, is now -1.
				if (!weHaveMoved(focusIndex())) break;
				else if (focusIndex() >= 0) ++nth; // We found one!
				else if (behaviour().is_recycle_on_next()) {
					setFocusIndex(endIndex()); // No more previous valid elements, so look from the end.
					move_focus_to(endIndex());
					if (focusIndex() == 0) break;
				}
				else {
					if (behaviour().is_viewOneUpDn() || cursorMode(this) == HI_BD::e_inEdit) setFocusIndex(weCouldNotMove); // if can't move out to left-right == view-one or in edit.
					break;
				}
			}
		}
		return weHaveMoved(focusIndex()) && !atEnd(focusIndex()) && focusIndex() > -1;
	}

	const char * Collection_Hndl::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		//std::cout << "Streaming : " << std::hex << (long long)(this) << std::endl;
		if (behaviour().is_viewable()) {
			auto collectionPtr = get()->collection();
			if (behaviour().is_viewAll()) {
				collectionPtr->filter(viewable());
				auto focus_index = focusIndex();
				for (int i = collectionPtr->nextActionableIndex(0); !atEnd(i); i = collectionPtr->nextActionableIndex(++i)) { // need to check all elements on the page
					auto ith_objectPtr = collectionPtr->item(i);
					//auto behav = ith_objectPtr->get()->behaviour().is_viewAll();
					auto ith_collectionPtr = ith_objectPtr->get()->collection();
					if (ith_objectPtr->get()->behaviour().is_OnNewLine())
						buffer.newLine();
					//std::cout << "Streaming ViewAll at: " << (long long)(collHndl) << std::endl;
					auto lastVisibleIndex = collectionPtr->endVisibleItem();
					if (lastVisibleIndex) {
						if (i < collectionPtr->firstVisibleItem()) continue;
						if (i > lastVisibleIndex) break;
					}

					if (ith_collectionPtr && ith_collectionPtr->behaviour().is_viewAll()) { // get the handle pointing to the nested collection
						ith_collectionPtr->streamElement(buffer, activeElement, static_cast<const I_SafeCollection *>(ith_objectPtr->get()), i);
					}
					else {
						ith_objectPtr->streamElement(buffer, activeElement, shortColl, streamIndex);
					}
					auto debug = buffer.toCStr();
				}
			}
			else if (collectionPtr) {
				auto active = activeUI();
				if (active) active->streamElement(buffer, activeElement, shortColl, streamIndex);
			}
			else {
				get()->streamElement(buffer, activeElement, shortColl, streamIndex);
			}
		}
		return 0;
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
		_onUpDown = obj;
	}

	Collection_Hndl * Custom_Select::select(Collection_Hndl *) {
		if (_onSelectFn.targetIsSet()) {
			_onSelectFn();
		}
		return 0;
	}

	bool Custom_Select::move_focus_by(int moveBy) {
		if (_onUpDown) {
			return _onUpDown->move_focus_by(moveBy);
		}
		else return false;
	}

	///////////////////////////////////////////
	// **********   I_SafeCollection **********
	///////////////////////////////////////////
	Behaviour I_SafeCollection::_filter = viewable();

	int I_SafeCollection::nextActionableIndex(int index) const { // index must be in valid range including endIndex()
		// returns endIndex() if none found
		setObjectIndex(index);
		while (!atEnd(objectIndex()) && !isActionableObjectAt(objectIndex()) && !atEnd(objectIndex())) {
			setObjectIndex(nextIndex(objectIndex()));
		}
		return objectIndex();
	}

	int I_SafeCollection::prevActionableIndex(int index) const { // index must in valid range including endIndex()
		// Returns -1 of not found
		// Lambdas
		auto tryAnotherMatch = [index, this](auto index, auto match) {return !(objectIndex() <= index && match) && objectIndex() >= 0; };
		auto leastOf = [](auto index, auto objectIndex) { return index < objectIndex ? index : objectIndex; };
		// Algorithm
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

	const char * I_SafeCollection::streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const {
		// only called for viewall.
		filter(viewable());
		auto focus_index = focusIndex();
		for (int i = nextActionableIndex(0); !atEnd(i); i = nextActionableIndex(++i)) { // need to check all elements on the page
			auto element = item(i)->get();
			if (item(i)->get()->behaviour().is_OnNewLine()) buffer.newLine();
			auto lastVisibleIndex = shortColl->endVisibleItem();
			if (lastVisibleIndex) {
				if (i < shortColl->firstVisibleItem()) continue;
				if (i > lastVisibleIndex) break;
			}
			if (element->collection() && element->behaviour().is_viewOne()) {
				auto collHndl = static_cast<const Collection_Hndl *>(item(i));
				auto active = collHndl->activeUI();
				if (active) active->streamElement(buffer, activeElement, shortColl, streamIndex);
			}
			else element->streamElement(buffer, activeElement, shortColl, i);
		}
		return buffer.toCStr();
	}

	Collection_Hndl * I_SafeCollection::move_focus_to(int index) {
		auto obj = static_cast<Collection_Hndl *>(item(index));
		if (obj && obj->behaviour().is_selectable()) {
			setFocusIndex(index);
		}
		return obj;
	}

	///////////////////////////////////////////
	// **********   UI_ShortCollection **********
	///////////////////////////////////////////

	UI_ShortCollection::UI_ShortCollection(int endPos, I_SafeCollection & safeCollection)
		: I_SafeCollection(safeCollection.endIndex(), viewable())
		, _nestedCollection(&safeCollection)
		, _endPos(endPos)
		, _endShow(endIndex())
	{
		std::cout << "Sort_Coll at: " << (long long)this << " with collHdl at " << (long long)&_nestedCollection << " to: " << (long long)collection() << std::endl;
	}

	void UI_ShortCollection::focusHasChanged(bool hasFocus) {
		collection()->focusHasChanged(hasFocus);
		collection()->begin();
		_beginIndex = objectIndex();
		setCount(collection()->endIndex());
		auto end = _beginIndex + 1;
		_endShow = end;
		if (_beginIndex > _beginShow) _beginShow = _beginIndex;
		if (_beginShow >= end) {
			_beginShow = end - 1;
			_endShow = end;
		}
	}

	int UI_ShortCollection::firstVisibleItem() const {
		auto focus = collection()->focusIndex();
		if (focus == endIndex()) focus = _beginShow;
		if (focus >= 0 && _beginShow > focus)
			_beginShow = focus; // move back through the list
		if (_endShow <= focus) { // move forward through the list
			_beginShow = collection()->nextIndex(_beginShow);
			_endShow = endIndex();
		}
		return _beginShow;
	}

	ListStatus UI_ShortCollection::listStatus(int streamIndex) const {
		auto thisListStatus = ListStatus::e_showingAll;
		if (streamIndex == _beginShow && _beginShow > _beginIndex) thisListStatus = ListStatus::e_notShowingStart;
		if (_endShow < endIndex()) {
			thisListStatus = ListStatus(thisListStatus | ListStatus::e_not_showingEnd);
			if (streamIndex > _endShow) thisListStatus = ListStatus(thisListStatus | ListStatus::e_this_not_showing);
		}
		return thisListStatus;
	}

	void UI_ShortCollection::endVisibleItem(bool thisWasShown, int streamIndex) const {
		// endShow is 1st element NOT to show.
		if (thisWasShown && streamIndex >= _endShow) _endShow = streamIndex + 1;
		if (!thisWasShown && streamIndex < _endShow) _endShow = streamIndex;
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
		//auto & collectibleHndl = _collection->at(_index); // returns the element as a collectibleHndl
		//auto & collection_Proxy = static_cast<const Collection_Hndl&>(collectibleHndl); // Interpret it as a Proxy
		//auto & safeCollection = collection_Proxy.getCollection(); // return its pointer as a safeCollection reference
		return static_cast<Collection_Hndl&>(*_collection->item(_index)).getCollection();
	}
}