#include "A_UI.h"
#include "Convertions.h"

//std::map<const LCD_UI::UI *, std::string> objectNames;

namespace LCD_UI {
	using namespace std;
	using namespace GP_LIB;
	//

	// ********************* UI *******************
	// Single Responsibility: To provide the interface and default behaviour for UIs
	// _backUI is the object upon which the virtual functions are called.
	//

	UI::UI() :
		_parentAddr(0),
		_onSelectFn(0),
		_behaviour(Behaviour::selectable),
		_focusIndex(0),
		_getElementsFn(0),
		_parentFocus(0)
	{
		//std::cout << "UI Addr: " << std::hex << long long(this) << std::endl;
		initialise(0);
	}

	UI::UI(OnSelectFn onSelect, int behaviour, GetUI_CollectionFn getElementsFn, CollectionPtr * collection)
		:
		_parentAddr(0),
		_onSelectFn(onSelect),
		_behaviour(behaviour),
		_focusIndex(0),
		_getElementsFn(getElementsFn),
		_parentFocus(0)
	{
		initialise(0);
		//std::cout << "** UI Addr: " << std::hex << long long(this) << std::dec << " Behaviour:" << _behaviour << std::endl;
	}

	UI::UI(OnSelectFn onSelect, int behaviour /*, GetUI_CollectionFn getElementsFn, CollectionPtr * collection*/)
		:
		_parentAddr(0),
		_onSelectFn(onSelect),
		_behaviour(behaviour),
		_focusIndex(0),
		_getElementsFn(0),
		_parentFocus(0)
	{
		initialise(0);
	}

	UI::UI(int behaviour)
		:
		_parentAddr(0),
		_onSelectFn(0),
		_behaviour(behaviour),
		_focusIndex(0),
		_getElementsFn(0),
		_parentFocus(0)
	{
		initialise(0);
		//std::cout << "** UI Addr: " << std::hex << long long(this) << std::dec << " Behaviour:" << _behaviour << std::endl;
	}

	UI::UI(I_SafeCollection & safeCollection, int default_active_index, int behaviour)
		: UI_Coll_Hndl{ safeCollection },
		_parentAddr(0),
		_onSelectFn(0),
		_behaviour(behaviour),
		_focusIndex(0),
		_getElementsFn(0),
		_parentFocus(0)
	{
		//std::cout << "UI Addr: " << std::hex << long long(this) << std::endl;
		initialise(default_active_index);
	}	
	
	//UI::UI(UI ** firstElementPtr, int noOfElements, int default_active_index, OnSelectFn onSelect, int behaviour, GetUI_CollectionFn getElementsFn)
	//	: UI_Coll_Hndl{ firstElementPtr, noOfElements },
	//	//	: CollectionPtr{ firstElementPtr, noOfElements },
	//	_parentAddr(0),
	//	_onSelectFn(onSelect),
	//	_behaviour(behaviour),
	//	_focusIndex(0),
	//	_getElementsFn(getElementsFn),
	//	_parentFocus(0)
	//{
	//	//std::cout << "UI Addr: " << std::hex << long long(this) << std::endl;
	//	initialise(default_active_index);
	//}



	void UI::initialise(int default_active_index) {
		_backUI = this;
		giveParentAddressToChildren();
		set_OnSelFn_TargetUI(*this);
		set_GetElementsFn_TargetUI(*this);
		move_focus_by(default_active_index);
	}

	UI * UI::activeUI() {
		return getItem(_focusIndex);
	}

	const char * UI::streamCollection(UI_DisplayBuffer & buffer, const UI * activeElement) const {
		if (invalidElements()) {
			const_cast<UI*>(this)->refreshElements();
		}
		if (_behaviour.is_viewable()) {
			if (_behaviour.is_viewAll()) {
				for (int i = firstVisibleItem(); i < endVisibleItem(); ++i) {
					streamElement(buffer, i, activeElement);
				}
			}
			else {
				streamElement(buffer, focusIndex(), activeElement);
			}
		}
		return buffer.toCStr();
	}

	void UI::move_focus_to(int index) {
		if (getItem(index)->_behaviour.is_selectable()) {
			bool wasSelected = (_backUI == activeUI());
			set_focus(index);
			if (wasSelected) {
				_backUI = activeUI();
			}
		}
	}

	bool UI::move_focus_by(int nth) { // move to next nth selectable item
		if (!empty() && _focusIndex >= 0 && (nth != 0 || ((!activeUI()->_behaviour.is_selectable())))) {
			if (nth == 0) nth = 1;
			const int startFocus = _focusIndex;
			int direction = (nth > 0) ? 1 : -1;
			do {
				set_focus(nextIndex(0, _focusIndex, 0 + count() - 1, direction));
			} while (focusNotValid_and_NotSame(startFocus, _focusIndex) || (nth -= direction) != 0);
			if (startFocus == _focusIndex) {
				set_focus(-1);
				return false;
			}
			else return true;
		}
		return false;
	}

	bool UI::focusNotValid_and_NotSame(int was, int trythis) const {
		if (!empty()) {
			auto item = getItem(trythis);
			return (was != trythis && !getItem(trythis)->_behaviour.is_selectable());
		}
		else return false;
	}

	int UI::set_focus(int index) { // for moving focus to a specified element as defined by an enum member
		if (index < 0) index = 0;
		if (index >= count()) index = count() - 1;
		_focusIndex = index;
		return _focusIndex;
	}

	bool UI::invalidElements() const {
		if (!_getElementsFn) return false;
		UI * target = (*_getElementsFn).getTarget();
		return (/*empty() || backUI()->activeUI() == 0 ||*/target != 0 && target->focusIndex() != _parentFocus);
	}

	bool UI::canMoveFocusInThisDirection(int move) const {
		if (!_behaviour.is_viewAll()) return false;
		if (focusIndex() + move >= count()) return false;
		if (focusIndex() + move < 0) return false;
		return true;
	}

	UI & UI::selectUI(UI * from) { // This is the default function called if a selector function is not set.
		//cout << "selectUI() for " << objectNames[this] << endl;
		//cout << "activeUI() : " << objectNames[activeUI()] << endl;
		//cout << "backUI : " << objectNames[_backUI] << endl;

		UI * newRecipient = 0;
		UI & active_UI = *activeUI();
		if (active_UI._behaviour.is_selectable()) {
			active_UI.setBackUI(from);
			newRecipient = active_UI.on_selected(from); // notify recipient it has been selected
			if (!newRecipient) newRecipient = &active_UI;
		}
		return *newRecipient;
	}

	void UI::refreshElements() {
		if (_getElementsFn && _getElementsFn->targetIsSet()) {
			CollectionPtr * newCollection = (*_getElementsFn)();
			if (newCollection) {
				reset(newCollection->get());
				//_count = newCollection->count();
			}
			UI * target = (*_getElementsFn).getTarget();
			_parentFocus = target->focusIndex();
		}
	}

	void UI::giveParentAddressToChildren() {
		for (int i = 0; i < count(); ++i) {
			getItem(i)->setParentAddress(this);
			getItem(i)->set_OnSelFn_TargetUI(*this);
		}
	}

	void UI::set_OnSelFn_TargetUI(UI & obj) { if (_onSelectFn) _onSelectFn->target(obj); }
	void UI::set_GetElementsFn_TargetUI(UI & obj) { if (_getElementsFn)_getElementsFn->target(obj); }


}