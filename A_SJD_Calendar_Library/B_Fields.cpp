#include "debgSwitch.h"
#include ".\B_Fields.h"
#include "B_Displays.h"
#include "B_Pages.h"
#include "D_Group.h"
#include "Zone_Stream.h"
#include "DailyProg_Stream.h"
#include "D_Factory.h"
#include "TempSens_Run.h"
#include "Relay_Run.h"
#include "Mix_Valve.h"
#include "I2C_Helper.h"
#include "MixValve_Run.h"

//#include "Display.h"
#include "A_Stream_Utilities.h" 

#if defined (ZPSIM)
	#include <iostream>
	using namespace std;
#endif

using namespace Utils;
template <int noDevices> class I2C_Helper_Auto_Speed;
extern I2C_Helper_Auto_Speed<27> * i2C;
// A fields class defines a single field by pointing to an array of data members (string pointers)

// ********************* Fields_Base *******************
FactoryObjects * Fields_Base::f;

Fields_Base::Fields_Base (Pages * page, U1_byte fld_mode)
		: f_data(page)
		{set_noOf(0); // clear mode flags
		mode(fld_mode);
		set_active_index(NOT_A_LIST);// This is a marker to indicate the field is not a list
}

Group & Fields_Base::group () const {
	return page().page_data().ed_group;
}

D_Data & Fields_Base::displ_data() {return page().displ_data();}

const D_Data & Fields_Base::displ_data() const {return page().displ_data();}

TimeTemp_Stream & Fields_Base::ttStream() const {
	U1_ind firstTT = dpStream().getVal(E_firstTT);
	return f->timeTempS(firstTT + displ_data().dspl_TT_pos);
}

DataStream & Fields_Base::dStream() const {
	return group()[parent_index()].dStream();
}

Zone_Stream & Fields_Base::zStream() const {
	return f->zoneS(parent_index());
}

DailyProg_Stream & Fields_Base::dpStream() const {
	U1_ind thisDP = Zone_Stream::currZdtData.currDP + displ_data().dspl_DP_pos;
	return f->dailyProgS(thisDP);
}

S1_ind Fields_Base::parent_index() const {
	S1_ind parentIndex = displ_data().dspl_Parent_index;
	if (parentIndex > group().count()) {
		return 0;
	} else {
		return parentIndex;
	}
}

void Fields_Base::set_parent_index(S1_ind index) {
	if (index > group().count()) {
		index = 0;
	}
	displ_data().dspl_Parent_index = index;
	displ_data().dspl_DT_pos = 0;
	displ_data().dspl_DP_pos = 0;
	displ_data().dspl_TT_pos = 0;
}

bool Fields_Base::onUpDown(S1_inc move) {
	make_member_active(move); 
	return false;
}
	
B_fld_isSel Fields_Base::up_down_key(S1_inc move) // always returns true.
{	// either gets next member for the field, puts in edit-mode or performs edit.
	S1_ind myActiveIndex = get_active_index();
	Edit * debug = editPtr;
	if (!displ_data().displayInEdit) {
		if (modeIs(CHANGEABLE)) {
			bool enableEdit = edit_on_updn();
			if (!enableEdit) { // do up/down action when not in edit.
				enableEdit = onUpDown(-move);  // insert TT creates newTT and puts it in edit
				myActiveIndex = get_active_index();
				zStream().setZdata();
			}
			if (enableEdit && startEdit(myActiveIndex) == DO_EDIT) { // or active_index
				displ_data().displayInEdit = true;
				editPtr->nextVal(move);	
			}
		}
	} else {
		editPtr->nextVal(move);	
	}
	return true;
}

