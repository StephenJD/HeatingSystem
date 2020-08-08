
// *********************** Tests *********************
#include <catch.hpp>
#include "UI_Coll_Hndl.h"

//#include "EEPROM.h"
//#include <Convertions.h>

#include <iostream>
#include <iomanip>

//#define ZPSIM
#define BASIC_TESTS
#define UI_ELEMENT_TESTS

using namespace LCD_UI;
using namespace std;

class ConstLabel : public I_Object {
public:
	ConstLabel(const int & label) : _label(label) {}
	// Queries
	const int & show() const { return _label; }
	operator int() const { return _label; }

private:
	const int _label;
};

class Label : public UI_Object {
public:
	Label(int label, Behaviour behaviour = viewable()) : UI_Object(behaviour), _label(label) {}
	// Queries
	int show() const { return _label; }
	operator int() const { return _label; }
private:
	int _label;
};

class Cmd : public Custom_Select {
public:
	Cmd(int label, OnSelectFnctr onSelect, Behaviour behaviour = selectableViewable()) : Custom_Select(onSelect, behaviour), _label(label) {}
	// Queries
	int show() const { return _label; }
	operator int() const { return _label; }
private:
	int _label;
};

class MyLazyCollection : public LCD_UI::LazyCollection {
public:
	using I_SafeCollection::item;
	MyLazyCollection() : LazyCollection(5) {}
private:
	Object_Hndl & item(int newIndex) override {
		if (newIndex == index() && object().get() != 0) return object();
		setIndex(newIndex);
		return swap(new ConstLabel(newIndex * 2));
	}
};

TEST_CASE("Collection_Indexing_of_non_const") {
	Label L0(0), L1(1), L2(2), L3(3), L4(4);
	auto page_c = makeCollection(L0, L1, L2, L3, L4);

	CHECK(static_cast<Label &>(page_c[0]) == 0);
	CHECK(static_cast<Label &>(page_c[1]) == 1);
	CHECK(static_cast<Label &>(page_c[2]) == 2);
	CHECK(static_cast<Label &>(page_c[4]) == 4);
	static_cast<Label &>(page_c[4]) = 5;
	CHECK(static_cast<Label &>(page_c[4]) == 5);
	CHECK(static_cast<Label &>(page_c.at(5)) == 5);
	CHECK(static_cast<Label &>(page_c.at(-5)) == 0);
}

TEST_CASE("Collection_Clock_MoveFocus_non_const") {
	Cmd C0(0,0, hidden()), C1(1,0), C2(2,0, hidden()), C3(3,0), C4(4, 0,hidden());
	auto page_c = makeCollection(C0, C1, C2, C3, C4);
	auto page_h = UI_Coll_Hndl{ page_c };
	// 1st valid element after default 
	CHECK(page_h.focusIndex() == 1);
	page_h.move_focus_to(0);
	CHECK(page_h.focusIndex() == 1);
	page_h.move_focus_to(3);
	CHECK(page_h.focusIndex() == 3);
	page_h.move_focus_by(1);
	CHECK(page_h.focusIndex() == 1);
	page_h.move_focus_by(-1);
	CHECK(page_h.focusIndex() == 3);
	// Cycling forward to the same place returns failure to move.
	auto succeeded = page_h.move_focus_by(2);
	CHECK(page_h.focusIndex() == 3);
	CHECK(succeeded == false);
}

TEST_CASE("Collection_MoveFocus_non_const") {
	Cmd C0(0,0, hidden()), C1(1,0), C2(2,0, hidden()), C3(3,0), C4(4, 0,hidden());
	auto page_c = makeCollection(C0, C1, C2, C3, C4);
	auto page_h = UI_Coll_Hndl{ page_c, 2, selectableViewAll() };// non-clock behaviour
	// 1st valid element after default 
	CHECK(page_h.focusIndex() == 3);
	// moves to prev valid element
	auto succeeded = page_h.move_focus_by(-1);
	CHECK(page_h.focusIndex() == 1);
	CHECK(succeeded == true);
	// won't move before first valid element
	succeeded = page_h.move_focus_by(-1);
	CHECK(page_h.focusIndex() == 1);
	CHECK(succeeded == false);
	// can't move to invalid element
	page_h.move_focus_to(2);
	CHECK(page_h.focusIndex() == 1);
	// moves to valid element
	page_h.move_focus_to(3);
	CHECK(page_h.focusIndex() == 3);
	// won't move past last valid element
	succeeded = page_h.move_focus_by(1);
	CHECK(page_h.focusIndex() == 3);
	CHECK(succeeded == false);
}

