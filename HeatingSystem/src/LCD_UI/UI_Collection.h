#pragma once
#include "Behaviour.h"
#include "UI_DisplayBuffer.h"

#ifdef ZPSIM 
	#include <iostream> 
	#include <map>
	#include <string>
	std::map<long, std::string> & ui_Objects();
#endif

namespace LCD_UI {
	class Object_Hndl;
	class Collection_Hndl;
	class I_SafeCollection;
	template<int noOfObjects> class UI_IteratedCollection;
	class UI_Cmd;

	/// <summary>
	/// Stateless Abstract Base-class for Arbitrary sized streamable element types, pointed to by Object_Handles.
	/// All concrete classes have behaviour data member, but a stateless baseclass reduces object size. 
	/// Public interface provides for streaming, run-time switchable bahaviour and selection.
	/// </summary>
	class UI_Object {
	public:
		UI_Object() = default;
		virtual bool isCollection() const { return false; }
		virtual const I_SafeCollection * collection() const { return 0; }
		// Queries supporting streaming
		using HI_BD = HardwareInterfaces::LCD_Display;
		virtual bool				streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const { return false; }
		virtual HI_BD::CursorMode	cursorMode(const Object_Hndl * activeElement) const;
		virtual int					cursorOffset(const char * data) const;
		virtual bool				upDn_IsSet() { return false; }
		bool						streamToBuffer(const char * data, UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, UI_DisplayBuffer::ListStatus listStatus) const;
		const Behaviour				behaviour() const { return const_cast<UI_Object*>(this)->behaviour(); }
		// Modifiers
		virtual I_SafeCollection *	collection() { return 0; }
		virtual Collection_Hndl *	select(Collection_Hndl * from);
		virtual Collection_Hndl *	edit(Collection_Hndl * from) { return select(from); }
		virtual bool				back() { return true; }
		virtual Behaviour &			behaviour() = 0;
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
		bool				upDn_IsSet() override { return _upDownTarget != 0; }

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
		Collection_Hndl * _upDownTarget = 0;
		Behaviour _behaviour;
	};

	/// <summary>
	/// Collectible pointer wrapper with no additional state, to give type-safety and a name to a pointer.
	/// Provides streaming of the pointed-to object.
	/// Offers empty() and get()
	/// Object_Hndl will outward static_cast to the underlying UI_Object reference for use.
	/// Object size: 8 bytes
	/// </summary>
	class Object_Hndl {
	public:
		Object_Hndl() = default;
		Object_Hndl(const UI_Object * object) : _objectHndl(object) {}
		Object_Hndl(const UI_Object & object) : _objectHndl(&object) {}
		Object_Hndl(UI_Object & object) : _objectHndl(&object) {}
		Object_Hndl(const Object_Hndl & rhs) : _objectHndl(rhs.get()) {}
		Object_Hndl &			operator=(const UI_Object & rhs) { _objectHndl = &rhs; return *this; }

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
	
	/// <summary>
	/// An Object_Hndl pointing to an I_SafeCollection - possibly a nested collection of Collection_Hndl
	/// Provides atEnd(int pos) and getItem()
	/// Adds state: backUI.
	/// Object size: 12 bytes
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
		template<int noOfObjects>
		explicit Collection_Hndl::Collection_Hndl(const UI_IteratedCollection<noOfObjects> & shortList_Hndl, int default_focus)
			: Object_Hndl(shortList_Hndl) {
				get()->collection()->filter(selectable()); // collection() returns the _objectHndl, cast as a collection.
				set_focus(get()->collection()->nextActionableIndex(default_focus)); // must call on mixed collection of Objects and collections
				move_focus_by(0); // recycle if allowed. 
			}
		
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
		void enter_collection(int direction);

		template<typename RT>
		explicit operator RT & () { return reinterpret_cast<RT &>(*this); } // prevent further offset being applied

		Collection_Hndl * move_focus_to(int index); // Returns new recipient
		Collection_Hndl * backUI() { return _backUI; }
		void setBackUI(Object_Hndl * back) { _backUI = static_cast<Collection_Hndl *>(back); }

