#pragma once
#include "UI_Collection.h"

namespace LCD_UI {
	class UI_ShortCollection;

	/////////////////////////////////////////////////////////////////////////
	//         Menu decorator for catching move_by() on a collection handle 
	/////////////////////////////////////////////////////////////////////////

	//class UI_MenuCollection : public I_SafeCollection {
	//public:
	//	UI_MenuCollection(const I_SafeCollection & hiddenCollection, I_SafeCollection & safeCollection)
	//		: I_SafeCollection(safeCollection.count(), viewOneUpDnRecycle())
	//		, _hidden_pages_hndl(hiddenCollection)
	//		, _collection(&safeCollection)
	//	{
	//	}

	//	// Polymorphic Queries
	//	using UI_Object::behaviour;
	//	Behaviour & behaviour() override { return _collection->behaviour(); }
	//	bool isCollection() const override { return _collection->isCollection(); }
	//	const char * streamElement(UI_DisplayBuffer & buffer
	//		, const Object_Hndl * activeElement
	//		, const I_SafeCollection * shortColl
	//		, int streamIndex) const override {
	//		return _collection->streamElement(buffer, activeElement, this, streamIndex);
	//	}

	//	// Polymorphic Modifiers		
	//	Object_Hndl & item(int newIndex) override { return _collection->item(newIndex); }
	//	void focusHasChanged(bool hasFocus) override;

	//	// new Modifier
	//	void set_cmd_h(Collection_Hndl & obj) { _cmd_c_hndl = &obj; }

	//private:
	//	I_SafeCollection * _collection;
	//	Collection_Hndl _hidden_pages_hndl;
	//	Collection_Hndl * _cmd_c_hndl = 0;
	//};

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
		LazyCollection(int count, Behaviour behaviour) : I_SafeCollection(count, behaviour) {}

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
}