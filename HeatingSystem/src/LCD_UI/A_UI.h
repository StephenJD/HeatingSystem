#pragma once
//#include "UI_Collection.h"
#include "Behaviour.h"
#include "UI_DisplayBuffer.h"

//#define _DEBUG_
#ifdef _DEBUG_
	#include <string>
	#include <iomanip>
	#include <iostream>
	//#include <map>
	//extern std::map<const LCD_UI::UI *, std::string> objectNames;
#endif
	
namespace LCD_UI {
	class UI;
	class Edit_Data;
	struct BasicWrapper;
	class ValRange;


	// ********************* UI *******************
	// Single Responsibility: To provide the interface and default behaviour for UI elements
	// A Concrete UI may be an element of a User Interface, such as a page or Field
	// Or it may be part of the data-structure providing the text for a User Interface.
	// Since both the User Interface and the Data it is manipulating need to respond to the same User interactions,
	// both hierarchys must conform to the same interface.
	// Each UI element consists of a UI_Coll_Hndl of sub-elements obtained by executing a call-back.
	// The call-back is a UI member function, typically provided by the parent object, returning the child members.
	// This ensures that if a different parent object is selected in one UI field, dependant fields can get the related members.
	// When a child field is created, it is given a parent function to call to get its members.
	// Thus both the data and the UI pages can be dynamically changed.
	// In addition each UI is given an On-Select callback which is also a member function. 
	// This would typically belong to the Parent UI to move the focus, or to edit the element provided by the parent.
	// A UI element has a modifiable behavioural state being a combination of b_Viewable, B_Selectable, b_EditOnNextItem
	// A UI element hierarchy might be:
	// A Display : a UI_Coll_Hndl of Pages, adds LCD, Keyboard, database keys and goBack()
	// A Page : a UI_Coll_Hndl of Fields
	// A Field : may be a variable-length list of sub-fields
	// or a window on a data UI
	// ******************* Action on Select (click) **********************
	// A page may have several elements with different actions on Select, so the action belongs to the page elements.
	// However, the action may be applied to either 
	//	a) Display, to change page, 
	//  b) Page, to change focus.
	//  c) Field to put it in edit
	//  d) Cmd field to insert a data member.
	// Therefore the member function to be called needs to contain the object it is to be called on.
	// A functor is used to wrap the member-function address with the object address. 
	// The command cannot be given its object address until after that object has been created - typicall after the command is created.
	// Most commands will call their parent as the command object, so when a UI is created,
	// it sets the command address for its children to itself. This can be changed explicitly: set_OnSelFn_TargetUI().
	// Could perhaps avoid the functor by just saving the target pointer and member function pointer
	// But the functor is created from a template using the target type, and this type is used for casting
	// to enable a UI:: member pointer to actually point to functions defined in derived classes.
	// Could this be a templated member function pointer of UI?



	//template<typename T> class UI_Fn;
	//typedef UI_Fn<UI&> * OnSelectFn;
	//typedef UI_Fn<CollectionPtr*> * GetUI_CollectionFn;

	class UI {
	//class UI : public I_SafeCollection {
		// Single Responsibility: To manage the focus, modified by its run-time behaviour, on its base-class CollectionPtr of UI elements
		// UI elements inherit from this class if they need to provide extra or specialised functions.
	public:
		UI();
		//UI(OnSelectFn onSelect, int behaviour, GetUI_CollectionFn getElementsFn, CollectionPtr * collection);
		//UI(int behaviour);
		//UI(OnSelectFn onSelect, int behaviour = Behaviour::viewable);
		//
		//UI(I_SafeCollection & safeCollection, int default_active_index = 0, int behaviour = Behaviour::selectable);
		////UI(UI ** firstElementPtr, int noOfElements, int default_active_index, OnSelectFn onSelect, int behaviour = Behaviour::selectable, GetUI_CollectionFn getElementsFn = 0);

		// Queries
		virtual const char * show() const { return 0; }
		bool operator ==(UI & rhs) const { return this == &rhs; }
		const UI *	activeUI() const { return (const_cast<UI*>(this))->activeUI(); }
		UI * backUI() const { return _backUI; } // function is called on the active object to notify it has been selected. Used by Edit_Data.
		int			focusIndex()  const { return _focusIndex; }

		// short-list query functions
		virtual int firstVisibleItem() const { return 0; }
		virtual int endVisibleItem() const { return getCollection().count(); } // required only by UI_Lists
		virtual void endVisibleItem(bool thisWasShown, int elementIndex) const {} // required only by UI_Lists
		virtual int fieldEndPos() const { return 0; }
		virtual UI_DisplayBuffer::ListStatus listStatus(int elementIndex) const { return UI_DisplayBuffer::e_showingAll; }