	private:
		Collection_Hndl * _backUI = 0;
	};

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
	/// Provides atEnd(int pos), behavour(), focus(). objectIndex() used by lazy-derivatives. 
	/// Object size: 8 bytes.
	/// </summary>
	class I_SafeCollection : public UI_Object {
	public:
		I_SafeCollection(int count, Behaviour behaviour);
		// Polymorphic Queries
		bool isCollection() const override { return true; }
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override;
		virtual Collection_Hndl * leftRight_Collection();

		// Polymorphic short-list query functions
		virtual int firstVisibleItem() const { return 0; }
		virtual int endVisibleItem() const { return 0; }
		virtual int fieldEndPos() const { return 0; }
		virtual UI_DisplayBuffer::ListStatus listStatus(int streamIndex) const { return UI_DisplayBuffer::e_showingAll; }
		virtual void endVisibleItem(bool thisWasShown, int streamIndex) const {}
		virtual int objectIndex() const { return _index; }

		// New Queries
		virtual int	focusIndex() const { return _focusIndex; }
		Collection_Hndl * activeUI() {
			return static_cast<Collection_Hndl*>(item(validIndex(focusIndex())));
		}

		const Collection_Hndl * activeUI() const {
			return static_cast<const Collection_Hndl*>(item(validIndex(focusIndex())));
		}

		bool atEnd(int pos) const {
			return pos >= endIndex();
		}
		virtual int nextIndex(int index) const { return ++index; }
		int validIndex(int index) const { return index < 0 ? 0 : (atEnd(index) ? endIndex() - 1 : index); }
		bool indexIsInRange(int index) const { return index >= 0 && !atEnd(index); }
		const Object_Hndl * item(int index) const { return const_cast<I_SafeCollection*>(this)->item(index); }
		const Object_Hndl & operator[](int index) const { return const_cast<I_SafeCollection*>(this)->operator[](index); }
		virtual int endIndex() const { return _count; }
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
		virtual int nextActionableIndex(int index) const; // returns _count if none found

		/// <summary>
		/// Returns first valid index starting from index and going backwards.
		/// </summary>
		virtual int prevActionableIndex(int index) const; // returns -1 if none found

		template <typename RT>
		explicit operator const RT & () const { return static_cast<const RT &>(*static_cast<const UI_Object*>(this)); }

		// Modifiers
		I_SafeCollection * collection() override { return this; }
		const I_SafeCollection * collection() const override { return this; }
		virtual Object_Hndl * item(int index) = 0; // returns object pointer at index [-1 to end()], if it exists.
		virtual void setObjectIndex(int index) const { _index = index; }
		virtual void setFocusIndex(int focus) { _focusIndex = focus; }
		Behaviour & behaviour() override { return _behaviour; }
		
		Coll_Iterator begin();
		Coll_Iterator end() { return { this, endIndex() }; }
		void setCount(int count) { _count = count; }
		I_SafeCollection & filter(Behaviour behaviour) { _filter = behaviour; return *this; }
		using UI_Object::behaviour;
		virtual Collection_Hndl * move_focus_to(int index);
		Collection_Hndl * move_to_object(int index); // Move to object without changing focus.

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
	inline int Collection_Hndl::focusIndex() const { return get()->collection() ? get()->collection()->focusIndex() : 0; }
	inline void Collection_Hndl::setFocusIndex(int index) { get()->collection()->setFocusIndex(index); }
	inline int Collection_Hndl::endIndex() const { return get()->collection()->endIndex(); }

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
	/// This class is not collectible since it has arbitrary size.
	/// </summary>
	template<int noOfObjects>
	class Collection : public I_SafeCollection {
	public:
		using I_SafeCollection::item;

		Collection(ArrayWrapper<noOfObjects, Collection_Hndl> arrayWrp, Behaviour behaviour) :I_SafeCollection(noOfObjects, behaviour), _array(arrayWrp) {
//#ifdef ZPSIM
//			logger() << F("Collection at : ") << L_hex << (long)(this) << L_endl;
//			for (int i = 0; i < noOfObjects; ++i)
//				logger() << F("   Has Element at : ") << (long)&_array[i] << F(" Pointing to : ") << (long)_array[i].get() << L_endl;
//			std::cout << std::endl;
//#endif
			_filter = filter_selectable();
			setFocusIndex(nextActionableIndex(0));
			_filter = filter_viewable();
		}		
		