B_to_next_fld Fields_Base::left_right_key(S1_inc move) {// if we return false, moves to next field	
	B_to_next_fld returnVal = true;
	bool editNextField = false;
	bool displayInEdit = displ_data().displayInEdit;
	if (displayInEdit) {
		editNextField = editPtr->nextCursorPos(move); // move edit cursor
		if (editNextField) 
			save_Edit(get_active_index());
	}
	if (editNextField || !displayInEdit) { // move to next field or list item	
		returnVal = false;
		S1_ind myActiveIndex = get_active_index(); // 
		if (myActiveIndex == NOT_A_LIST) { // not in list	
			displ_data().displayInEdit = false;
		} else if (myActiveIndex == 0 && move < 0) { // move out of list	
			set_active_index(LIST_NOT_ACTIVE);
			displ_data().displayInEdit = false;
		} else { // move to next item in a list field
			S1_byte last = lastInList();
			bool enableEdit = edit_on_updn();
			myActiveIndex = nextIndex(0,myActiveIndex,last,move);
			set_active_index(myActiveIndex);
			if (myActiveIndex == 0 && move > 0) {// move out of list
				set_active_index(LIST_NOT_ACTIVE);
				displ_data().displayInEdit = false;
			} else { // we are still in the list
				if (!enableEdit) {
					edit_on_updn(); // perform optional task
				}
				if (editNextField){
					editNextField = (startEdit(myActiveIndex) == DO_EDIT);
					displ_data().displayInEdit = editNextField;
				}
				returnVal = true;
			}
		}
	}
	return returnVal;
}

S1_newPage Fields_Base::select_key(){
	S1_newPage returnVal; 
	if (displ_data().displayInEdit){
		displ_data().displayInEdit = false;
		returnVal = save_Edit(get_active_index()); // DO_EDIT or 0 says don't move to new page
	} else {
		S1_ind debug = page().get_active_index();
		returnVal = onSelect(get_active_index()); // CMD fields change the active field
		// we want to check modeIs(EDITABLE) on active field, not necessarily this field.
		//Fields_Base & activeField = page().getField(page().get_active_index()); // can't do this unless every page class implements getField.
		//displ_data().displayInEdit = (returnVal == DO_EDIT && activeField.modeIs(EDITABLE));
		// because we can't do this, for cmds wanting to edit the field they make active, the cmd field must make itself editable.
		displ_data().displayInEdit = (returnVal == DO_EDIT && modeIs(EDITABLE));
	}
	return returnVal;
}

B_fld_desel Fields_Base::back_key(){
	B_fld_desel returnVal = false;
	if (displ_data().displayInEdit) {
		cancelEdit(get_active_index());
		displ_data().displayInEdit = false;
		returnVal = true;
	}
	return returnVal;
}

Collection & Fields_Base::get_member_at (S1_ind index) const {
	// TODO: cannot return a collection, since fields have eddata instead of collections.
	// this method is never called.
	return *(Collection *) (void *)this;
}

void Fields_Base::make_member_active(S1_inc move){ // makes the next list or text member active
	S1_ind activeIndex = parent_index();
	U1_count lastInd = lastMembIndex();
	S1_byte startMember = 0;
	activeIndex = nextIndex(startMember,activeIndex,lastInd,move);
	set_parent_index(activeIndex);
}

B_selectable Fields_Base::validate_active_index (S1_inc direction){
	return (modeIs(SELECTABLE));
} // returns true if can be made active

bool Fields_Base::inEdit() const {
	return displ_data().thisIteminEdit;
}

S1_byte Fields_Base::lastInList() const {
	DataStream & thisStream = dStream();
	EpData & thisData = thisStream.epD();
	S1_byte count = thisData.getNoOfItems();
	return count - 1;
}

S1_newPage Fields_Base::save_Edit(S1_ind item) {
	editPtr->exitEdit();
	return saveEdit(item);
}

S1_newPage Fields_Base::saveEdit(S1_ind item) {
	dStream().save();
	return 0;
}

void Fields_Base::cancelEdit(S1_ind item) const {editPtr->cancelEdit();}

U1_byte Fields_Base::mode() const {return Collection::lastMembIndex()+1;}

bool Fields_Base::modeIs(U1_byte tryMode) const { // modeIs() arguemnt must not be an inverted value
	U1_byte myMode = mode();
	if (tryMode == HIDDEN) return myMode & HIDDEN;
	else return !(myMode & HIDDEN) && (myMode >= tryMode);
}

void Fields_Base::mode(U1_byte newMode) { // just toggle bits
	if (newMode > 128) // not-mode
		set_noOf(mode() & newMode); // clear flag
	else set_noOf(mode() | newMode); // set flag
}