		// Modifiers
		virtual UI *	activeUI();
		CollectionPtr *	activeUI_SubAssembly(int sub) {
			UI * gotActiveUI = activeUI();
			if (gotActiveUI) return gotActiveUI->getItem(sub);
			else return 0;
		}
		void		set_OnSelFn_TargetUI(UI & obj);
		void		set_GetElementsFn_TargetUI(UI & obj);
		virtual UI * on_selected(UI *) { return 0; } // function is called on the active object to notify it has been selected. Used by Edit_Data.
		void setBackUI(UI * back) { _backUI = back; } // std::cout << "****** set backUI for " << objectNames[this] << " to " << objectNames[back] << std::endl;}
		virtual UI * saveEdit(const BasicWrapper * data) { return 0; } // return UI of wrapped item. Required to be called on backUI.
		virtual bool move_focus_by(int moveBy);
		// Data
		OnSelectFn _onSelectFn; // Functor pointer called by rec_select()

	protected:
		friend class A_Top_UI;
		using HI_BD = HardwareInterfaces::DisplayBuffer_I;
		// Queries
		HI_BD::CursorMode isActive(const UI * activeElement) const { return (this == activeElement) ? HI_BD::e_selected : HI_BD::e_unselected; }
		bool		invalidElements() const;

		// Modifiers
		virtual int	set_focus(int index);
		void		move_focus_to(int index);
		virtual UI & selectUI(UI *); // This is the default function called if a selector function is not set.
		virtual UI * on_back() { return _backUI; } // function is called on the active object to notify it has been de-selected. Used by Edit_Data.

		virtual void refreshElements();
		void		giveParentAddressToChildren();
		void		setParentAddress(UI * addr) { _parentAddr = addr; }
		//virtual ~UI() {};

		// Data
		Behaviour _behaviour; // for this collection
		UI * _parentAddr;
		int _parentFocus; // Curent Focus of parent object
		UI * _backUI; // the UI that selected this one.
	private:
		// Queries
		const char * streamCollection(UI_DisplayBuffer & buffer, const UI * activeElement) const;
		virtual const char * streamElement(UI_DisplayBuffer & buffer, int elementIndex, const UI * activeElement) const { return getItem(elementIndex)->streamCollection(buffer, activeElement); }
		bool canMoveFocusInThisDirection(int move) const;
		bool focusNotValid_and_NotSame(int was, int trythis) const;
		// Modifiers
		void initialise(int default_active_index);

		// Data
		int _focusIndex; // index of selected object in this UI_Coll_Hndl
		GetUI_CollectionFn _getElementsFn; // UI member function pointer, typically pointing to the parent object, returning the child members	
	};

	template <typename collectionT>
	class UI_T : public UI {
		// An adapter class to allow a UI object to point to a non-UI data collection.
	public:
		UI_T(OnSelectFn onSelect, int behaviour = Behaviour::viewable, GetUI_CollectionFn getElementsFn = 0, CollectionPtr * collection = 0)
			: UI(onSelect, behaviour, getElementsFn, collection) {
			//std::cout << "** UI_T<> Addr:" << std::hex << long long(this)
			//	<< std::dec << " Behaviour:" << behaviour << std::endl;
		}

		// Modifiers	
		collectionT *	activeUI_T() { return static_cast<collectionT *>(getItem(focusIndex())); }
		const collectionT *	activeUI_T() const { return static_cast<const collectionT *>(getUIelement_T(focusIndex())); }
		const collectionT *	getUIelement_T(int index) const { return static_cast<const collectionT *>(getItem(index)); }
		collectionT *	getUIelement_T(int index) { return static_cast<collectionT *>(getItem(index)); }

		virtual ~UI_T() {};
	protected:

	private:
	};

	template<typename retType = UI &>
	class UI_Fn {
	public:
		virtual retType operator()() = 0;
		void target(UI & obj) { _obj = &obj; }
		bool targetIsSet() { return _obj != 0; }
		UI * getTarget() { return _obj; }
	protected:
		UI_Fn() : _obj(0) {}
		UI * _obj;
	};

	template<class UI_target, typename retType = UI &>
	class UI_Derived_Fn : public UI_Fn<retType> {
	public:
		UI_Derived_Fn(retType(UI_target::* mfn)(int), int arg) : member_ptr(mfn), _arg(arg) {}
		UI_Derived_Fn() : member_ptr(0), _arg(0) {}
		retType operator()() override;
	private:
		retType(UI_target::* member_ptr)(int);
		int _arg;
	};

	template<class UI_target, typename retType>
	retType UI_Derived_Fn<UI_target,retType>::operator()() {
			return (static_cast<UI_target *>(UI_Fn<retType>::_obj)->*member_ptr)(_arg);
	}

}