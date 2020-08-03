
// *********************** Tests *********************
#include <catch.hpp>
#include "UI_Collection.h"
#include "UI_Cmd.h"
#include "LocalDisplay.h"
#include "Logging.h"
#include "A_Top_UI.h"
//#include "EEPROM.h"
//#include <Conversions.h>

#include <iostream>
#include <iomanip>

//#define ZPSIM
#define BASIC_TESTS
#define UI_ELEMENT_TESTS

using namespace LCD_UI;
//using namespace client_data_structures;
//using namespace RelationalDatabase;
using namespace HardwareInterfaces;
//using namespace GP_LIB;
//using namespace Date_Time;
using namespace std;

std::string	test_stream(UI_DisplayBuffer & buffer) {
	std::string result = buffer.toCStr();
	switch (buffer.cursorMode()) {
	case LCD_Display::e_selected:
		result.insert(buffer.cursorPos(), "_");
		break;
	case LCD_Display::e_inEdit:
		result.insert(buffer.cursorPos(), "#");
		break;
	default:
		break;
	}
	return result;
}

class LazyPage : public LCD_UI::LazyCollection {
public:
	using I_SafeCollection::item;
	LazyPage() : LazyCollection(5,viewAllRecycle()) {}
	Collection_Hndl & item(int newIndex) override {
		if (newIndex == objectIndex() && object().get() != 0) return object();
		setObjectIndex(newIndex);
		switch (validIndex(objectIndex())) {
		case 0: return swap(new UI_Label("L5"));
		case 1: return swap(new UI_Cmd("C6", 0));
		case 2: return swap(new UI_Label("L6", hidden()));
		case 3: return swap(new UI_Label("L7"));
		case 4: return swap(new UI_Cmd("C7", 0));
		default: return object();
		};
	}
};

TEST_CASE("Hidden Commands and Labels", "[Cmd/Label]") {
	cout << "\n **** Hidden Commands and Labels ****\n";
	LCD_Display_Buffer<10, 4> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2",hidden()), L3("L3"), L4("L4",hidden());
	UI_Cmd C1("C1", 0, hidden()), C2("C2", 0), C3("C3", 0), C4("C4", 0, hidden());
	auto page1_c = makeCollection(L1, C1, L2, C2, L3, C3, L4, C4);
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C2     L3 C3");
}

TEST_CASE("Lazy Page of Cmd/Label") {
	cout << "\n **** Lazy Page of Cmd/Label ****\n";
	LCD_Display_Buffer<10, 4> lcd;
	UI_DisplayBuffer tb(lcd);

	const LazyPage lazyPage_c;
	auto display1_c = makeDisplay(lazyPage_c);
	auto display1_h = A_Top_UI(display1_c);

	//cout << "lazyPage_c Addr: " << hex << (long long)&lazyPage_c << endl;
	//cout << "LazyElCount : " << lazyPage_c.count() << endl << endl;
	CHECK(lazyPage_c.endIndex() == 5);
	//auto display1_h = A_Top_UI(lazyPage_c);
	display1_h.stream(tb);
	cout << test_stream(display1_h.stream(tb)) << endl;
	CHECK(test_stream(display1_h.stream(tb)) == "L5 C6     L7 C7");
}

TEST_CASE("Check page Focus is valid", "[Page]") {
	cout << "\n **** Check page Focus is valid ****\n";
	LCD_Display_Buffer<80, 1> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1");
	UI_Cmd C1("C1", 0);
	auto page1_c = makeCollection(L1, C1);
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);

	cout <<  hex << "L1 addr: " << (long long)&L1 << endl;
	cout << "C1 addr: " << (long long)&C1 << endl;
	cout << "page1_c addr: " << (long long)&page1_c << endl;
	cout << "display1_h addr: " << (long long)&display1_h << endl;

	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1");
	display1_h.rec_select();
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1");
}