TEST_CASE("Collection_NoValidElement") {
	Cmd C0(0, 0, hidden()), C1(1, 0, hidden()), C2(2, 0, hidden()), C3(3, 0, hidden()), C4(4, 0, hidden());
	auto page_c = makeCollection(C0, C1, C2, C3, C4);
	auto page_h = UI_Coll_Hndl{ page_c };
	CHECK(page_h.focusIndex() == 5);
	auto succeeded = page_h.move_focus_by(0);
	CHECK(succeeded == false);
}

TEST_CASE("Collection_Indexing_of_const") {
	Label L0(0), L1(1), L2(2), L3(3), L4(4);
	const auto c_constPage = makeCollection(L0, L1, L2, L3, L4);
	CHECK(static_cast<const Label &>(c_constPage[0]) == 0);
	CHECK(static_cast<const Label &>(c_constPage[1]) == 1);
	CHECK(static_cast<const Label &>(c_constPage[4]) == 4);
	CHECK(static_cast<const Label &>(c_constPage.at(5)) == 4);
	CHECK(static_cast<const Label &>(c_constPage.at(-5)) == 0);
}

TEST_CASE("Collection_RangeFor_non_const") {
	Label L0(0), L1(1), L2(2), L3(3), L4(4);
	auto page_c = makeCollection(L0, L1, L2, L3, L4);
	int index = 0;
	for (auto & i : page_c) {
		auto & thisInt = static_cast<Label &>(i);
		cout << "Collection_RangeFor : " << thisInt << endl;
		CHECK(thisInt == index);
		thisInt = thisInt * 2;
		++index;
	}
	CHECK(index == 5);
	index = 0;
	for (auto & i : page_c) {
		auto & thisInt = static_cast<Label &>(i);
		cout << "Collection_RangeFor modified: " << thisInt << endl;
		CHECK(thisInt == index * 2);
		++index;
	}
	CHECK(index == 5);
}

TEST_CASE("Collection_RangeFor_const") {
	Label L0(0), L1(1), L2(2), L3(3), L4(4);
	const auto c_constPage = makeCollection(L0, L1, L2, L3, L4);
	int index = 0;
	for (const auto & i : c_constPage) {
		const auto & thisInt = static_cast<const Label &>(i);
		cout << "Const_Collection_RangeFor : " << thisInt << endl;
		CHECK(thisInt == index);
		//thisInt = thisInt * 2;
		++index;
	}
	CHECK(index == 5);
}

TEST_CASE("Collection_RangeFor_hidden") {
	Label L0(0,hidden()), L1(1), L2(2, hidden()), L3(3), L4(4, hidden());
	auto page_c = makeCollection(L0, L1, L2, L3, L4);
	int index = 0;
	for (const auto & i : page_c) {
		const auto & thisInt = static_cast<const Label &>(i);
		cout << "Const_Collection_RangeFor_hidden : " << thisInt << endl;
		switch (index) {
		case 0: CHECK(thisInt == 1); break;
		case 1: CHECK(thisInt == 3); break;
		}
		++index;
	}
	CHECK(index == 2);

	index = 0;	
	for (const auto & i : page_c.filter(hidden())) {
		const auto & thisInt = static_cast<const Label &>(i);
		cout << "Const_Collection_RangeFor_show_hidden : " << thisInt << endl;
		CHECK(thisInt == index);
		++index;
	}
	CHECK(index == 5);
}

TEST_CASE("LazyCollection_Indexing_of_non_const") {
	MyLazyCollection myVarCollection;
	auto & firstLbl = myVarCollection[0];
	static_cast<ConstLabel &>(myVarCollection[0]);
	cout << static_cast<ConstLabel &>(myVarCollection[0]).show() << endl;;
	CHECK(static_cast<ConstLabel &>(myVarCollection[0]) == 0);
	CHECK(static_cast<ConstLabel &>(myVarCollection[1]) == 2);
	CHECK(static_cast<ConstLabel &>(myVarCollection[2]) == 4);
	CHECK(static_cast<ConstLabel &>(myVarCollection[4]) == 8);
	CHECK(static_cast<ConstLabel &>(myVarCollection.at(5)) == 8);
	CHECK(static_cast<ConstLabel &>(myVarCollection.at(-5)) == 0);
}

TEST_CASE("LazyCollection_Indexing_of_const") {
	const MyLazyCollection c_lazyPage;
	const auto & myHdl = c_lazyPage[0];
	auto el0 = static_cast<const ConstLabel &>(c_lazyPage[0]);
	//c_lazyPage[0] = 2;
	CHECK(static_cast<const ConstLabel &>(c_lazyPage[0]) == 0);
	CHECK(static_cast<const ConstLabel &>(c_lazyPage[1]) == 2);
	CHECK(static_cast<const ConstLabel &>(c_lazyPage[2]) == 4);
	CHECK(static_cast<const ConstLabel &>(c_lazyPage[4]) == 8);
	CHECK(static_cast<const ConstLabel &>(c_lazyPage.at(5)) == 8);
	CHECK(static_cast<const ConstLabel &>(c_lazyPage.at(-5)) == 0);
}

