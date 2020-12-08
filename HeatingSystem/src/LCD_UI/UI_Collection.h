#pragma once
#include "Behaviour.h"
#include "UI_DisplayBuffer.h"
#include <cassert>

#ifdef ZPSIM 
#include <iostream> 
#include <map>
#include <string>
std::map<long, std::string>& ui_Objects();
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
		virtual bool				isCollection() const { return false; }
		virtual const I_SafeCollection* collection() const { return 0; }
		// Queries supporting streaming
		using HI_BD = HardwareInterfaces::LCD_Display;
		virtual bool				streamElement(UI_DisplayBuffer& buffer, const Object_Hndl* activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const { return false; }
		virtual HI_BD::CursorMode	cursorMode(const Object_Hndl* activeElement) const;
		virtual int					cursorOffset(const char* data) const;
		virtual bool				hasFocus() const { return false; }
		bool						streamToBuffer(const char* data, UI_DisplayBuffer& buffer, const Object_Hndl* activeElement, int endPos, UI_DisplayBuffer::ListStatus listStatus) const;
		const Behaviour				behaviour() const { return const_cast<UI_Object*>(this)->behaviour(); }
		// Modifiers
		virtual I_SafeCollection*	collection() { return 0; }
		virtual bool				upDown(int moveBy, Collection_Hndl* colln_hndl, Behaviour ud_behaviour);
		virtual bool				move_focus_by(int moveBy, Collection_Hndl* colln_hndl) { return false; }
		virtual Collection_Hndl*	select(Collection_Hndl* from);
		virtual Collection_Hndl*	edit(Collection_Hndl* from) { return select(from); }
		virtual bool				back() { return true; }
		virtual Behaviour&			behaviour() = 0;
		virtual bool				focusHasChanged(bool hasFocus) { return false; }

		virtual ~UI_Object() = default;
	};

	class OnSelectFnctr {
	public:
		OnSelectFnctr(Collection_Hndl* (Collection_Hndl::* mfn)(int), int arg) : member_ptr(mfn), _arg(arg) {}
		OnSelectFnctr(int) {}
		OnSelectFnctr() = default;
		// Queries
		bool				targetIsSet() const { return _obj != 0 && member_ptr != 0; }
		//Modifiers
		void				setTarget(Collection_Hndl* obj) { _obj = obj; }
		Collection_Hndl* getTarget() { return _obj; }

		Collection_Hndl* operator()();
	private:
		Collection_Hndl* (Collection_Hndl::* member_ptr)(int) = 0;
		Collection_Hndl* _obj = 0;
		int _arg = 0;
	};

	inline Collection_Hndl* OnSelectFnctr::operator()() {
		return (_obj->*member_ptr)(_arg);
	}

	/// <summary>
	/// The Base-class for UI_Objects that perform a special action on select, up/dn or LR.
	/// </summary>
	class Custom_Select : public UI_Object {
	public:
		Custom_Select(OnSelectFnctr onSelect, Behaviour behaviour);
		using UI_Object::behaviour;

		// Modifiers
		Behaviour&		behaviour() override { return _behaviour; }
		void			set_OnSelFn_TargetUI(Collection_Hndl* obj);
		void			set_UpDn_Target(Collection_Hndl* obj);
		Collection_Hndl* select(Collection_Hndl* from) override;
		bool			upDown(int moveBy, Collection_Hndl* colln_hndl, LCD_UI::Behaviour ud_behaviour) override;

		Collection_Hndl* selTarget() { return _onSelectFn.getTarget(); }
		virtual ~Custom_Select() = default;
	private:
		OnSelectFnctr _onSelectFn;
		Collection_Hndl* _upDownTarget = 0;
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
		Object_Hndl(const UI_Object* object) : _objectHndl(object) {}
		Object_Hndl(const UI_Object& object) : _objectHndl(&object) {}
		Object_Hndl(UI_Object& object) : _objectHndl(&object) {}
		Object_Hndl(const Object_Hndl& rhs) : _objectHndl(rhs.get()) {}
		Object_Hndl& operator=(const UI_Object& rhs) { _objectHndl = &rhs; return *this; }

		// New Queries
		bool					empty() const { return _objectHndl == 0; }
		bool					operator==(Object_Hndl rhs) const { return _objectHndl == rhs._objectHndl; }
		const UI_Object* get() const { return const_cast<Object_Hndl*>(this)->get(); }

		template <typename RT>
		explicit				operator const RT& () const { return static_cast<const RT&>(*_objectHndl); }

		const UI_Object& operator*() const { return const_cast<Object_Hndl*>(this)->operator*(); }
		const UI_Object* operator->() const { return const_cast<Object_Hndl*>(this)->operator->(); }

		// New Modifiers
		UI_Object* get() { return const_cast<UI_Object*>(_objectHndl); }
		void					reset(UI_Object* newObject) { _objectHndl = newObject; }

		template <typename RT>
		explicit				operator RT& () { return static_cast<RT&>(*const_cast<UI_Object*>(_objectHndl)); }

		UI_Object& operator*() { return *get(); }
		UI_Object* operator->() { return get(); }

	protected:
		const UI_Object* _objectHndl = 0; // Const required so we can take const * constructor args. pointer to an array of object pointers
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
		Collection_Hndl(const UI_Object* object);
		explicit Collection_Hndl(const UI_Object& object);
		explicit Collection_Hndl(const UI_Cmd& object); // required to avoid ambiguous construction due to dual-base-types.
		explicit Collection_Hndl(const I_SafeCollection& safeCollection);
		explicit Collection_Hndl(const I_SafeCollection& safeCollection, int default_focus);
		template<int noOfObjects>
		Collection_Hndl(const UI_IteratedCollection<noOfObjects>& shortList_Hndl, int default_focus);

		// Polymorphic Queries
		virtual const Behaviour behaviour() const { return get()->behaviour(); }

		// New Queries
		bool atEnd(int pos) const;
		const Collection_Hndl* activeUI() const { return const_cast<Collection_Hndl*>(this)->activeUI(); }
		const I_SafeCollection& operator*() const { return const_cast<Collection_Hndl*>(this)->operator*(); }
		const I_SafeCollection* operator->() const { return const_cast<Collection_Hndl*>(this)->operator->(); }
		UI_Object* getItem(int index) const { return const_cast<Collection_Hndl*>(this)->getItem(index); } // Must return polymorphicly		
		int	focusIndex() const;
		int endIndex() const;
		const Collection_Hndl* backUI() const { return _backUI; }
		virtual CursorMode cursorMode(const Object_Hndl* activeElement) const;
		virtual int cursorOffset(const char* data) const;

		// Polymorphic Modifiers
		virtual Collection_Hndl* on_back() { return backUI(); }   // function is called on the active object to notify it has been de-selected. Used by Edit_Data.
		virtual Collection_Hndl* on_select() { return backUI(); } // action performed when saving. Used by Edit_Data.
		virtual int	set_focus(int index); // Range-checked. Returns new focus.
		virtual void setCursorPos() {}
		virtual bool move_focus_by(int moveBy) { return get()->move_focus_by(moveBy, this); }
		virtual bool upDown(int moveBy, Behaviour ud_behaviour) { return get()->upDown(moveBy, this, ud_behaviour); }

		// New Modifiers
		Collection_Hndl* activeUI(); //  returns validated focus element - index is made in-range
		UI_Object* getItem(int index); // Must return polymorphicly
		I_SafeCollection& operator*() { return *get()->collection(); }
		I_SafeCollection* operator->() { return get()->collection(); }
		void focusHasChanged();
		void setFocusIndex(int index);
		void enter_collection(int direction);

		template<typename RT>
		explicit operator RT& () { return reinterpret_cast<RT&>(*this); } // prevent further offset being applied

		Collection_Hndl* move_focus_to(int index); // Returns new recipient
		Collection_Hndl* backUI() { return _backUI; }
		void setBackUI(Object_Hndl* back) { _backUI = static_cast<Collection_Hndl*>(back); }

	private:
		Collection_Hndl* _backUI = 0;
	};

	class Coll_Iterator {
	public:
		Coll_Iterator(const I_SafeCollection* collection, int index) : _collection(const_cast<I_SafeCollection*>(collection)), _index(index) {}
		// queries
		const I_SafeCollection& operator*() const { return const_cast<Coll_Iterator*>(this)->operator*(); }

		bool operator==(Coll_Iterator other) const { return _index == other._index; }
		bool operator!=(Coll_Iterator other) const { return _index != other._index; }
		int index() const { return _index; }
		// modifiers
		I_SafeCollection& operator*();
		Coll_Iterator& operator++();
		Coll_Iterator& operator--();
	private:
		I_SafeCollection* _collection;
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
		bool			isCollection() const override { return true; }
		bool			streamElement(UI_DisplayBuffer& buffer, const Object_Hndl* activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override;

		// New Polymorphic Queries
		virtual int		nextIndex(int index) const { return ++index; }
		virtual bool	isActionableObjectAt(int index) const;
		virtual int		iterableObjectIndex() const { return -1; }

		// New Non-Polymorphic Queries
		/// <summary>
		/// Returns first valid index starting from index and going forwards.
		/// </summary>
		int				nextActionableIndex(int index) const; // returns _count if none found

		/// <summary>
		/// Returns first valid index starting from index and going backwards.
		/// </summary>		
		int				prevActionableIndex(int index) const; // returns -1 if none found

		int				objectIndex() const { return _index; }
		int				focusIndex() const { return _focusIndex; }
		int				endIndex() const { return _count; }
		auto			activeUI() const -> const Collection_Hndl* { return static_cast<const Collection_Hndl*>(item(validIndex(focusIndex()))); }

		bool			atEnd(int pos) const { return pos >= endIndex(); }
		int				validIndex(int index) const { return index < 0 ? 0 : (atEnd(index) ? endIndex() - 1 : index); }
		bool			indexIsInRange(int index) const { return index >= 0 && !atEnd(index); }
		auto			item(int index) const -> const Object_Hndl* { return const_cast<I_SafeCollection*>(this)->item(index); }
		auto			operator[](int index) const -> const  Object_Hndl& { return const_cast<I_SafeCollection*>(this)->operator[](index); }
		Coll_Iterator	begin() const { return const_cast<I_SafeCollection*>(this)->begin(); }
		Coll_Iterator	end() const { return const_cast<I_SafeCollection*>(this)->end(); }

		auto			filter(Behaviour behaviour) const -> const I_SafeCollection& { return const_cast<I_SafeCollection*>(this)->filter(behaviour); }

		template <typename RT>
		explicit		operator const RT& () const { return static_cast<const RT&>(*static_cast<const UI_Object*>(this)); }

		bool			hasFocus() const override { return focusIndex() == objectIndex(); }

		// Specialised Polymorphic Modifiers
		I_SafeCollection* collection() override { return this; }
		const I_SafeCollection* collection() const override { return this; }
		using UI_Object::behaviour;
		Behaviour& behaviour() override { return _behaviour; }
		//ObjectFnPtr		upDn_Fn() override;
		bool			move_focus_by(int moveBy, Collection_Hndl* colln_hndl) override;

		// New Polymorphic Modifiers
		virtual Object_Hndl* item(int index) = 0; // returns object pointer at index [-1 to end()], if it exists.
		virtual void	setObjectIndex(int index) const { _index = index; }
		virtual void	setFocusIndex(int focus) { _focusIndex = focus; }
		virtual Collection_Hndl* move_focus_to(int index);

		// Non-Polymorphic Modifiers
		Collection_Hndl* activeUI();
		Coll_Iterator	begin();
		Coll_Iterator	end() { return { this, endIndex() }; }
		void			setCount(int count) { _count = count; }
		I_SafeCollection& filter(Behaviour behaviour) { _filter = behaviour; return *this; }
		Collection_Hndl* move_to_object(int index); // Move to object without changing focus.

		template <typename RT>
		explicit		operator RT& () { return static_cast<RT&>(*static_cast<UI_Object*>(this)); }

		Object_Hndl& operator[](int index) { return *item(index); }
	protected:
		static Behaviour _filter;
		int8_t _count = 0;
		Behaviour _behaviour;
		int8_t _focusIndex = 0; // index of selected object in this Collection
	private:
		mutable int8_t _index = 0; // used by lazy-collections to show which object is being delivered. Placed here to avoid padding.
	};

	inline UI_Object* Collection_Hndl::getItem(int index)  // Must return polymorphicly
	{
		if (get()) {
			auto object = get()->collection()->item(index);
			if (object) return object->get();
		}
		return 0;
	}

	inline bool Collection_Hndl::atEnd(int pos) const { return empty() ? 0 : get()->collection()->atEnd(pos); }
	inline int Collection_Hndl::focusIndex() const { return get()->collection() ? get()->collection()->focusIndex() : 0; }
	inline void Collection_Hndl::setFocusIndex(int index) { get()->collection()->I_SafeCollection::setFocusIndex(index); }
	inline int Collection_Hndl::endIndex() const { return get()->collection()->endIndex(); }

	template<int noOfObjects>
	Collection_Hndl::Collection_Hndl(const UI_IteratedCollection<noOfObjects>& shortList_Hndl, int default_focus)
		: Object_Hndl(shortList_Hndl) {
		get()->collection()->filter(filter_selectable()); // collection() returns the _objectHndl, cast as a collection.
		set_focus(get()->collection()->nextActionableIndex(default_focus)); // must call on mixed collection of Objects and collections
		move_focus_by(0); // recycle if allowed. 
	}

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
		const ObjectType& operator[](int index) const { return const_cast<ArrayWrapper*>(this)->operator[](index); }
		// modifiers
		ObjectType& operator[](int index) { return _array[index]; }
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
			_filter = filter_selectable();
			setFocusIndex(nextActionableIndex(0));
			_filter = filter_viewable();
		}

		Collection(Collection<noOfObjects> collection, Behaviour behaviour)
			: I_SafeCollection(noOfObjects, behaviour)
			, _array(collection._array) {
			_filter = filter_selectable();
			setFocusIndex(nextActionableIndex(0));
			_filter = filter_viewable();
		}

		Collection(const Collection<noOfObjects>& collection)
			: I_SafeCollection(collection)
			, _array(collection._array) {
			if (noOfObjects != collection.endIndex()) {
				logger() << F("\n!!! Collection size missmatch. Capacity is ") << noOfObjects << F(" Size is ") << collection.endIndex() << L_endl;
				assert(false);
			}
			_filter = filter_selectable();
			setFocusIndex(nextActionableIndex(0));
			_filter = filter_viewable();
		}

		Collection_Hndl* item(int index) override {
			setObjectIndex(index);
			return indexIsInRange(index) ? &_array[index] : 0;
		}

		Collection& set(Behaviour behaviour) { _behaviour = behaviour; return *this; }

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
		Collection_Hndl* item(int newIndex) override = 0;
		Collection_Hndl& object() { return _handle; }

		~LazyCollection() { delete _handle.get(); }
	protected:
		Collection_Hndl& swap(const UI_Object* newObj) {
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
		bool h_move_iteration_focus_by(int moveBy, Collection_Hndl* colln_hndl);
	protected:
		UI_IteratedCollection_Hoist(int endPos)
			: _endPos(endPos) {}

		virtual const I_SafeCollection* iterated_collection() const = 0;
		int h_firstVisibleItem() const;
		UI_DisplayBuffer::ListStatus h_listStatus(int streamIndex) const;
		void h_endVisibleItem(bool thisWasShown, int streamIndex) const;
		bool h_streamElement(UI_DisplayBuffer& buffer, const Object_Hndl* activeElement, int endPos, UI_DisplayBuffer::ListStatus listStatus) const;
		int16_t h_iterableObjectIndex() const { return _iteratedMemberIndex; }

		virtual I_SafeCollection* iterated_collection() = 0;

		bool h_focusHasChanged();
		Collection_Hndl* h_item(int newIndex);

		int16_t _iteratedMemberIndex = 0;
		const int16_t _endPos; // 0-based character endIndex on the display for the end of this list; i.e. no of visible chars including end markers: < ... >.
		mutable int16_t _beginShow = 0; // streamIndex of first visible element
		mutable int16_t _endShow = 0; // streamIndex after the last visible element
		mutable int16_t _beginIndex = 0; // required streamIndex of first element in collection
	};


	/// <summary>
	/// An Iterated collection views all its members and and provides limited-width. It iterates over its active member.
	/// The Iteration may be view-all(default) or view-one (only one iteration shown).
	/// The nested elements are always internally modified to view-one, as the iteration selects which member is shown.
	/// LR always moves to the next field in the current iteration (if there is one). 
	/// R0 allows LR to move into the next iteration.
	/// R recycles within the current iteration so requires UD_A to escape. 
	/// UD_A/C/E/S apply to all members and override the same settings on the selected member.
	/// UD_A makes UD move to the next iteration. IR0 prevents recycling.
	///		Coll-Focus (moved by LR)
	///			|
	///	{L1, Active[0], Slave[0]} -> LR (next iteration if R0)
	///	{L1, Active[1], Slave[1]} -- Active-focus (moved by UD if set to UD_A)
	///	{L1, Active[2], Slave[2]}
	/// </summary>
	template<int noOfObjects>
	class UI_IteratedCollection : public Collection<noOfObjects>, public UI_IteratedCollection_Hoist {
	public:
		// Zero-based endPos, endPos=0 means no characters are displayed. 
		UI_IteratedCollection(int endPos, Collection<noOfObjects> collection) // Inherit behaviour from the active member
			: Collection<noOfObjects>(collection, {}) // behaviour is for the iteration. The collection is always view-all. All members modified to view-one.
			, UI_IteratedCollection_Hoist(endPos) {
			_iteratedMemberIndex = Collection<noOfObjects>::focusIndex();
			Collection<noOfObjects>::_filter = filter_viewable();
			auto activeBehaviour = collection[_iteratedMemberIndex]->behaviour();
			Collection<noOfObjects>::behaviour() = activeBehaviour;
			for (auto& object : collection) {
				object.behaviour().make_viewOne();
			}

		}

		UI_IteratedCollection(int endPos, Collection<noOfObjects> collection, Behaviour itBehaviour)
			: Collection<noOfObjects>(collection, itBehaviour) // behaviour is for the iteration. The collection is always view-all. All members modified to view-one.
			, UI_IteratedCollection_Hoist(endPos) {
			_iteratedMemberIndex = Collection<noOfObjects>::focusIndex();
			Collection<noOfObjects>::_filter = filter_viewable();
			for (auto& object : collection) {
				object.behaviour().make_viewOne();
			}
		}

		// Polymorphic Queries

		bool hasFocus() const override { return (Collection<noOfObjects>::focusIndex() == Collection<noOfObjects>::objectIndex()) && Collection<noOfObjects>::activeUI()->get()->hasFocus(); }
		const I_SafeCollection* iterated_collection() const override { return this; }
		bool streamElement(UI_DisplayBuffer& buffer, const Object_Hndl* activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override {
			return h_streamElement(buffer, activeElement, endPos, listStatus);
		}
		int iterableObjectIndex() const override { return h_iterableObjectIndex(); }

		// Polymorphic Modifiers
		I_SafeCollection* iterated_collection() override { return this; }
		bool focusHasChanged(bool hasFocus) override { return h_focusHasChanged(); }

		// New non-polymorphic modifier
		bool move_iteration_focus_by(int moveBy, Collection_Hndl* colln_hndl) { return h_move_iteration_focus_by(moveBy, colln_hndl); }
	};

	template<>
	class UI_IteratedCollection<1> : public Collection<1>, public UI_IteratedCollection_Hoist {
	public:
		// Zero-based endPos, endPos=0 means no characters are displayed. 
		UI_IteratedCollection(int endPos, Collection<1> collection)
			: Collection<1>(collection, Behaviour((collection[0].get()->behaviour() + IR0 + R0)).make_viewAll().make_UD_Cmd()) // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos) {
			item(0)->get()->behaviour().make_viewOne();
		}

		UI_IteratedCollection(int endPos, Collection<1> collection, Behaviour itBehaviour)
			: Collection<1>(collection, Behaviour(itBehaviour)) // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos) {
			item(0)->get()->behaviour().make_viewOne();
		}

		template<int collectionSize> // prevent construction from non-one collections.
		UI_IteratedCollection(int endPos, Collection<collectionSize> collection) = delete;

		UI_IteratedCollection(int endPos, I_SafeCollection& collection)
			: Collection<1>{ ArrayWrapper<1,Collection_Hndl>{Collection_Hndl{collection}}, Behaviour(collection.behaviour() + IR0 + R0).make_viewAll() } // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos)
		{
			item(0)->get()->behaviour().make_viewOne();
		}

		UI_IteratedCollection(int endPos, I_SafeCollection& collection, Behaviour itBehaviour)
			: Collection<1>{ ArrayWrapper<1,Collection_Hndl>{Collection_Hndl{collection}}, Behaviour(itBehaviour) } // behaviour is for the iteration. The collection is always view-all. ActiveUI is always set to view-one.
			, UI_IteratedCollection_Hoist(endPos)
		{
			item(0)->get()->behaviour().make_viewOne();
		}

		// Polymorphic Queries
		const I_SafeCollection* iterated_collection() const override { return this; }
		I_SafeCollection* iterated_collection() override { return this; }
		int iterableObjectIndex() const override { return 0; }

		bool streamElement(UI_DisplayBuffer& buffer, const Object_Hndl* activeElement, int endPos = 0, UI_DisplayBuffer::ListStatus listStatus = UI_DisplayBuffer::e_showingAll) const override {
			return h_streamElement(buffer, activeElement, endPos, listStatus);
		}

		bool hasFocus() const override { return (focusIndex() == objectIndex()) && activeUI()->get()->hasFocus(); }

		// Polymorphic Modifiers
		bool focusHasChanged(bool hasFocus) override { return h_focusHasChanged(); }

		// New non-polymorphic modifier
		bool move_iteration_focus_by(int moveBy, Collection_Hndl* colln_hndl) { return h_move_iteration_focus_by(moveBy, colln_hndl); }
	};

	/// <summary>
	/// A Variadic Helper function to make viewAllUpDnRecycle Static Collections of Collection_Hndl.
	/// The elements have focus() and activeUI()
	/// It takes a list of Collectible objects by reference
	/// </summary>
	template<typename... Args>
	auto makeCollection(Args & ... args) -> Collection<sizeof...(args)> {
		return Collection<sizeof...(args)>(ArrayWrapper<sizeof...(args), Collection_Hndl>{Collection_Hndl{ args }...}, Behaviour{ V + S + VnLR + R });
	}

}
