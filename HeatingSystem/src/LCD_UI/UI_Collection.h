#pragma once
#include "Behaviour.h"
#include "UI_DisplayBuffer.h"
#ifdef ZPSIM 
	#include <iostream> 
#endif
//using namespace std;

namespace LCD_UI {
	class UI_DisplayBuffer;
	class Object_Hndl;
	class Collection_Hndl;
	class I_SafeCollection;
	class UI_ShortCollection;
	/// <summary>
	/// Base-class for Arbitrary sized streamable element types, pointed to by Object_Handles.
	/// Public interface provides for streaming, run-time switchable bahaviour and selection.
	/// </summary>
	class UI_Object {
	public:
		UI_Object() = default;
		virtual bool isCollection() const { return false; }
		virtual const I_SafeCollection * collection() const { return 0; }
		// Queries supporting streaming
		using HI_BD = HardwareInterfaces::LCD_Display;
		virtual const char *		streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex = 0) const { return 0; }
		virtual HI_BD::CursorMode	cursorMode(const Object_Hndl * activeElement) const;
		virtual int					cursorOffset(const char * data) const;
		virtual bool				upDn_IsSet() { return false; }
		const char *				streamToBuffer(const char * data, UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const;
		const Behaviour				behaviour() const { return const_cast<UI_Object*>(this)->behaviour(); }
		// Modifiers
		virtual I_SafeCollection *	collection() { return 0; }
		virtual Collection_Hndl *	select(Collection_Hndl * from);
		virtual Collection_Hndl *	edit(Collection_Hndl * from) { return select(from); }
		virtual bool				back() { return true; }
		virtual Behaviour &			behaviour() = 0;
		void						addBehaviour(Behaviour::BehaviourFlags b) { behaviour().addBehaviour(b); }
		void						removeBehaviour(Behaviour::BehaviourFlags b) { behaviour().removeBehaviour(b); }
		virtual void				focusHasChanged(bool hasFocus) {}

		virtual ~UI_Object() = default;
	};

	class OnSelectFnctr {
	public:
		OnSelectFnctr(Collection_Hndl * (Collection_Hndl::* mfn)(int), int arg) : member_ptr(mfn), _arg(arg) {}
		OnSelectFnctr(int) {}
		OnSelectFnctr() = default;
		// Queries
		bool				targetIsSet() const { return _obj != 0 && member_ptr != 0; }
		//Modifiers
		void				setTarget(Collection_Hndl * obj) { _obj = obj; }
		Collection_Hndl *	getTarget() { return _obj; }

		Collection_Hndl *	operator()();
	private:
		Collection_Hndl * (Collection_Hndl::* member_ptr)(int) = 0;
		Collection_Hndl * _obj = 0;
		int _arg = 0;
	};

	inline Collection_Hndl * OnSelectFnctr::operator()() {
		return (_obj->*member_ptr)(_arg);
	}

	/// <summary>
	/// The Base-class for UI_Objects that perform a special action on select or up/dn.
	/// </summary>
	class Custom_Select : public UI_Object {
	public:
		Custom_Select(OnSelectFnctr onSelect, Behaviour behaviour);
		using UI_Object::behaviour;
		// Query
		bool				upDn_IsSet() override { return _onUpDown != 0; }

		// Modifiers
		Behaviour &			behaviour() override { return _behaviour; }
		void				set_OnSelFn_TargetUI(Collection_Hndl * obj);
		void				set_UpDn_Target(Collection_Hndl * obj);
		Collection_Hndl *	select(Collection_Hndl * from) override;
		bool				move_focus_by(int moveBy);
		Collection_Hndl *	target() { return _onSelectFn.getTarget(); }
		virtual ~Custom_Select() = default;
	private:
		OnSelectFnctr _onSelectFn;
		Collection_Hndl * _onUpDown = 0;
		Behaviour _behaviour;
	};