TEST_CASE("Check display Focus is valid", "[Page]") {
	cout << "\n **** Check display Focus is valid ****\n";
	LCD_Display_Buffer<80, 1> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1");
	UI_Cmd C1("C1", 0);
	auto page1_c = makeCollection(L1, C1);
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);

	cout <<  hex << "L1 addr: " << (long long)&L1 << endl;
	cout << "C1 addr: " << (long long)&C1 << endl;
	cout << "page1_c addr: " << (long long)&page1_c << endl;
	cout << "display1_c addr: " << (long long)&display1_c << endl;
	cout << "display1_h addr: " << (long long)&display1_h << endl;

	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1");
	display1_h.rec_select();
	display1_h.stream(tb);
	cout << test_stream(display1_h.stream(tb)) << endl;
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1");
}

TEST_CASE("Move to next Selectable Element - recycle", "[Page]") {
	cout << "\n **** Move to next Selectable Element - recycle ****\n";
	LCD_Display_Buffer<10, 4> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2");
	UI_Cmd C1("C1", 0), C2("C2", 0), C3("C3", 0);

	auto page1_c = makeCollection(L1, C1, L2, C2, C3);
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);

	display1_h.rec_select();
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L2 C2 C3");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_2 C3");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C2 C_3");
	display1_h.rec_left_right(1);
	REQUIRE(test_stream(display1_h.stream(tb)) == "L1 C_1     L2 C2 C3");

	display1_h.rec_left_right(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C2 C_3");
	display1_h.rec_left_right(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_2 C3");
	display1_h.rec_left_right(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L2 C2 C3");
}

TEST_CASE("Command Changes Element Focus", "[Page]") {
	cout << "\n **** Command Changes Element Focus on Page ****\n";
	LCD_Display_Buffer<80, 1> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1");
	UI_Cmd C0("C0", 0, hidden());
	UI_Cmd	C1("C1", { &Collection_Hndl::move_focus_to,0 });
	UI_Cmd C2("C2", { &Collection_Hndl::move_focus_to,4 });
	UI_Cmd C3("C3", { &Collection_Hndl::move_focus_to,1 });
	auto page1_c = makeCollection(L1, C0, C1, C2, C3);
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);

	C1.set_OnSelFn_TargetUI(display1_c.item(0));
	C2.set_OnSelFn_TargetUI(display1_c.item(0));
	C3.set_OnSelFn_TargetUI(display1_c.item(0));

	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1 C2 C3");
	display1_h.rec_select();
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1 C2 C3");
	display1_h.rec_select(); // focus should not change as cmd tries to move to unselectable label.
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1 C2 C3");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1 C_2 C3");
	display1_h.rec_select(); // focus should change as cmd moves focus.
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1 C2 C_3");
	display1_h.rec_select(); // focus should not change as cmd tries to move to unselectable cmd.
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1 C2 C_3");
}

// ********************* Display TESTS *******************

TEST_CASE("Pages are selectable", "[Display]") {
	cout << "\n **** [Display] Pages are selectable ****\n";
	LCD_Display_Buffer<80, 1> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2");
	UI_Cmd C1("C1", 0), C2("C2", 0), C3("C3", 0);

	auto page1_c = makeCollection(L1, C1, C2);
	auto page2_c = makeCollection(L2, C3);
	auto display1_c = makeDisplay(Collection_Hndl { page1_c,2 }, Collection_Hndl{ page2_c,1 });
	auto display1_h = A_Top_UI(display1_c);
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1 C2");
	display1_h.rec_select();
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1 C_2");
}