char Fields_Base::lineMode() const {return (mode() & NEW_LINE) ? NEW_LINE_CHR : 0;}

// ********************* Abstract Field_Cmd *******************
Field_Cmd::Command Field_Cmd::cmd = C_none;

Field_Cmd::Field_Cmd(Pages * page, const char * label_text, U1_byte mode)
	: Fields_Base(page, mode), text(label_text) {
	//text = new char[strlen(label_text)+1];
	//strcpy(text,label_text);
}

Field_Cmd::~Field_Cmd() {
	//delete[] text;
}

char * Field_Cmd::stream(S1_ind list_index) const {
	char * buff = Format::streamBuffer();
	strcpy(buff+2,text);
	buff[0] = 1;
	buff[1] = lineMode();
	return buff;
}

// ********************* Concrete Fld_Label *******************

Fld_Label::Fld_Label(Pages * page, const char * label_text, U1_byte fldMode)
	: Field_Cmd(page,label_text, fldMode) {}

// ********************* Concrete Fld_Name *******************
Fld_Name::Fld_Name(Pages * page, U1_byte mode)
	: Fields_Base(page, mode) {}

S1_ind Fld_Name::lastMembIndex() const { // number of members in a group
	return group().last();
}

char * Fld_Name::stream(S1_ind) const {
	return dStream().nameFldStr(inEdit(),lineMode());
}

S1_newPage Fld_Name::onSelect(S1_byte) {
	return dStream().onNameSelect();
}

S1_newPage Fld_Name::saveEdit(S1_ind item){ 
	dStream().saveName();
	return 0;
}

// ********************* Abstract Field_List *******************
Field_List::Field_List(Pages * page, U1_byte mode) 
	:Fields_Base(page, mode),
	first_list_index(0), 
	last_list_index(0)
	{set_active_index(LIST_NOT_ACTIVE);} // overide the base constructor which sets NOT_A_LIST to indicate not a list

char * Field_List::stream(S1_ind list_index) const { // list_index specifies item to return
	S1_byte last = lastInList();
	char * list_data(0);
	if (list_index <= last) {
		S1_ind active_index = get_active_index();
		bool hasCursor = (list_index == active_index);
		displ_data().thisIteminEdit = (displ_data().displayInEdit && hasCursor);
		list_data = listStream(list_index);
		if (!hasCursor){ // Clear cursor on non-active list members.
			list_data[0] = 0;
		}
		if (list_index == first_list_index) {
			list_data[1] = lineMode();
		}
		strcat(list_data+2,"\t");
	}
	return list_data;
}

B_selectable Field_List::validate_active_index(S1_inc direction) {// returns true if can be made active
	if (!modeIs(SELECTABLE)) 
		return false;
	else {
		S1_inc activeIndex = get_active_index();
		if (activeIndex == LIST_NOT_ACTIVE){
			activeIndex = 0;
			set_active_index(0);
		}	
		if (direction < 0 && activeIndex == 0) {
			set_active_index(lastInList());
		}
		return true;
	}
}

S1_newPage Field_List::startEdit(S1_byte myListPos) {
	return dStream().onItemSelect(myListPos,displ_data());
}

S1_newPage Field_List::saveEdit(S1_ind item) {
	dStream().saveItem(item);
	return 0;
}

// ********************* Concrete Fld_I2C_Addr *******************

Fld_I2C_Addr::Fld_I2C_Addr(Pages * page, U1_byte mode) : Fields_Base(page, mode), addrIndex(0), regIndex(0), relayOn(0) {}

