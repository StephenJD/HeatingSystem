#include "debgSwitch.h"
#include ".\B_Pages.h"
#include "A_Stream_Utilities.h"
#include "D_EpDat_A.h"
#include "B_Fields.h"
#include "B_Displays.h"
#if defined (ZPSIM)
	#include <iostream>
	using namespace std;
#endif

using namespace Utils;

// This file contains the Pages classes defining all the pages
// A Pages class defines a single page by pointing to an array of field pointers
// Pages.active_index is the current field

// ********************* Pages *******************
FactoryObjects * Pages::f;

Pages::Pages(Displays * display, Group & ed_group, U1_ind defaultFld ) : p_data(display, ed_group, defaultFld) {
}

char * Pages::stream(S1_ind list_index) const { // list_index used by this function for Field_List::stream
	/* A Pages defines a single page.
		This method streams all the fields on a page.
	*/
	clear_pageBuffer();

	bool isAlist = false, showingFirstInList = true;
	bool getAnother;
	U1_byte lastField = lastMembIndex(); // number of fields
	U1_byte activeField = get_active_index();

	for (int i = 0; i<=lastField;i++){// stream each field
		char * nextField;
		Collection & thisCollection = get_member_at(i);
		const Field_List & this_list = (Field_List &) thisCollection;
		if (this_list.modeIs(HIDDEN)) {
			nextField = thisCollection.stream(list_index) + 2;
			continue;
		}
		// thisCollection & this_list are the same object.
		S1_ind membActInd = thisCollection.get_active_index(); // index of last item
		isAlist = (membActInd > NOT_A_LIST);
		if (isAlist){
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
			S1_ind debugAI = activeField;
			//bool debugIsSelected = is_selected();
		#endif
				if (nextField && !hasCursor)
					nextField[0] = 0; // field not selected. List item selection handled by Field_List::stream
				getAnother = add_to_pageBuffer(nextField,page_data().display->rows(),page_data().display->cols(),showingFirstInList); // returns true if is a list item & there is room
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
	close_pageBuffer(displ_data().displayInEdit,page_data().display->rows(),page_data().display->cols());
	return get_pageBuffer();
}

B_to_next_fld Pages::left_right_key(S1_inc move) { // always returns true
	B_to_next_fld returnVal;
	if (is_selected()){
		returnVal = get_active_member().left_right_key(move);
		if (!returnVal) {
			make_member_active(move); // move to next field
			returnVal = true;
		}
	} else { // Done -> on unselected page, so select the page.
		change_to_selected();
		returnVal =  true;
	}
	return returnVal;	
}

D_Data &		Pages::displ_data()	   {return page_data().display->displ_data();}
const D_Data &	Pages::displ_data() const {return page_data().display->displ_data();}
const P_Data &	Pages::page_data() const {return p_data;}
 
Fields_Base & Pages::getField(S1_ind index) { //"Pages::getField must not be called"
	long cludge = reinterpret_cast<long>(this);
	cludge = cludge/0; // Intentional to throw exception if called!
	Fields_Base * cludge2 = reinterpret_cast<Fields_Base *>(cludge);
	return reinterpret_cast<Fields_Base &> (*cludge2); 
}