	/// <summary>
	/// Collectible pointer wrapper, to give type-safety and a name to a pointer.
	/// Provides streaming of the pointed-to object.
	/// Offers empty() and get()
	/// Object_Hndl will outward static_cast to the underlying UI_Object reference for use.
	/// </summary>
	class Object_Hndl {
	public:
		Object_Hndl() = default;
		Object_Hndl(const UI_Object * object) : _objectHndl(object) {}
		Object_Hndl(const UI_Object & object) : _objectHndl(&object) {}
		Object_Hndl(UI_Object & object) : _objectHndl(&object) {}
		Object_Hndl(const Object_Hndl & rhs) : _objectHndl(rhs.get()) {}
		Object_Hndl &			operator=(const UI_Object & rhs) { _objectHndl = &rhs; return *this; }

		// Polymorphic Queries
		virtual const char *	streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const /*override*/;

		// New Queries
		bool					empty() const { return _objectHndl == 0; }
		bool					operator==(Object_Hndl rhs) const { return _objectHndl == rhs._objectHndl; }
		const UI_Object *		get() const { return const_cast<Object_Hndl*>(this)->get(); }

		template <typename RT>
		explicit				operator const RT & () const { return static_cast<const RT &>(*_objectHndl); }

		const UI_Object &		operator*() const { return const_cast<Object_Hndl*>(this)->operator*(); }
		const UI_Object *		operator->() const { return const_cast<Object_Hndl*>(this)->operator->(); }

		// New Modifiers
		UI_Object *				get() { return const_cast<UI_Object *>(_objectHndl); }
		void					reset(UI_Object * newObject) { _objectHndl = newObject; }

		template <typename RT>
		explicit				operator RT & () { return static_cast<RT &>(*const_cast<UI_Object*>(_objectHndl)); }

		UI_Object &				operator*() { return *get(); }
		UI_Object *				operator->() { return get(); }

	protected:
		const UI_Object * _objectHndl = 0; // Const required so we can take const * constructor args. pointer to an array of object pointers
	};

	//////////////////////////////////////////////////////////////////////
	//                   Collection Handle Base Class                   //
	//////////////////////////////////////////////////////////////////////
	class I_SafeCollection;
	class UI_Cmd;
	/// <summary>
	/// An Object_Hndl pointing to an I_SafeCollection - possibly a nested collection of Collection_Hndl
	/// Provides atEnd(int pos) and getItem()
	/// Adds backUI.
	/// </summary>
	class Collection_Hndl : public Object_Hndl {
	public:
		// Casting from a I_SafeCollection to a Object_Hndl adds a 4-address offset
		Collection_Hndl() = default;
		Collection_Hndl(const UI_Object * object);
		explicit Collection_Hndl(const UI_Object & object);
		explicit Collection_Hndl(const UI_Cmd & object); // required to avoid ambiguous construction due to dual-base-types.
		explicit Collection_Hndl(const I_SafeCollection & safeCollection);
		explicit Collection_Hndl(const I_SafeCollection & safeCollection, int default_focus);
		explicit Collection_Hndl(const UI_ShortCollection & shortList_Hndl, int default_focus = 0);

		// Polymorphic Queries
		bool atEnd(int pos) const;
		virtual const Behaviour behaviour() const { return get()->behaviour(); }
		const Collection_Hndl * activeUI() const { return const_cast<Collection_Hndl *>(this)->activeUI(); }

		// New Queries
		const I_SafeCollection & operator*() const { return const_cast<Collection_Hndl *>(this)->operator*(); }
		const I_SafeCollection * operator->() const { return const_cast<Collection_Hndl *>(this)->operator->(); }
		UI_Object * getItem(int index) const { return const_cast<Collection_Hndl*>(this)->getItem(index); } // Must return polymorphicly		
		int	focusIndex() const;
		int endIndex() const;
		const Collection_Hndl * backUI() const { return _backUI; }
		virtual CursorMode cursorMode(const Object_Hndl * activeElement) const;
		virtual int cursorOffset(const char * data) const;

		// Polymorphic Modifiers
		const char * streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex = 0) const override;

		virtual Collection_Hndl * on_back() { return backUI(); }   // function is called on the active object to notify it has been de-selected. Used by Edit_Data.
		virtual Collection_Hndl * on_select() { return backUI(); } // action performed when saving. Used by Edit_Data.
		Collection_Hndl * activeUI(); //  returns validated focus element - index is made in-range
		virtual bool move_focus_by(int moveBy); // true if moved
		virtual int	set_focus(int index); // Range-checked. Returns new focus.
		virtual void setCursorPos() {}

