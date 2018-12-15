#include "DisplayBuffer.h"

namespace HardwareInterfaces {

	void DisplayBuffer_I::setCursor(int col, int row) {
		setCursorPos(col + row * cols());
		for (int i = strlen(buff()); i < cursorPos(); ++i) {
			buff()[i] = ' ';
		}
	}

	size_t DisplayBuffer_I::write(uint8_t character) {
		buff()[cursorPos()] = character;
		setCursorPos(cursorPos() + 1);
		return 1;
	}

	void DisplayBuffer_I::print(const char * str, uint32_t val) {
		print(str);
		print(val);
	}

	void DisplayBuffer_I::reset() {
		buff()[0] = 0;
		setCursorPos(0);
		setCursorMode(e_unselected);
	}

	void DisplayBuffer_I::truncate(int newEnd) {
		if (newEnd <= size()) buff()[newEnd] = 0;
	}


	//void iniPrint(char * msg) {
	//	if (lineNo >= MAX_NO_OF_ROWS) lineNo = 0;
	//	mainLCD->setCursor(0, lineNo++);
	//	uint8_t len = strlen(msg);
	//	mainLCD->print(msg);
	//	Serial.println(msg);
	//	for (uint8_t i = len; i<MAX_LINE_LENGTH; ++i)
	//		mainLCD->print(" ");
	//};

	//void iniPrint(char * msg, U4_byte val) {
	//	uint8_t len = strlen(msg);
	//	mainLCD->print(msg);
	//	Serial.print(msg);
	//	mainLCD->print(val);
	//	Serial.println(val);
	//};

	/*
	char * Page::stream(S1_Ind list_index) const { // list_index used by this function for Field_List::stream

	clear_pageBuffer();

	bool isAlist = false, showingFirstInList = true;
	bool getAnother;
	U1_Byte lastField = lastMembIndex(); // number of fields
	U1_Byte activeField = get_active_index();

	for (int i = 0; i <= lastField; i++) {// stream each field
	char * nextField;
	Collection & thisCollection = get_member_at(i);
	const Field_List & this_list = (Field_List &)thisCollection;
	if (this_list.modeIs(HIDDEN)) {
	nextField = thisCollection.stream(list_index) + 2;
	continue;
	}
	// thisCollection & this_list are the same object.
	S1_Ind membActInd = thisCollection.get_active_index(); // index of last item
	isAlist = (membActInd > NOT_A_LIST);
	if (isAlist) {
	if (i != activeField) { // deactivate list if exited a list by going back a page
	thisCollection.set_active_index(LIST_NOT_ACTIVE);
	membActInd = 0;
	}
	//if (membActInd == LIST_NOT_ACTIVE) membActInd = 0;
	if (this_list.first_list_index > membActInd) this_list.first_list_index = membActInd; // move back through the list
	list_index = this_list.first_list_index;
	if (this_list.first_list_index > 0)
	showingFirstInList = false;
	}
	do { // once for a non-list field, or loop for each member of a list
	bool hasCursor = (is_selected() && i == activeField);
	displ_data().thisIteminEdit = (displ_data().displayInEdit && hasCursor);
	nextField = thisCollection.stream(list_index);
	//if (nextField != 0) {
	#if defined (ZPSIM)
	char * debugChar = nextField + 2;
	S1_Ind debugAI = activeField;
	//bool debugIsSelected = is_selected();
	#endif
	if (nextField && !hasCursor)
	nextField[0] = 0; // field not selected. List item selection handled by Field_List::stream
	getAnother = add_to_pageBuffer(nextField, page_data().display->rows(), page_data().display->cols(), showingFirstInList); // returns true if is a list item & there is room
	if (getAnother) {
	this_list.last_list_index = list_index++;
	} // else membActInd = this_list.last_list_index; // else isAlist = false;
	//}
	} while (nextField && getAnother);
	while (isAlist && this_list.last_list_index < membActInd) { // move forward through the list
	this_list.first_list_index++;
	stream(0); // recursive call
	} // make it go round again
	}
	close_pageBuffer(displ_data().displayInEdit, page_data().display->rows(), page_data().display->cols());
	return get_pageBuffer();
	}
	*/
}