		Collection(const Collection<noOfObjects> & collection, Behaviour behaviour) 
			: I_SafeCollection(noOfObjects, behaviour)
			, _array(collection._array) {}

		Collection_Hndl * item(int index) override {
			setObjectIndex(index);
			return indexIsInRange(index) ? &_array[index] : 0;
		}

		Collection & set(Behaviour behaviour) { _behaviour = behaviour; return *this; }

	private:
		ArrayWrapper<noOfObjects, Collection_Hndl> _array;
	};

	//////////////////////////////////////////////////////////////////////
	//                   Lazy-Collection Base Class                     //
	//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// The Base-class for Lazy-Loading SafeCollections of Collection_Hndl.
	/// Derived classes must provide object() & item(int newIndex) override;
	/// and "using I_SafeCollection::item;" Objects size: 20 bytes
	/// </summary>
	class LazyCollection : public I_SafeCollection { // array-semantics wrapper for lazy-initialisation
	public:
		LazyCollection(int count, Behaviour behaviour = { V + S + R + V1 + UD_A }) : I_SafeCollection(count, behaviour) {}

		// Modifiers
		Collection_Hndl * item(int newIndex) override = 0;
		Collection_Hndl & object() { return _handle; }

		~LazyCollection() { delete _handle.get(); }
	protected:
		Collection_Hndl & swap(const UI_Object * newObj) {
			delete _handle.get();
			_handle = newObj;
			return _handle;
		}
	private:
		Collection_Hndl _handle;
	};

////////////////////////////////////////////////////////////////////////
//         Iterated collection with endPos 
/////////////////////////////////////////////////////////////////////////
	class UI_IteratedCollection_Hoist {
	public:
	protected:
		UI_IteratedCollection_Hoist(int endPos)
			: _endPos(endPos)
		{}

		int h_firstVisibleItem() const;
		void h_focusHasChanged(bool hasFocus);
		bool h_streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos, UI_DisplayBuffer::ListStatus listStatus) const;
		UI_DisplayBuffer::ListStatus h_listStatus(int streamIndex) const;
		void h_endVisibleItem(bool thisWasShown, int streamIndex) const;
		Collection_Hndl * h_item(int newIndex);
		virtual const I_SafeCollection * iterated_collection() const = 0;
		virtual I_SafeCollection * iterated_collection() = 0;
		Collection_Hndl * h_leftRight_Collection();
		const int16_t _endPos; // 0-based character endIndex on the display for the end of this list; i.e. no of visible chars including end markers: < ... >.
		mutable int16_t _beginShow = 0; // streamIndex of first visible element
		mutable int16_t _endShow = 0; // streamIndex after the last visible element
		mutable int16_t _beginIndex = 0; // required streamIndex of first element in collection
		int16_t _itFocus = 0;
	};


	/// <summary>
	/// A View-all Collection which iterates its active member and provides limited-width.
	/// The Iteration may be view-all(default) or view-one, but the nested elements are always internally set to view-one, as the iteration selects which member is shown.
	/// If the active member is EditOnUpDn/SaveAfterEdit, Up/Down edits and LR moves to the next iteration, otherwise Up/Down moves to the next iteration and LR moves within the collection.
	/// When shared across different displays, it is used as a viewOne-iteration.
	///
	///		Coll-Focus (moved by LR if not set to b_LR_ActiveMember)
	///			|
	///	{L1, Active[0], Slave[0]}
	///	{L1, Active[1], Slave[1]} -- Active-focus (moved by LR if set to b_LR_ActiveMember)
	///	{L1, Active[2], Slave[2]}
	/// </summary>
	template<int noOfObjects>
	class UI_IteratedCollection : public Collection<noOfObjects>, public UI_IteratedCollection_Hoist {
	public:
		// Zero-based endPos, endPos=0 means no characters are displayed. 
		UI_IteratedCollection(int endPos, Collection<noOfObjects> & collection, Behaviour behaviour = { V + S + Vn + LR_A + UD_0 + R0 })
			: Collection<noOfObjects>(collection, behaviour) // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos)
		{
			if (behaviour.is_NextActive_On_LR()) {
				activeUI()->get()->behaviour() = behaviour;
			}
			activeUI()->get()->behaviour().make_viewOne();
		}