TEST_CASE("Multi-Page Command Changes Element Focus", "[Display]") {
	cout << "\n **** Multi-Page Command Changes Element Focus ****\n";
	LCD_Display_Buffer<10, 2> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2");
	UI_Cmd C1("C1", { &Collection_Hndl::move_focus_to,4 }), C2("C2", { &Collection_Hndl::move_focus_to,1 }), Ch("Ch", 0, hidden());
	UI_Cmd C3("C3", { &Collection_Hndl::move_focus_to,0 });

	auto page1_c = makeCollection(L1, C1, L2, Ch, C2);
	auto page2_c = makeCollection(L2, C3);
	auto display1_c = makeDisplay(page1_c, page2_c);
	auto display1_h = A_Top_UI(display1_c);

	C1.set_OnSelFn_TargetUI(display1_c.item(0));
	C2.set_OnSelFn_TargetUI(display1_c.item(0));
	C3.set_OnSelFn_TargetUI(display1_h);

	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C2");
	display1_h.rec_select(); // Make page_1 recipient of commands.
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L2 C2");
	display1_h.rec_left_right(1); // move selection to second command
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_2");
	display1_h.rec_select(); // Click on Cmd1 on Page 0. Causes focus to shift
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L2 C2");
}

TEST_CASE("Multi-Page Command Changes Page", "[Display]") {
	cout << "\n **** Multi-Page Command Changes Page ****\n";
	LCD_Display_Buffer<10, 2> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2");
	UI_Cmd C1("C1", { &Collection_Hndl::move_focus_to,4 }), C2("C2", { &Collection_Hndl::move_focus_to,1 });
	UI_Cmd C3("C3", { &Collection_Hndl::move_focus_to,1 }), C4("C4", { &Collection_Hndl::move_focus_to,0 });

	auto page1_c = makeCollection(L1, C1, L2, C3, C2);
	auto page2_c = makeCollection(L2, C4);
	auto display1_c = makeDisplay(page1_c, page2_c);
	auto display1_h = A_Top_UI(display1_c);

	C1.set_OnSelFn_TargetUI(display1_c.item(0));
	C2.set_OnSelFn_TargetUI(display1_c.item(0));
	C3.set_OnSelFn_TargetUI(display1_h);
	C4.set_OnSelFn_TargetUI(display1_h);

	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C3 C2");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L2 C3 C2");
	display1_h.rec_select(); 
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C3 C_2");
	display1_h.rec_left_right(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_3 C2");
	display1_h.rec_select(); // Make page_2 recipient of commands, C4 is ActiveUI.
//	CHECK(test_stream(display1_h.stream(tb)) == "L2 C_4");
	display1_h.rec_select(); // Make page_1 recipient of commands, C4 is ActiveUI.
//	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_3 C2");
}

TEST_CASE("Multi-Page Changed Page retains Selection", "[Display]") {
	cout << "\n **** Multi-Page Changed Page retains Selection ****\n";
	LCD_Display_Buffer<10, 2> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2");
	UI_Cmd C1("C1", { &Collection_Hndl::move_focus_to,4 }), C2("C2", { &Collection_Hndl::move_focus_to,1 });
	UI_Cmd C3("C3", { &Collection_Hndl::move_focus_to,1 }), C4("C4", { &Collection_Hndl::move_focus_to,0 });

	auto page1_c = makeCollection(L1, C1, L2, C3, C2);
	auto page2_c = makeCollection(L2, C4);
	auto display1_c = makeDisplay(page1_c, page2_c);
	auto display1_h = A_Top_UI(display1_c);

	C1.set_OnSelFn_TargetUI(display1_c.item(0));
	C2.set_OnSelFn_TargetUI(display1_c.item(0));
	C3.set_OnSelFn_TargetUI(display1_h);
	C4.set_OnSelFn_TargetUI(display1_h);

	display1_h.rec_select(); // Make page_1 recipient of commands, C1 is ActiveUI.
	display1_h.rec_left_right(1); // Move to C3
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_3 C2");
	display1_h.rec_select(); // Click C3 Makes page_2 recipient of commands, C4 is ActiveUI.
	//CHECK(test_stream(display1_h.stream(tb)) == "L2 C_4");
	display1_h.rec_select(); // Calls select on recipient's (i.e page-2) activeUI; i.e. Click on C4 on Page 2. Causes page to change
							// Because display1_h.activeUI() i.e. page1, was selected, selection needs to move to new page.
							// ie. Display's recipient obj needs to be changed.
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L2 C_3 C2"); // same as when last on this page
	display1_h.rec_select(); // Calls select on recipient's (i.e page-1) activeUI; i.e. Click on Cmd1 on Page 0. Causes focus to change
	//CHECK(test_stream(display1_h.stream(tb)) == "L2 C_4");
}