TEST_CASE("LazyCollection_RangeFor_non_const") {
	MyLazyCollection myVarCollection;
	int index = 0;
	for (auto & i : myVarCollection) {
		auto & thisInt = static_cast<ConstLabel &>(i);
		CHECK(thisInt == index * 2);
		cout << "LazyCollection_RangeFor : " << thisInt << endl;
		//i = i * 2;
		++index;
	}
	CHECK(index == 5);
}

TEST_CASE("LazyCollection_RangeFor_const") {
	const MyLazyCollection myVarCollection;
	int index = 0;
	for (const auto & i : myVarCollection) {
		const auto & thisInt = static_cast<const ConstLabel &>(i);
		CHECK(thisInt == index * 2);
		//CHECK(i == index * 2);
		cout << "const LazyCollection_RangeFor : " << thisInt << endl;
		//i = i * 2;
		++index;
	}
	CHECK(index == 5);
}

TEST_CASE("Command Change Focus") {
	Cmd C0(0, 0, hidden());
	Cmd	C1(1, { &UI_Coll_Hndl::move_focus_to,3 });
	Cmd C2(2, 0, hidden()), C3(3, 0), C4(4, 0, hidden());
	auto page_c = makeCollection(C0, C1, C2, C3, C4);
	auto page_h = UI_Coll_Hndl{ page_c };
	cout << hex << "C1 addr: " << (long long)&C1 << endl;
	// 1st valid element after default 
	CHECK(page_h.focusIndex() == 1);
	auto element = page_h.activeUI(); // pointer to UI_Coll_Hndl, pointing to C1 object
	// No action because target not set
	element->select(); // calls page.MoveFocusTo()
	CHECK(page_h.focusIndex() == 1);

	C1.set_OnSelFn_TargetUI(page_h);
	element->select(&page_h); // calls page.MoveFocusTo()
	CHECK(page_h.focusIndex() == 3);
}

TEST_CASE("Select Active UI on Page") {
	Label L0(0, hidden()), L1(1), L2(2, hidden()), L3(3), L4(4, hidden());
	Cmd C0(0, 0, hidden()), C1(1, { &UI_Coll_Hndl::move_focus_to,1 }), C2(2, { &UI_Coll_Hndl::move_focus_to,3 }), C3(3, 0), C4(4, 0, hidden());
	cout << "C1 Addr:" << hex << (long long)&C1 << endl;
	cout << "C3 Addr:" << hex << (long long)&C3 << endl;
	auto page1_c = makeCollection(L1, C1);
	auto page2_c = makeCollection(L3, C2, C3);
	cout << "Page1 collection of Objects Addr:" << hex << (long long)&page1_c << endl;
	cout << "Page2 collection of Objects Addr:" << (long long)&page2_c << endl;
	auto page1_h = UI_Coll_Hndl{page1_c,1 };
	auto page2_h = UI_Coll_Hndl{page2_c,1 };
	auto display1_c = makeNestedCollection(page1_h, page2_h);
	cout << "page1_h Coll_Hndl Addr:" << (long long)&page1_h << endl;
	cout << "page2_h Coll_Hndl Addr:" << (long long)&page2_h << endl;
	cout << "display1_c collection of Coll_Hndl Addr:" << (long long)&display1_c << endl;

	auto display1_h = UI_Coll_Hndl{ display1_c };

	C1.set_OnSelFn_TargetUI(display1_h);
	C2.set_OnSelFn_TargetUI(page2_h);

	// 1st valid element after default 
	CHECK(page1_h.focusIndex() == 1);
	CHECK(display1_h.focusIndex() == 0);
	//auto activeUI = static_cast<Cmd &>(*page1_h.activeUI());
	//auto activeUIThing = page1_h.activeUI();
	//auto selectThing = activeUIThing->selectUI(0);
	auto selectedUI = page1_h.select(0);
	//auto & newRecipient = static_cast<Cmd &>(*page1_h.select(0));
	auto newRecipient = page1_h.select();
	newRecipient->select(); // calls page.MoveFocusTo()
	CHECK(display1_h.focusIndex() == 1);
	CHECK(page2_h.focusIndex() == 1);

	//newRecipient = static_cast<Cmd &>(*page2_h.select(0));
	newRecipient = page2_h.select();
	newRecipient->select(); // calls page.MoveFocusTo()
	CHECK(page2_h.focusIndex() == 2);
}