char *	Fld_I2C_Addr::stream (S1_ind list_index) const {
	char * buff = Format::streamBuffer();
	int dInd = addrIndex - Factory::noOf(noTempSensors) + 1; // not interrested in display(0)
	U1_byte data[2];
	U1_byte result;
	if (addrIndex < Factory::noOf(noTempSensors)) {
		int addr = f->tempSensorR(addrIndex).getVal(0);
		strcpy(buff+2,"T ");
		strcat(buff+2,cIntToStr(addr,3,'0'));
		strcat(buff+2," ");
		strcat(buff+2, f->tempSensors[addrIndex].epD().getName());
		strcat(buff+2," ");
		result = f->tempSensorR(addrIndex).getSensTemp();
		if (temp_sense_hasError) strcat(buff+2,"Failed");
		else strcat(buff+2,cIntToStr(result,2));
	} else if (dInd  < Factory::noOf(noDisplays)) { // Displays
		strcpy(buff+2,"D ");
		strcat(buff+2, f->displays[dInd].epD().getName());
		strcat(buff+2," ");
		result = i2C->read(0x23 + dInd,0,1,data);
		if (result) strcat(buff+2,"Failed");
		else strcat(buff+2,cIntToStr(data[0],3));
	} else if (dInd  == Factory::noOf(noDisplays)){ // relays
		strcpy(buff+2,"R ");
		strcat(buff+2,f->relays[RELAY_ORDER[regIndex]].epD().getName());
		strcat(buff+2," ");
		if (i2C->notExists(0x20)) strcat(buff+2,"Failed");
		else {
			strcat(buff+2,"OK");
			f->relayR(RELAY_ORDER[regIndex]).setRelay(relayOn);
			if (relayOn) strcat(buff+2," ON"); else strcat(buff+2," OFF");
		}
	} else { // mix valve
		U1_byte mixIndex = dInd - Factory::noOf(noDisplays) - 1;
		strcpy(buff+2,"M ");
		strcat(buff+2,f->mixingValves[mixIndex].epD().getName());
		if (i2C->notExists(0x10)) strcat(buff+2," Failed");
		else {
			switch (regIndex) {
			case Mix_Valve::status : strcat(buff+2," Status "); break;
			case Mix_Valve::mode : strcat(buff+2," Mode "); break;
			case Mix_Valve::count : strcat(buff+2," Count "); break;
			case Mix_Valve::valve_pos : strcat(buff+2," Valve_pos "); break;
			case Mix_Valve::state : strcat(buff+2," State "); break;
			case Mix_Valve::flow_temp : strcat(buff+2," Flow_temp "); break;
			case Mix_Valve::request_temp : strcat(buff+2," Req_temp "); break;
			case Mix_Valve::ratio : strcat(buff+2," Ratio "); break;
			case Mix_Valve::control : strcat(buff+2," Control "); break;
			case Mix_Valve::temp_i2c_addr : strcat(buff+2," Temp_addr "); break;
			case Mix_Valve::max_ontime : strcat(buff+2," Max_time "); break;
			case Mix_Valve::wait_time : strcat(buff+2," Wait_time "); break;
			case Mix_Valve::max_flow_temp : strcat(buff+2," Max_flow "); break;
			}
			result = f->mixValveR(mixIndex).readFromValve(Mix_Valve::Registers(regIndex));
			strcat(buff+2,cIntToStr(result,3));
		}
	}
	
	buff[0] = 1;
	buff[1] = 0;
	return buff;
}

B_fld_isSel	Fld_I2C_Addr::up_down_key (S1_inc move) { 
	addrIndex = nextIndex(0,addrIndex,lastMembIndex(),move);
	return true;
}

B_to_next_fld Fld_I2C_Addr::left_right_key (S1_inc move) {
	if (addrIndex == Factory::noOf(noDisplays) + Factory::noOf(noTempSensors)-1) // relays
		regIndex = nextIndex(0,regIndex,6,move);
	else if (addrIndex >= Factory::noOf(noDisplays) + Factory::noOf(noTempSensors) ) { // mix valves
		regIndex = nextIndex(0,regIndex,13,move);
		if (regIndex == Mix_Valve::valve_pos + 1) regIndex = nextIndex(0,regIndex,13,move);
	}
	return true;
}

S1_newPage Fld_I2C_Addr::select_key () {
	if (addrIndex == Factory::noOf(noDisplays) + Factory::noOf(noTempSensors) -1) { // relays
		relayOn = !relayOn;
		f->relayR(RELAY_ORDER[regIndex]).setRelay(relayOn);
		Relay_Run::refreshRelays();
	}
	return 0;
}

S1_ind Fld_I2C_Addr::lastMembIndex() const {// number of members in a group
	return Factory::noOf(noDisplays) + Factory::noOf(noTempSensors) + 1;
}