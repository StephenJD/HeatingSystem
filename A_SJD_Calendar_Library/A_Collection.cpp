#include "debgSwitch.h"
#include ".\A_Collection.h"
#include "A_Displays_Data.h"

#include "A_Stream_Utilities.h"
#if defined (ZPSIM)
	#include <iostream>
#endif

using namespace Utils;

// ********************** Collection ******************
Collection::Collection()
	:active_index(0), count(0) {
}

char * Collection::stream(S1_ind list_index)const {
	return get_active_member().stream(list_index);
}

B_fld_isSel Collection::up_down_key(S1_inc move) {
	if (is_selected())
		return get_active_member().up_down_key(move);
	else
		return false;
}

B_to_next_fld Collection::left_right_key(S1_inc move){
	if (is_selected())
		return get_active_member().left_right_key(move);
	else
		return false;
}

S1_newPage Collection::select_key(){ // returns true if the field data goes into / out of edit mode
	if (is_selected())
		return get_active_member().select_key();
	else {
		if (get_active_member().validate_active_index(1)) {
			if (!change_to_selected()) return DO_EDIT; else return 0;
		} else return 0;
	}
}

B_fld_desel Collection::back_key(){ // // returns true if object is deselected
	B_fld_desel returnVal = false;
	if (is_selected()){
		returnVal = get_active_member().back_key(); // returns true if field taken out of edit mode
	}
	return returnVal;
}

S1_ind Collection::lastMembIndex() const {return count-1;}

Collection & Collection::get_active_member() const { // returns a reference to the active member
	S1_ind activeIndex = get_active_index();
	return get_member_at(activeIndex);
}

void Collection::make_member_active(S1_inc move){ // makes the next member active
	S1_byte activeIndex = nextIndex(0,active_index,lastMembIndex(),move);
	if (move >= 0){ // we need to pass validate +/- 1.
		move = 1;
	} else move = -1;
	Collection::set_active_index(activeIndex);
	validate_active_index(move);
}

B_selectable Collection::validate_active_index(S1_inc direction){// returns true if has set active_index to a selectable member
	S1_byte originalActiveIndex = active_index;
	S1_byte try_active_ind = originalActiveIndex;
	bool successful;
	do {
		Collection & try_active_memb = get_member_at(try_active_ind);
		successful = try_active_memb.validate_active_index(direction);
	} while (!successful && ((try_active_ind = nextIndex(0,try_active_ind,lastMembIndex(),direction))?true:true) && try_active_ind != originalActiveIndex);
	
	if (successful) 
		set_active_index(try_active_ind); 
	else  
		set_active_index(originalActiveIndex);
	return successful;
} 

B_fld_isSel Collection::change_to_selected(){ // a request to select a member which may not have selectable data
	D_Data & getData = displ_data();
	getData.hasCursor = true;
	return true; // base-class version always returns true.
}

void Collection::change_to_unselected(){
	displ_data().hasCursor = false;
}

B_fld_isSel Collection::is_selected() const {
	const D_Data & getData = displ_data();
	bool result = getData.hasCursor;
	return result; // displ_data().hasCursor;
}