		// New Modifiers
		UI_Object * getItem(int index); // Must return polymorphicly
		I_SafeCollection & operator*() { return *get()->collection(); }
		I_SafeCollection * operator->() { return get()->collection(); }
		void focusHasChanged(bool hasFocus);
		void setFocusIndex(int index);

		template<typename RT>
		explicit operator RT & () { return reinterpret_cast<RT &>(*this); } // prevent further offset being applied

		Collection_Hndl * move_focus_to(int index); // Returns new recipient
		Collection_Hndl * backUI() { return _backUI; }
		void setBackUI(Object_Hndl * back) { _backUI = static_cast<Collection_Hndl *>(back); }

	private:
		Collection_Hndl * _backUI = 0;
	};

	class I_SafeCollection;

	class Coll_Iterator {
	public:
		Coll_Iterator(const I_SafeCollection * collection, int index) : _collection(const_cast<I_SafeCollection *>(collection)), _index(index) {}
		// queries
		const I_SafeCollection & operator*() const { return const_cast<Coll_Iterator *>(this)->operator*(); }

		bool operator==(Coll_Iterator other) const { return _index == other._index; }
		bool operator!=(Coll_Iterator other) const { return _index != other._index; }
		int index() const { return _index; }
		// modifiers
		I_SafeCollection & operator*();
		Coll_Iterator & operator++();
		Coll_Iterator & operator--();
	private:
		I_SafeCollection * _collection;
		int _index;
	};

	//////////////////////////////////////////////////////////////////////
	//                   Collection Base Classes                        //
	//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// The Public Interface for rangeable collections.
	/// It assumes the elements are Object_Hndl objects.
	/// It implements streamElement() to stream all its elements
	/// Provides atEnd(int pos), behavour(), focus(). objectIndex() used by lazy-derivatives. Object size: 8 bytes.
	/// </summary>
	class I_SafeCollection : public UI_Object {
	public:
		I_SafeCollection(int count, Behaviour behaviour) :_count(count), _behaviour(behaviour) {}
		// Polymorphic Queries
		bool isCollection() const override { return true; }
		const char * streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl = 0, int streamIndex = 0) const override;

		// Polymorphic short-list query functions
		virtual int firstVisibleItem() const { return 0; }
		virtual int endVisibleItem() const { return 0; }
		virtual int fieldEndPos() const { return 0; }
		virtual UI_DisplayBuffer::ListStatus listStatus(int streamIndex) const { return UI_DisplayBuffer::e_showingAll; }
		virtual void endVisibleItem(bool thisWasShown, int streamIndex) const {}
		virtual int objectIndex() const { return _index; }

		// New Queries
		virtual int	focusIndex() const { return _focusIndex; }
		bool atEnd(int pos) const {
			return pos >= _count;
		}
		virtual int nextIndex(int index) const { return ++index; }
		int validIndex(int index) const { return index < 0 ? 0 : (atEnd(index) ? _count - 1 : index); }
		bool indexIsInRange(int index) const { return index >= 0 && !atEnd(index); }
		const Object_Hndl * item(int index) const { return const_cast<I_SafeCollection*>(this)->item(index); }
		const Object_Hndl & operator[](int index) const { return const_cast<I_SafeCollection*>(this)->operator[](index); }
		int endIndex() const { return _count; }
		Coll_Iterator begin() const { return const_cast<I_SafeCollection*>(this)->begin(); }
		Coll_Iterator end() const { return const_cast<I_SafeCollection*>(this)->end(); }

		const I_SafeCollection & filter(Behaviour behaviour) const { return const_cast<I_SafeCollection*>(this)->filter(behaviour); }
		virtual bool isActionableObjectAt(int index) const {
			auto object = item(index);
			if (object) return object->get()->behaviour().is(_filter);
			else return false;
		}

		/// <summary>
		/// Returns first valid index starting from index and going forwards.
		/// </summary>
		int nextActionableIndex(int index) const; // returns _count if none found

		/// <summary>
		/// Returns first valid index starting from index and going backwards.
		/// </summary>
		int prevActionableIndex(int index) const; // returns -1 if none found

		template <typename RT>
		explicit operator const RT & () const { return static_cast<const RT &>(*static_cast<const UI_Object*>(this)); }

		// Modifiers
		I_SafeCollection * collection() override { return this; }
		const I_SafeCollection * collection() const override { return this; }
		virtual Object_Hndl * item(int index) = 0; // returns object pointer at index [-1 to end()], if it exists.
		virtual void setObjectIndex(int index) const { _index = index; }
		Coll_Iterator begin();
		Coll_Iterator end() { return { this, _count }; }
		void setCount(int count) { _count = count; }
		virtual void setFocusIndex(int focus) { _focusIndex = focus; }
		I_SafeCollection & filter(Behaviour behaviour) { _filter = behaviour; return *this; }
		using UI_Object::behaviour;
		Behaviour & behaviour() override { return _behaviour; }
		Collection_Hndl * move_focus_to(int index);

		template <typename RT>
		explicit operator RT & () { return static_cast<RT &>(*static_cast<UI_Object*>(this)); }

		Object_Hndl & operator[](int index) { return *item(index); }
	protected:
		static Behaviour _filter;
		int8_t _count = 0;
		Behaviour _behaviour;
		int8_t _focusIndex = 0; // index of selected object in this Collection
	private:
		mutable int8_t _index = 0; // used by lazy-collections to show which object is being delivered. Placed here to avoid padding.
	};

	inline UI_Object * Collection_Hndl::getItem(int index)  // Must return polymorphicly
	{
		if (get()) {
			auto object = get()->collection()->item(index);
			if (object) return object->get();
		}
		return 0;
	}

	inline bool Collection_Hndl::atEnd(int pos) const { return empty() ? 0 : get()->collection()->atEnd(pos); }
	inline int Collection_Hndl::focusIndex() const { return /*get()->collection() ? */get()->collection()->focusIndex()/* : 0*/; }
	inline void Collection_Hndl::setFocusIndex(int index) { get()->collection()->setFocusIndex(index); }
	inline int Collection_Hndl::endIndex() const { return get()->collection()->endIndex(); }

	////////////////////////////////////////////////////////////////////////
	////                        Dummy-Collection                          //
	////////////////////////////////////////////////////////////////////////

	///// <summary>
	///// Dummy Collection of Collection_Hndl.
	///// </summary>
	//class DummyCollection : public I_SafeCollection {
	//public:
	//	DummyCollection(int count, Behaviour behaviour) : I_SafeCollection(count, behaviour), _handle(this) {}
	//	using I_SafeCollection::item;
	//	// Modifiers
	//	Collection_Hndl * item(int newIndex) { return &_handle; }
	//	Collection_Hndl & object() { return _handle; }
	//private:
	//	Collection_Hndl _handle;
	//};

	/////////////////////////////////////////////////////////////////////////
	//         Short List decorator for any I_SafeCollection derivative 
	/////////////////////////////////////////////////////////////////////////

	/// <summary>
	/// A Collection-Wrapper providing limited-width view-all collections.
	/// It acts as a proxy to the nested collection and is the collection seen by the UI (i.e. the parent handle).
	/// When indexed with item() it returns the nested item() and reports the nested collection behaviour.
	/// This is necessary so that it can capture left-right move requests from its parent handle.
	/// This means that its parent handle->get will see the short-list, not its nested collection.
	/// If the contained collection is a UI_FieldData its Field_Interface_h.backUI() expects to be pointing at the parent of UI_FieldData
	/// and parent->get() should be castable to a UI_FieldData object. This means that the nested collection.activeUI() should have
	/// its backUI() set to a handle in the short collection, which points to the nested collection.
	/// </summary>
	class UI_ShortCollection : public I_SafeCollection {
	public:
		// Zero-based endPos, endPos=0 means no characters are displayed. 
		UI_ShortCollection(int endPos, I_SafeCollection & safeCollection);

		// Polymorphic Queries
		using UI_Object::behaviour;
		Behaviour & behaviour() override { return collection()->behaviour(); }
		HI_BD::CursorMode cursorMode(const Object_Hndl * activeElement) const override { return collection()->cursorMode(activeElement); }
		int cursorOffset(const char * data) const override { return collection()->cursorOffset(data); }
		int objectIndex() const override { return collection()->objectIndex(); }

		bool isCollection() const override { return collection()->isCollection(); }
		const char * streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, const I_SafeCollection * shortColl, int streamIndex) const override;

		int firstVisibleItem() const override;
		int fieldEndPos() const override { return _endPos; }
		UI_DisplayBuffer::ListStatus listStatus(int streamIndex) const override;
		void endVisibleItem(bool thisWasShown, int streamIndex) const override;
		int endVisibleItem() const override { return _endShow; } // streamIndex after the last visible 

		// Polymorphic Modifiers
		I_SafeCollection * collection() override { return _nestedCollection->collection(); }
		const I_SafeCollection * collection() const override { return _nestedCollection->collection(); }

		Object_Hndl * item(int newIndex) override { return collection()->item(newIndex); }
		void focusHasChanged(bool hasFocus) override;
		void setObjectIndex(int index) const override { I_SafeCollection::setObjectIndex(index); collection()->setObjectIndex(index); }
		void setFocusIndex(int focus) override { I_SafeCollection::setFocusIndex(focus); collection()->I_SafeCollection::setFocusIndex(focus); }

	private:
		int endIndex() const { return collection()->endIndex(); }
		Collection_Hndl _nestedCollection;
		const int _endPos; // 0-based character endIndex on the display for the end of this list; i.e. no of visible chars including end markers: < ... >.
		mutable int _beginShow = 0; // streamIndex of first visible element
		mutable int _endShow; // streamIndex after the last visible element
		int _beginIndex = 0; // streamIndex of first element in collection
	};

	//////////////////////////////////////////////////////////////////////
	//                   Static Collection Base Class                   //
	//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// A wrapper for arrays of any UI_Object-type to make them copiable.
	/// Can be brace-initialised with compile-time constant values
	/// Provided to populate Collections
	/// </summary>
	template <int noOfObjects, typename ObjectType>
	struct ArrayWrapper { // must be POD to be brace-initialised. No base-class.
		const ObjectType & operator[](int index) const { return const_cast<ArrayWrapper*>(this)->operator[](index); }
		// modifiers
		ObjectType & operator[](int index) { return _array[index]; }
		ObjectType _array[noOfObjects];
	};

	/// <summary>
	/// Template-class for holding static SafeCollections of any type.
	/// Objects are held in an array in this object.
	/// This class is not collectible.
	/// </summary>
	template<int noOfObjects, typename ObjectType>
	class Collection : public I_SafeCollection {
	public:
		using I_SafeCollection::item;

		Collection(ArrayWrapper<noOfObjects, ObjectType> arrayWrp, Behaviour behaviour) :I_SafeCollection(noOfObjects, behaviour), _array(arrayWrp) {
#ifdef ZPSIM
			std::cout << "Collection at : " << std::hex << (long long)(this) << std::endl;
			for (int i = 0; i < noOfObjects; ++i)
				std::cout << "   Has Element at : " << (long long)&_array[i] << " Pointing to : " << (long long)_array[i].get() << std::endl;
			std::cout << std::endl;
#endif
		}
		ObjectType * item(int index) override {
			setObjectIndex(index);
			return indexIsInRange(index) ? &_array[index] : 0;
		} // can't return Collection_Hndl *
		Collection & set(Behaviour behaviour) { _behaviour = behaviour; return *this; }

	private:
		ArrayWrapper<noOfObjects, ObjectType> _array;
	};

	/// <summary>
	/// A Variadic Helper function to make viewAllUpDnRecycle Static Collections of Collection_Hndl.
	/// The elements have focus() and activeUI()
	/// It takes a list of Collectible objects by reference
	/// </summary>
	template<typename... Args>
	auto makeCollection(Args & ... args) -> Collection<sizeof...(args), Collection_Hndl> {
		return Collection<sizeof...(args), Collection_Hndl>(ArrayWrapper<sizeof...(args), Collection_Hndl>{Collection_Hndl{ args }...}, viewAllRecycle());
	}

}
