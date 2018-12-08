#pragma once

#include "Behaviour.h"
#include <iterator>
#include <iostream>
using namespace std;

namespace LCD_UI {
	/////////////////////////////////////////////////////////////
	//        TO DO                                            //
	// Make a Collectible root-class                           //
	// Make CollectionHndl a Collection of Collectible objects   //
	// Usable for collecting volatile-data objects or UI's     //
	/////////////////////////////////////////////////////////////


	/// <summary>
	/// The Base-class for Collectible types.
	/// Collectible types are of known constant size
	/// Pointers to arrays are Collectible, but the arrays are not.
	/// </summary>
	class Collectible {};
	class PolyCollectible : public Collectible {};
	
	/// <summary>
	/// do-nothing Collectible pointer wrapper, to give type-safety and a name to a pointer.
	/// CollectibleHndl will be static_cast to the underlyimg type for use.
	/// </summary>
	class CollectibleHndl : public Collectible {
	public:
		CollectibleHndl(Collectible * collectible) : _collectibleHndl(collectible) {}
		CollectibleHndl(const Collectible * collectible) : _collectibleHndl(collectible) {}
		CollectibleHndl(Collectible & collectible) : _collectibleHndl(&collectible) {
			std::cout << "CollectibleHndl addr: " << std::hex << (long long)this;
			std::cout << " points to : " << (long long)_collectibleHndl << std::endl;
		}

		// Queries
		bool operator==(CollectibleHndl rhs) const { return _collectibleHndl == rhs._collectibleHndl; }
		const Collectible * get() const { return const_cast<CollectibleHndl*>(this)->get(); }
		// Modifier
		const CollectibleHndl & operator*() const { return const_cast<CollectibleHndl*>(this)->operator*(); }
		const CollectibleHndl * operator->() const { return const_cast<CollectibleHndl*>(this)->operator->(); }
		
		template <typename RT>
		explicit operator RT & () { return static_cast<RT &>(*const_cast<Collectible*>(_collectibleHndl)); }

		template <typename RT>
		explicit operator const RT & () const { return static_cast<const RT &>(*_collectibleHndl); }

		CollectibleHndl & operator*() { return *static_cast<CollectibleHndl *>(get()); }
		CollectibleHndl * operator->() { return static_cast<CollectibleHndl *>(get()); }
		Collectible * get() { return static_cast<CollectibleHndl *>(const_cast<Collectible *>(_collectibleHndl)); }
		
		CollectibleHndl & operator++() {
			// needs to move to next Handle
			_collectibleHndl = reinterpret_cast<const CollectibleHndl *>(_collectibleHndl) + 1;
			return *this;
		}

	protected:
		const Collectible * _collectibleHndl; // pointer to an array of object pointers
	};

//////////////////////////////////////////////////////////////////////
//              Wrappers to support Collections                     //
//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// A wrapper to make primitives Collectible.
	/// Implicit inward and outward conversions to underlying type
	/// Helper function, makeCollectible(), provided
	/// </summary>
	template <typename BaseType>
	struct Collectible_T : Collectible {
		Collectible_T(BaseType obj) : _obj(obj) {}
		operator const BaseType &() const { return _obj; }
		operator BaseType & () { return _obj; }
		BaseType _obj;
	};

	/// <summary>
	/// A Helper function to make Collectible wrapped primitives.
	/// </summary>
	template <typename BaseType>
	auto makeCollectible(BaseType obj) { return Collectible_T<BaseType>(obj); }
	
	/// <summary>
	/// A wrapper for arrays of any element-type to make them copiable.
	/// Can be brace-initialised with compile-time constant values
	/// Provided to populate Collections
	/// </summary>
	template <int noOfObjects, typename CollectibleType>
	struct ArrayWrapper { // must be POD to be brace-initialised. No base-class.
		const CollectibleType & operator[](int index) const {return const_cast<ArrayWrapper*>(this)->operator[](index);}
		// modifiers
		CollectibleType & operator[](int index) { return _array[index]; }
		CollectibleType _array[noOfObjects];
	};

	//////////////////////////////////////////////////////////////////////
	//                   Collection Handle Base Class                   //
	//////////////////////////////////////////////////////////////////////
	class SafeCollection;

