#include ".\B_Displays.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "B_Pages.h"
#include "B_Fields.h"

using namespace Utils;

// This file contains the Displays classes defining all page collections
// A Displays defines a single Displays by pointing to an array of page pointers, each handling a single page
// Displays.active_index is the current page
FactoryObjects * Displays::f;

Displays::Displays(MultiCrystal * lcd, Keypad * keypad ) : _lcd(lcd),keypad(keypad), page(0), pageIndex(-1) {
	static bool OK = Factory::initialiseProgrammer(); // only executed first time round.
}

B_fld_isSel Displays::up_down_key(S1_inc move){
	B_fld_isSel returnVal = get_active_member().up_down_key(move);
	if (!returnVal) { // change page need to reverse move on page selection
		S1_ind myActiveIndex = get_active_index();
		if (myActiveIndex == 0) { // Version
			myActiveIndex = nextIndex(0,myActiveIndex,1,-move);
		} else if (myActiveIndex == 1) { // Setup
			myActiveIndex = nextIndex(0,myActiveIndex,2,-move);
		} else { // Time ... Zone Sequence
			myActiveIndex = nextIndex(mp_currDTime,myActiveIndex,mp_diary,-move);
		}
		set_page_default_field(myActiveIndex);
		make_member_active(0); // reset active member to 0 on each new page
	}
	return returnVal;
}

B_to_next_fld Displays::left_right_key(S1_inc move){
	B_to_next_fld returnVal = Collection::left_right_key(move);
	if (!returnVal && get_active_member().change_to_selected()) { // do not select a display on a page that cannot be selected
		 change_to_selected();
	}
	return returnVal;
}

S1_newPage Displays::select_key(){
	S1_newPage returnVal = Collection::select_key(); // returns > 0 for page change
	if (returnVal > 0) {
		newPage(returnVal);
	} else make_member_active(0); // validate old page since selection may change selectable fields

	return returnVal;
}

void Displays::newPage(S1_ind newPage) {
	d_data.backField[d_data.bpIndex] = get_active_member().get_active_index(); // current field index
	d_data.backList[d_data.bpIndex] = get_active_member().get_active_member().get_active_index(); // current list index
	d_data.backPage[d_data.bpIndex++] = get_active_index(); // current page index
	set_page_default_field(newPage);
	get_active_member().change_to_selected(); // select it
	make_member_active(0); // validate new page //returnVal-get_active_index()
}

B_fld_desel Displays::back_key(){ // returns true if object is deselected
	B_fld_desel returnVal = Collection::back_key();
	if (!returnVal) {
		if (d_data.bpIndex == 0) {
			get_active_member().change_to_unselected();
		} else {
			set_page_default_field(d_data.backPage[--d_data.bpIndex], false);
			make_member_active(0); // revalidate new page required to reset dspl_DT_pos
		}
	}
	return returnVal;
}

void Displays::set_page_default_field(S1_ind index, bool defaultField){
	set_active_index(index);
	Pages & actPage = (Pages &) get_active_member();
	if(defaultField) {
		const P_Data & pageData = actPage.page_data();
		S1_ind page = pageData.defaultField;
		actPage.set_active_index(page);
		d_data.dspl_DP_pos = 0;
		d_data.dspl_TT_pos = 0;
	} else {
		actPage.set_active_index(d_data.backField[d_data.bpIndex]);
		actPage.get_active_member().set_active_index(d_data.backList[d_data.bpIndex]);
	}

	if (&(actPage.page_data().ed_group) != &(f->zones))
		d_data.dspl_Parent_index = 0;
}

D_Data &		Displays::displ_data() {return d_data;}
const D_Data &	Displays::displ_data() const {return d_data;}

B_selectable Displays::validate_active_index(S1_inc direction){// All pages return true
	bool selectable = get_member_at(active_index).validate_active_index(direction);
	if (!selectable) change_to_unselected();
	return true;
}

U1_byte showError(int err, int place) {
	if (err) {
		logToSD("streamToLCD_Err :",err," at:",place);
	}
	return err;
}

U1_byte Displays::streamToLCD(bool showCursor) {
	char * gotPage = stream();
	U1_byte error = 0;
	error = _lcd->setCursor(0,0);
	showError(error,0);
	U1_ind row = 0;
	if (error) return error;

	for (char * pos = gotPage+1; * pos != 0;) {
		for (U1_byte col = 0; col < _lcd->_numcols; ++col){
			_lcd->print(*pos); // print() does not return an error code
			++pos;
		}
		error = error | showError(_lcd->setCursor(0,++row),2);
		if (error) return error;
	}
	
	if (showCursor && gotPage[0]!=0) { // show cursor
		U1_ind cursorPos = abs((S_char)gotPage[0])-1;
		U1_ind col = cursorPos % _lcd->_numcols; // 0-19
		row = cursorPos / _lcd->_numcols; // 0-3
		error = error | showError(_lcd->setCursor(col,row),3);
		error = error | showError(_lcd->cursor(),4);
		static bool blink = true;
		if ((S_char)gotPage[0] < 0) // in edit
			error = error | showError(_lcd->blink(),5);
		else {
			error = error | showError(_lcd->noBlink(),6);
			#if !defined (ZPSIM)
				if (!blink) error = error | showError(_lcd->noCursor(),7); // cause ordinary cursor to alternate on/off to advertise its position.
				blink = !blink;
			#endif
		} 
	} else error = error | showError(_lcd->noCursor(),8);
	return error;
}

Displays::~Displays() {
	delete 	_lcd;
	delete 	keypad;
	delete 	page;
}