TEST_CASE("Page-element which is a view-all collection", "[Display]") {
	cout << "\n **** Page-element which is a view-all collection ****\n";
	LCD_Display_Buffer<10, 4> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2"), L3("L3");
	UI_Cmd C1("C1",0), C2("C2", 0);
	UI_Cmd C3("C3",0), C4("C4", 0);


	cout << " CmdGroup Coll of Object_Hndl\n";
	auto cmdGroup_c = makeCollection(L2, C1, C2, C3);
	cmdGroup_c.removeBehaviour(Behaviour::b_RecycleInList);
	cout << " page1_c Coll of Collection_Hndl\n";
	auto page1_c = makeCollection(L1, cmdGroup_c, L3, C4);
	cout << " display1_c Coll of Collection_Hndl\n";
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);

	display1_h.stream(tb);

	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C3        L3 C4");
	display1_h.rec_select(); // Make page_1 recipient of commands, cmdGroup_c is ActiveUI.
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C_1 C2  C3        L3 C4");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C_2  C3        L3 C4");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C_3        L3 C4");
	display1_h.rec_up_down(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C_3        L3 C4");
	display1_h.rec_up_down(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C_3        L3 C4");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C3        L3 C_4");
	display1_h.rec_up_down(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C3        L3 C_4");
	display1_h.rec_left_right(-1);
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C_3        L3 C4");
	display1_h.rec_left_right(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C_2  C3        L3 C4");
	display1_h.rec_left_right(-1); 
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C_1 C2  C3        L3 C4");
	display1_h.rec_left_right(-1); 
	CHECK(test_stream(display1_h.stream(tb)) == "L1        L2 C1 C2  C3        L3 C_4");
}

TEST_CASE("Page-element which is a view-one collection", "[Display]") {
	cout << "\n **** Page-element which is a view-one collection ****\n";
	LCD_Display_Buffer<10, 2> lcd;
	UI_DisplayBuffer tb(lcd);

	UI_Label L1("L1"), L2("L2"), L3("L3");
	UI_Cmd C1("C1", 0), C2("C2", 0);
	UI_Cmd C3("C3", 0), C4("C4", 0);


	cout << " CmdGroup Coll of Object_Hndl\n";
	auto cmdGroup_c = makeCollection(C1, C2, C3);
	cmdGroup_c.behaviour().make_viewOne();

	cout << " page1_c Coll of Collection_Hndl\n";
	auto page1_c = makeCollection(L1, cmdGroup_c, L3, C4);
	cout << " display1_c Coll of Collection_Hndl\n";
	auto display1_c = makeDisplay(page1_c);
	auto display1_h = A_Top_UI(display1_c);

	display1_h.stream(tb);
	// Recipient is display, activeUI is page.
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L3 C4");
	display1_h.rec_select(); // Make page_1 recipient of commands, cmdGroup_c is ActiveUI.
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L3 C4");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C1     L3 C_4");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L3 C4");
	display1_h.rec_up_down(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_2     L3 C4");
	display1_h.rec_up_down(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_3     L3 C4");
	display1_h.rec_left_right(1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C3     L3 C_4");
	display1_h.rec_up_down(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C3     L3 C_4");
	display1_h.rec_left_right(-1);
	display1_h.stream(tb);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_3     L3 C4");
	display1_h.rec_up_down(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_2     L3 C4");
	display1_h.rec_up_down(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_1     L3 C4");
	display1_h.rec_up_down(-1);
	CHECK(test_stream(display1_h.stream(tb)) == "L1 C_3     L3 C4");
}