	/// <summary>
	/// Collectible class, pointing to a SafeCollection - possibly a nested collection of CollectionHndl
	/// </summary>
	class CollectionHndl : public CollectibleHndl {
	public:
		// Casting from a SafeCollection to a CollectibleHndl adds a 4-address offset
		CollectionHndl(SafeCollection & safeCollection);
		CollectionHndl(const CollectibleHndl * collectibleHndl);

		// Queries
		bool operator==(CollectionHndl rhs) const { return _collectibleHndl == rhs._collectibleHndl; }
		bool operator!=(CollectionHndl rhs) const { return _collectibleHndl != rhs._collectibleHndl; }

		const SafeCollection & operator*() const { return const_cast<CollectionHndl *>(this)->operator*(); }
		const SafeCollection * operator->() const { return const_cast<CollectionHndl *>(this)->operator->(); }
		SafeCollection & operator*() { return *getCollection(); }
		SafeCollection * operator->() { return getCollection(); }

		const SafeCollection * getCollection() const { return const_cast<CollectionHndl *>(this)->getCollection(); }

		SafeCollection * getCollection();

		CollectionHndl & operator++() {
			// needs to move to next Handle
			_collectibleHndl = reinterpret_cast<const CollectionHndl *>(_collectibleHndl) + 1;
			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////
	//                   Collection Base Classes                        //
	//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// The Base-class for bounds-checked rangeable collections.
	/// Derived classes must provide item(), count(), begin(), end() and operator[],
	/// all in both const and non-const.
	/// </summary>
	class SafeCollection : public Collectible {
	public:
		// Queries
		virtual int	count() const = 0;
		int validIndex(int index) const { return index < 0 ? 0 : (index >= count() ? count() - 1 : index); }

		const CollectibleHndl & getItem(int index) const { return const_cast<SafeCollection *>(this)->getItem(index); }
		const CollectionHndl & begin() const { return const_cast<SafeCollection*>(this)->begin(); }
		const CollectionHndl & end() const { return const_cast<SafeCollection*>(this)->end(); }
		const CollectibleHndl * item(int index) const { return const_cast<SafeCollection*>(this)->item(index); }

		virtual const CollectionHndl & begin() = 0;
		virtual const CollectionHndl & end() = 0;
		virtual CollectibleHndl * item(int index) = 0;

		template <typename RT>
		explicit operator RT & () { return static_cast<RT &>(*static_cast<Collectible*>(this)); }

		template <typename RT>
		explicit operator const RT & () const { return static_cast<const RT &>(*static_cast<const Collectible*>(this)); }

		CollectibleHndl & getItem(int index) { return *item(validIndex(index)); }

		const CollectibleHndl & operator[](int index) const { return const_cast<SafeCollection*>(this)->operator[](index); }
		// Modifiers
		virtual CollectibleHndl & operator[](int index) = 0;
	};

	//////////////////////////////////////////////////////////////////////
	//                   Lazy-Collection Base Class                     //
	//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// The Base-class for Lazy-Loading SafeCollections.
	/// Derived classes must provide non-const operator[]
	/// and "using LazyCollection::operator[];"
	/// Derived classes may be stateless and therefore collectible.
	/// </summary>
	template<int noOfObjects, typename CollectibleType>
	class LazyCollection : public SafeCollection { // array-semantics wrapper for lazy-initialisation
	public:
		using SafeCollection::operator[];
		using SafeCollection::begin;
		using SafeCollection::end;
		using SafeCollection::item;

		template<typename LazyType>
		class LC_iterator;
		int	count() const override {return noOfObjects;}
		const LC_iterator<LazyCollection> & begin() { return LC_iterator<LazyCollection>{ this,0 }; }
		const LC_iterator<LazyCollection> & end() { return LC_iterator<LazyCollection>{ this, noOfObjects }; }
		CollectibleHndl * item(int index) override { return &operator[](index); }
	};

	template<int noOfObjects, typename CollectibleType>
	template<typename LazyType>
	class LazyCollection<noOfObjects, CollectibleType>::LC_iterator : public CollectionHndl {
	public:
		LC_iterator(LazyType * collection, int index) : CollectionHndl(*collection), _index(index) {}
		// queries
		const CollectibleType & operator*() const { return static_cast<const LazyType&>(*_collectibleHndl)[_index]; }
		bool operator==(LC_iterator other) const { return _index == other._index; }
		// modifiers
		LC_iterator & operator++() { ++_index; return *this; }
		LC_iterator & operator--() { --_index; return *this; }
	private:
		int _index;
	};

	//////////////////////////////////////////////////////////////////////
	//                   Static Collection Base Class                   //
	//////////////////////////////////////////////////////////////////////

	/// <summary>
	/// Template-class for holding static SafeCollections of any type.
	/// Objects are held in an array in this object.
	/// This class is not collectible.
	/// </summary>
	template<int noOfObjects, typename CollectibleType>
	class Collection : public SafeCollection {
	public:
		using SafeCollection::operator[];
		using SafeCollection::begin;
		using SafeCollection::end;
		using SafeCollection::item;

		Collection(ArrayWrapper<noOfObjects, CollectibleType> arrayWrp) : _array(arrayWrp) {
			for (int i = 0; i < noOfObjects; ++i)
				cout << "Element at : " << hex << (long long)&_array[i] << " Contains : " <<  (long long)_array[i].get() << endl;
			cout << endl;
		}
		int	count() const override { return noOfObjects; }

		CollectibleType * item(int index) override { return &operator[](index); } // can't return CollectionHndl *
		const CollectionHndl & begin() override {	return &_array[0]; } // OK returning const CollectibleType &
		const CollectionHndl & end() override { return &_array[noOfObjects]; } // OK returning const CollectibleType &

		// modifiers
		CollectibleType & operator[](int index) {
			index = index >= noOfObjects ? noOfObjects-1 : (index < 0 ? 0 : index );
			return _array[index];
		}
	private:
		ArrayWrapper<noOfObjects, CollectibleType> _array;
	};

	/// <summary>
	/// A Helper function to make Static Collections of any literal type.
	/// </summary>
	template <int noOfObjects, typename CollectibleType>
	auto makeCollection(ArrayWrapper<noOfObjects, CollectibleType> arr) { return Collection<noOfObjects, CollectibleType>(arr); }

	/// <summary>
	/// PolyCollectible class, pointing to a SafeCollection
	/// </summary>
	class CollectionPtr : public PolyCollectible {
		// This is a constant-sized pointer to a collection, enabling this to be collectible
		// Single Responsibility: To provide a common base-class providing bounds-checked access to an array of object pointers. The array must exist outside this object.
		// This class needs getItem() rather than operator[] since it is mostly called via pointers or on this, which is ugly for operator[].
	public:
		CollectionPtr(PolyCollectible ** firstElementPtr, int count);
		CollectionPtr();
		// Queries
		bool	hasElements() const { return _elements != 0 && _count != 0; } //
		const PolyCollectible	* getItem(int index) const { return const_cast<CollectionPtr *>(this)->getItem(index); }
		virtual int count() const { return _count; }
		PolyCollectible ** elements() const { return _elements; }
		// Modifiers
		PolyCollectible *	getItem(int index);
		void setCount(int count) { _count = count; }
	protected:
		PolyCollectible ** _elements; // pointer to an array of object pointers
		int _count; // Number of object pointers in the _elements array
	};

	class UI;

	class UI_Coll_Ptr : public CollectionPtr {
	// Single Responsibility: To allow a CollectionHndl to manage an array of pointers to UI's.
	public:
		UI_Coll_Ptr(UI ** firstElementPtr, int noOfElements) : CollectionPtr(reinterpret_cast<PolyCollectible **>(firstElementPtr), noOfElements) {
			//std::cout << "Collection_of_T: " << std::hex << long long(this) << std::endl;
		}
		// Queries
		const UI *	getItem(int index) const { return reinterpret_cast<UI *>(const_cast<UI_Coll_Ptr *>(this)->getItem(index)); }
		// Modifiers
		UI	* getItem(int index) { return reinterpret_cast<UI*>(CollectionPtr::getItem(index)); }
	};
}