		// Polymorphic Queries
		const I_SafeCollection * iterated_collection() const override { return this; }
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override {
			return h_streamElement(buffer, activeElement, endPos, listStatus); 
		}
		int	focusIndex() const override {return _itFocus;}
		int endVisibleItem() const override { return _endShow; } // streamIndex after the last visible 
		int firstVisibleItem() const override { return h_firstVisibleItem(); }
		int fieldEndPos() const override { return _endPos; }
		UI_DisplayBuffer::ListStatus listStatus(int streamIndex) const override {return h_listStatus(streamIndex);}
		void endVisibleItem(bool thisWasShown, int streamIndex) const override { h_endVisibleItem(thisWasShown, streamIndex); }

		// Polymorphic Modifiers
		I_SafeCollection * iterated_collection() override { return this; }
		void setFocusIndex(int focus) override { _itFocus = focus; }
		Collection_Hndl * leftRight_Collection() { return h_leftRight_Collection(); }

		void focusHasChanged(bool hasFocus) override { return h_focusHasChanged(hasFocus); }
	};
	
	template<>
	class UI_IteratedCollection<1> : public Collection<1>, public UI_IteratedCollection_Hoist {
	public:
		// Zero-based endPos, endPos=0 means no characters are displayed. 
		UI_IteratedCollection(int endPos, Collection<1> & collection)
			: Collection<1>(collection, Behaviour(collection[0].get()->behaviour()+LR_A+R0).make_viewAll()) // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos)
		{
			collection[0].get()->behaviour() = (collection[0].get()->behaviour() + V1+LR_A+R0) & ~UD_A;
		}

		UI_IteratedCollection(int endPos, I_SafeCollection & collection)
			: Collection<1>{ {Collection_Hndl{collection}}, (collection.behaviour()).make_viewAll() } // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos)
		{
			activeUI()->get()->behaviour().make_viewOne();
		}

		// Polymorphic Queries
		const I_SafeCollection * iterated_collection() const override { return this; }
		I_SafeCollection * iterated_collection() override { return this; }
		
		bool streamElement(UI_DisplayBuffer & buffer, const Object_Hndl * activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override {
			return h_streamElement(buffer, activeElement, endPos, listStatus);
		}
		int	focusIndex() const override {return _itFocus;}
		int endVisibleItem() const override { return _endShow; } // streamIndex after the last visible 
		int firstVisibleItem() const override { return h_firstVisibleItem(); }
		int fieldEndPos() const override { return _endPos; }
		UI_DisplayBuffer::ListStatus listStatus(int streamIndex) const override {return h_listStatus(streamIndex);}
		void endVisibleItem(bool thisWasShown, int streamIndex) const override { h_endVisibleItem(thisWasShown, streamIndex); }

		// Polymorphic Modifiers
		void setFocusIndex(int focus) override { _itFocus = focus; }
		Collection_Hndl * leftRight_Collection() { return h_leftRight_Collection(); }

		void focusHasChanged(bool hasFocus) override { return h_focusHasChanged(hasFocus); }
	};

	/// <summary>
	/// A Variadic Helper function to make viewAllUpDnRecycle Static Collections of Collection_Hndl.
	/// The elements have focus() and activeUI()
	/// It takes a list of Collectible objects by reference
	/// </summary>
	template<typename... Args>
	auto makeCollection(Args & ... args) -> Collection<sizeof...(args)> {
		return Collection<sizeof...(args)>(ArrayWrapper<sizeof...(args), Collection_Hndl>{Collection_Hndl{ args }...}, Behaviour{V+S+Vn+UD_A+R});
	}

}
