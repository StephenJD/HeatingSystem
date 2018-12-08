#include "debgSwitch.h"
#include "C_Fields.h"
#include "A_Stream_Utilities.h"
#include "D_Group.h"
#include "DailyProg_Stream.h"
#include "D_Factory.h"
#include "Zone_Stream.h"
#include "Zone_Run.h"
//#include "D_Factory.h"
#include "D_DataStream.h"
#include "TimeTemp_Stream.h"
#include "C_Pages.h"

#if defined DEBUG_INFO
	#include <iostream>
	#include <string>
	using namespace std;
#endif

using namespace Utils;

// ********************* Concrete Fld_Name Classes *******************

// ********************* Fld_Cmd_Insert *******************

Fld_Cmd_Insert::Fld_Cmd_Insert(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_Insert::onSelect(S1_byte) {
	page().set_active_index(E_prog);

#if defined DEBUG_INFO	
	debugDTs(0,"Pre-Insert");
	cout << debugDPs("Pre-Insert");
	cout << "FromDate: " << DateTime_Stream::getTimeDateStr(Zone_Stream::currZdtData.fromDate) << " Yr:" << (int)Zone_Stream::currZdtData.fromDate.getYr() << '\n';
#endif
	zStream().insertNewDT(-1);

#if defined DEBUG_INFO
	debugDTs(0,"Post-Insert");
	cout << debugDPs("Post-Insert");
#endif

	U1_ind currDisplPos = displ_data().dspl_DT_pos;
	Zone_Stream::refreshDTsequences(); 
	Zone_Stream::currZdtData.firstDT = 255; // force reload
	zStream().setZdata(currDisplPos,1);
	zStream().setZdata(currDisplPos,1); // repeat to get currDisplPos set

#if defined DEBUG_INFO
	debugDTs(0,"After-Insert");
	debugDPPos("After Insert");
#endif

	return 0;
}

Fld_Cmd_Delete::Fld_Cmd_Delete(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_Delete::onSelect(S1_byte) {
	// modify field modes on this page
	page().getField(E_edit).mode(HIDDEN);
	page().getField(E_ins).mode(HIDDEN);	
	page().getField(E_del).mode(HIDDEN);
	page().getField(E_prgPge).mode(HIDDEN);
	page().getField(E_commit).mode(~HIDDEN);	
	page().set_active_index(E_commit);
	command(C_delete);
	return 0;
}

Fld_Cmd_Prog::Fld_Cmd_Prog(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_Prog::onSelect(S1_byte) {
	return mp_prog;
}

Fld_Cmd_Commit::Fld_Cmd_Commit(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_Commit::onSelect(S1_byte) {
	// modify field modes on this page
	page().getField(E_edit).mode(~HIDDEN);
	page().getField(E_ins).mode(~HIDDEN);	
	page().getField(E_del).mode(~HIDDEN);
	page().getField(E_prgPge).mode(~HIDDEN);
	page().getField(E_commit).mode(HIDDEN);	
	page().set_active_index(E_ins);
#if defined DEBUG_INFO
	cout << "Deleting Diary Entry\n";
#endif
	zStream().deleteDT();
	if (displ_data().dspl_DT_pos > 0) --displ_data().dspl_DT_pos;
	U1_ind currDisplPos = displ_data().dspl_DT_pos;
	Zone_Stream::refreshDTsequences(); 
	Zone_Stream::currZdtData.firstDT = 255; // force reload
	zStream().setZdata(currDisplPos,1);
	zStream().setZdata(currDisplPos,1); // repeat to get currDisplPos set

	zStream().commit();
	command(C_none);
	return 0;
}

Fld_Cmd_wk::Fld_Cmd_wk(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_wk::onSelect(S1_byte) {
	// modify field modes on this page
	page().getField(E_Cmd_DO).mode(HIDDEN);	
	page().getField(E_Cmd_IO).mode(HIDDEN);
	page().getField(E_pprog).mode(EDITABLE);
	// for cmds wanting to edit the field they make active, the cmd field must make itself editable.
	page().getField(E_Cmd_Wk).mode(EDITABLE);
	
	page().set_active_index(E_pprog);
	//zStream().setZdata();
	command(C_weekly);
	page().getField(E_pprog).onSelect(0);
	return DO_EDIT;
}

Fld_Cmd_do::Fld_Cmd_do(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_do::onSelect(S1_byte) {
	// modify field modes on this page
	page().getField(E_Cmd_Wk).mode(HIDDEN);
	page().getField(E_Cmd_IO).mode(HIDDEN);
	page().getField(E_pprog).mode(EDITABLE);
	// for cmds wanting to edit the field they make active, the cmd field must make itself editable.
	page().getField(E_Cmd_DO).mode(EDITABLE);

	page().set_active_index(E_pprog);
	//zStream().setZdata();
	command(C_dayOff);
	page().getField(E_pprog).onSelect(0);
	return DO_EDIT;
}

Fld_Cmd_io::Fld_Cmd_io(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_io::onSelect(S1_byte) {
	// modify field modes on this page
	page().getField(E_Cmd_Wk).mode(HIDDEN);
	page().getField(E_Cmd_DO).mode(HIDDEN);	
	page().getField(E_pprog).mode(EDITABLE);
	// for cmds wanting to edit the field they make active, the cmd field must make itself editable.
	page().getField(E_Cmd_IO).mode(EDITABLE);

	page().set_active_index(E_pprog);
	//zStream().setZdata();
	command(C_inout);
	page().getField(E_pprog).onSelect(0);
	return DO_EDIT;
}

Fld_Cmd_Hr::Fld_Cmd_Hr(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_Hr::onSelect(S1_byte) {
	for (int i=0;i<60;i++) currTime().testAdd1Min();;
	return 0;
}

Fld_Cmd_10m::Fld_Cmd_10m(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_10m::onSelect(S1_byte) {
	for (int i=0;i<10;i++) currTime().testAdd1Min();;
	return 0;
}

Fld_Cmd_Min::Fld_Cmd_Min(Pages * page, const char * label_text, U1_byte mode): Field_Cmd(page,label_text, mode) {}

S1_newPage Fld_Cmd_Min::onSelect(S1_byte) {
	currTime().testAdd1Min();
	return 0;
}

// ********************* Fld_Name types *******************

// Fld_EdZname  *******************

Fld_EdZname::Fld_EdZname(Pages * page, U1_byte mode) : Fld_Name(page, mode){}

S1_newPage Fld_EdZname::onSelect(S1_byte) {
	return zStream().nameEdit();
}

// Fld_ZAbbrev *******************

Fld_ZAbbrev::Fld_ZAbbrev(Pages * page, U1_byte mode) : Fld_Name(page, mode){}

char * Fld_ZAbbrev::stream(S1_ind) const {
	return zStream().abbrevFldStr(inEdit(),lineMode());	
}

// Fld_ProgName *******************

Fld_ProgName::Fld_ProgName(Pages * page, U1_byte mode) : Fld_Name(page, mode), pageMode(E_dpWeekly){}

char * Fld_ProgName::stream(S1_ind) const {
	setFields();
	return dpStream().nameFldStr(inEdit(),lineMode());	
}

S1_newPage Fld_ProgName::onSelect(S1_byte) {
	U1_byte nameWidth = dpStream().epD().getNameSize();
	if (dpStream().getDPtype() == E_dpNew) {
		if (!modeIs(EDITABLE)) {
			page().set_active_index(E_Cmd_Wk);
		}
		editChar.setRange(ValRange(nameWidth),"");
	} else
		editChar.setRange(ValRange(nameWidth),dpStream().getName());
	return DO_EDIT;
}

S1_newPage Fld_ProgName::saveEdit(S1_ind item){
	dpStream().saveName(parent_index());

#if defined DEBUG_INFO
	cout << "Post-SaveDPName DisplayPos: " << (int)displ_data().dspl_DT_pos << '\n';
	cout << "Curr DP:" << (int)Zone_Stream::currZdtData.currDP << '\n';
	cout << "Curr DT:" << (int)Zone_Stream::currZdtData.currDT << '\n';
	cout << "Curr DT:DPindex:" << (int)f->dateTimesS(Zone_Stream::currZdtData.currDT).dpIndex() << '\n';
	cout << debugDPs("Saved Name");
	debugDPPos("Saved Name");
#endif

	pageMode = E_dpRefresh;
	return 0;
}

B_fld_desel	Fld_ProgName::back_key () { // returns true if object is deselected
	Field_Cmd::command(Field_Cmd::C_none);
	pageMode = E_dpRefresh;
	if (Fields_Base::back_key())
		return true;
	else {
		if (Zone_Stream::currZdtData.currDPtype == E_dpNew) {
			f->dateTimesS(Zone_Stream::currZdtData.currDT).setDPindex(Zone_Stream::currZdtData.prevDP);
		}
		return false;
	}
}

void Fld_ProgName::setFields() const {
	U1_ind currDP = dpStream().index();
	// E_dpNew = 0, E_dpRefresh = 1, E_dpWeekly = 64
	E_dpTypes newPageMode = dpStream().getDPtype();
	if (pageMode != newPageMode) {
		pageMode = newPageMode;
		page().getField(E_pprog).mode(EDITABLE);
		page().getField(E_dpTypeLbl).mode(HIDDEN);
		page().getField(E_Cmd_Wk).mode(SELECTABLE|HIDDEN);
		page().getField(E_Cmd_DO).mode(SELECTABLE|HIDDEN);
		page().getField(E_Cmd_IO).mode(SELECTABLE|HIDDEN);
		page().getField(E_TTlbl).mode(HIDDEN);
		page().getField(E_TTs).mode(EDITABLE|HIDDEN);
		page().getField(E_DOlbl).mode(HIDDEN);
		page().getField(E_DO).mode(EDITABLE|HIDDEN);
		page().getField(E_daysLbl).mode(HIDDEN);
		page().getField(E_pdays).mode(EDITABLE|HIDDEN);
		switch (pageMode) {
		case E_dpWeekly:
			page().getField(E_daysLbl).mode(~HIDDEN);
			page().getField(E_pdays).mode(~HIDDEN);
			page().getField(E_TTlbl).mode(~HIDDEN);
			page().getField(E_TTs).mode(~HIDDEN);
			//page().set_active_index(E_pdays);
			break;
		case E_dpDayOff:
			page().getField(E_DOlbl).mode(~HIDDEN);
			page().getField(E_DO).mode(~HIDDEN);
			//page().set_active_index(E_DO);
			break;
		case E_dpInOut:
			//page().set_active_index(E_pprog);
			break;
		case E_dpNew:
			page().getField(E_pprog).mode(~EDITABLE);
			page().getField(E_pprog).mode(CHANGEABLE);
			page().getField(E_dpTypeLbl).mode(~HIDDEN);
			page().getField(E_Cmd_Wk).mode(~HIDDEN);
			page().getField(E_Cmd_DO).mode(~HIDDEN);
			page().getField(E_Cmd_IO).mode(~HIDDEN);
			//page().set_active_index(E_Cmd_Wk);
			break;
		default:
			;
		}
	}
}

void Fld_ProgName::make_member_active(S1_inc move){
	U1_ind dpIndex = zStream().nextDP(Zone_Stream::currZdtData.currDP + displ_data().dspl_DP_pos, move);
	//U1_ind dpIndex = zStream().nextDP(Zone_Stream::currZdtData.currDP, displ_data().dspl_DP_pos);
	//dpIndex = zStream().nextDP(dpIndex, move);
	displ_data().dspl_DP_pos = dpIndex - Zone_Stream::currZdtData.currDP;
}

// ********************* Field_Base types *******************
Fld_EdZAbbrev::Fld_EdZAbbrev(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_EdZAbbrev::stream(S1_ind) const {
	return zStream().abbrevFldStr(inEdit(),lineMode());
		
}

S1_newPage Fld_EdZAbbrev::startEdit(S1_ind index) {
	return zStream().abbrevSelect();
}

S1_newPage Fld_EdZAbbrev::saveEdit(S1_ind index) {
	zStream().abbrevSave();
	return 0;
}

// *********************
Fld_ZOffset::Fld_ZOffset(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_ZOffset::stream(S1_ind) const {
	return zStream().offsetFldStr(inEdit(),lineMode());
}

S1_newPage Fld_ZOffset::startEdit(S1_ind index) {
	return zStream().offsetSelect();
}

S1_newPage Fld_ZOffset::saveEdit(S1_ind index) {
	zStream().offsetSave();
	return 0;
}

// *********************
Fld_CurrDate::Fld_CurrDate(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_CurrDate::stream(S1_ind) const {
	return currTime().dateFldStr(inEdit());
}

S1_newPage Fld_CurrDate::startEdit(S1_ind index) {
	return currTime().dateSelect();
}

S1_newPage Fld_CurrDate::saveEdit(S1_ind index) {
	currTime().dateSave();
	return 0;
}

// *********************
Fld_CurrTime::Fld_CurrTime(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_CurrTime::stream(S1_ind) const {
	return currTime().timeFldStr(inEdit());
}

S1_newPage Fld_CurrTime::startEdit(S1_ind index) {
	return currTime().timeSelect();
}

S1_newPage Fld_CurrTime::saveEdit(S1_ind index) {
	currTime().timeSave();
	return 0;
}

// *********************
Fld_DSTmode::Fld_DSTmode(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_DSTmode::stream(S1_ind) const {
	return currTime().dstFldStr(inEdit());
}

S1_newPage Fld_DSTmode::startEdit(S1_ind index) {
	return currTime().dstSelect();
}

S1_newPage Fld_DSTmode::saveEdit(S1_ind index) {
	currTime().dstSave();
	return 0;
}

// *********************
Fld_RemoteTemp::Fld_RemoteTemp(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_RemoteTemp::stream(S1_ind) const {
	return zStream().remoteFldStr(inEdit(),lineMode());
}

S1_newPage Fld_RemoteTemp::startEdit(S1_ind index) {
	return zStream().offsetSelect();
}

S1_newPage Fld_RemoteTemp::saveEdit(S1_ind index) {
	zStream().offsetSave();
	return 0;
}

// *********************
Fld_Zfrom::Fld_Zfrom(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_Zfrom::stream(S1_ind item) const {
	return zStream().dtFromFldStr(displ_data().dspl_DT_pos,inEdit(),lineMode());
}
	
B_fld_isSel	Fld_Zfrom::up_down_key (S1_inc move) {
	displ_data().dspl_DT_pos = nextIndex(0,displ_data().dspl_DT_pos,zStream().lastDT(),-move);
	displ_data().dspl_DP_pos = 0;
	zStream().setZdata(displ_data().dspl_DT_pos, move);
	return true;
}

S1_newPage Fld_Zfrom::onSelect(S1_byte) {
	return mp_edit_diary;
}

// *********************
Fld_ZEdfrom::Fld_ZEdfrom(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_ZEdfrom::stream(S1_ind item) const {
	return zStream().dtFromFldStr(displ_data().dspl_DT_pos, inEdit(),lineMode());
}

B_fld_isSel	Fld_ZEdfrom::up_down_key (S1_inc move) {
	if (displ_data().displayInEdit) {
		return Fields_Base::up_down_key(move);
	} else {
		displ_data().dspl_DT_pos = nextIndex(0,displ_data().dspl_DT_pos,zStream().lastDT(),-move);
		displ_data().dspl_DP_pos = 0;
		zStream().setZdata(displ_data().dspl_DT_pos, move);
		return true;
	}
}

S1_newPage Fld_ZEdfrom::startEdit(S1_ind index) {
	Field_Cmd::command(Field_Cmd::C_edit);
	return zStream().fromSelect();
}

S1_newPage Fld_ZEdfrom::saveEdit(S1_ind index) {
	zStream().fromSave();
	Field_Cmd::command(Field_Cmd::C_none);
	return 0;
}

// *********************
Fld_Zto::Fld_Zto(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_Zto::stream(S1_ind item) const {
	return zStream().dtToFldStr(displ_data().dspl_DT_pos, inEdit(),lineMode());
}

B_fld_isSel	Fld_Zto::up_down_key (S1_inc move) {
	if (displ_data().displayInEdit) {
		return Fields_Base::up_down_key(move);
	} else {
		displ_data().dspl_DT_pos = nextIndex(0,displ_data().dspl_DT_pos,zStream().lastDT(),-move);
		displ_data().dspl_DP_pos = 0;
		zStream().setZdata(displ_data().dspl_DT_pos, move);
		return true;
	}
}

S1_newPage Fld_Zto::startEdit(S1_ind index) {
	Field_Cmd::command(Field_Cmd::C_edit);
	S1_newPage retVal = zStream().toSelect();
	return retVal;
}

S1_newPage Fld_Zto::saveEdit(S1_ind index) {
	zStream().toSave();
	Field_Cmd::command(Field_Cmd::C_none);
	return 0;
}

// *********************
Fld_Zprog::Fld_Zprog(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_Zprog::stream(S1_ind) const {
	return zStream().progFldStr(displ_data().dspl_DP_pos,inEdit(),lineMode());
}

S1_newPage Fld_Zprog::onSelect(S1_byte) {
	if (Field_Cmd::command() == Field_Cmd::C_none)
		return mp_prog; // new prog page
	else
		return zStream().progEdit();
}

S1_newPage Fld_Zprog::startEdit(S1_ind index) {
	Field_Cmd::command(Field_Cmd::C_edit);
	return zStream().progEdit();
}

S1_newPage Fld_Zprog::saveEdit(S1_ind index) {
	Field_Cmd::command(Field_Cmd::C_none);
	return zStream().progSave();
}

 //*********************
Fld_DPdays::Fld_DPdays(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_DPdays::stream(S1_ind) const {
	return dpStream().daysToFullStr(inEdit(),lineMode());
}

void Fld_DPdays::make_member_active(S1_inc move){ // cycle round DP's of same name, but different day selections
	U1_ind dpIndex = Zone_Stream::currZdtData.currDP + displ_data().dspl_DP_pos;
	dpIndex = zStream().sameDP(dpIndex,move);
	displ_data().dspl_DP_pos = dpIndex - Zone_Stream::currZdtData.currDP;
}

S1_newPage Fld_DPdays::startEdit(S1_ind index) {
	return dpStream().daysSelect();
}

S1_newPage Fld_DPdays::saveEdit(S1_ind index) {
	dpStream().daysSave();
	S1_inc movePos = DP_EpMan::removeDuplicateDays(zStream().index(), displ_data().dspl_DP_pos + Zone_Stream::currZdtData.currDP);
	// Where we have inserted a new DP in a weekly group, we need to reset the start of the group
	U1_ind startGroup = f->dailyProgS(Zone_Stream::currZdtData.currDP).getFirstDPinGroup();
	f->dateTimesS(Zone_Stream::currZdtData.currDT).moveDPindex(startGroup);
	displ_data().dspl_DP_pos = displ_data().dspl_DP_pos + movePos;
#if defined DEBUG_INFO
	cout << "SaveDays-DisplPos:" << (int)displ_data().dspl_DP_pos << "Move:" << (int)movePos << '\n';
	cout << "CurrDT:" << (int)Zone_Stream::currZdtData.currDT << " DTIndex:" << (int)f->dateTimesS(Zone_Stream::currZdtData.currDT).dpIndex() << '\n';
#endif
	return 0;
}

// *********************
Fld_DPdayOff::Fld_DPdayOff(Pages * page, U1_byte mode) : Fields_Base(page, mode){}

char * Fld_DPdayOff::stream(S1_ind) const {
	return dpStream().dpDayOffStr(inEdit(),lineMode());
}

S1_newPage Fld_DPdayOff::startEdit(S1_ind index) {
	return dpStream().dayOffEdit();
}

S1_newPage Fld_DPdayOff::saveEdit(S1_ind index) {
	dpStream().dayOffSave();
	return 0;
}

// ********************* Field_List types *******************

Fld_L_Items::Fld_L_Items(Pages * page, U1_byte mode) : Field_List(page, mode) {}

char * Fld_L_Items::listStream(S1_ind item) const {return dStream().itemFldStr(item, inEdit());}

S1_newPage Fld_L_Items::startEdit(S1_ind index) {
	return dStream().onItemSelect(index, displ_data());
}

// *********************
Fld_L_Objects::Fld_L_Objects(Pages * page, U1_byte mode) : Field_List(page, mode) {}

char * Fld_L_Objects::listStream(S1_ind index) const { // item from list of objects
	DataStream & gotDstream = group()[index].dStream();
	S1_ind gotActInd = get_active_index();
	return gotDstream.objectFldStr(gotActInd, inEdit());
}

S1_byte Fld_L_Objects::lastInList() const {return group().last();}

bool Fld_L_Objects::edit_on_updn() const {
	return dStream().editOnUpDown(); // call required only for temp sensors
}

S1_newPage Fld_L_Objects::onSelect(S1_ind index) {
	displ_data().dspl_Parent_index = index;
	DataStream & gotDstream = group()[index].dStream();
	return gotDstream.onSelect(displ_data());
}

S1_newPage Fld_L_Objects::startEdit(S1_ind index) {
	DataStream & gotDstream = group()[index].dStream();
	return gotDstream.startEdit(displ_data());
}

S1_newPage Fld_L_Objects::saveEdit(S1_ind item){
	DataStream & gotDstream = group()[item].dStream();
	gotDstream.save();
	return 0;
}

// *********************
Fld_L_ZUsr::Fld_L_ZUsr(Pages * page, U1_byte mode) : Field_List(page, mode) {}

char * Fld_L_ZUsr::listStream(S1_ind item) const {return zStream().usrFldStr(item,inEdit());}

S1_byte Fld_L_ZUsr::lastInList() const {return zStream().noOfUsrSets()-1;}

S1_newPage Fld_L_ZUsr::startEdit(S1_ind index) {
	return zStream().onUsrSelect(index, displ_data());
}

S1_newPage Fld_L_ZUsr::saveEdit(S1_ind item){
	zStream().saveUsr(item);
	return 0;
}

// *********************
Fld_TTlist::Fld_TTlist(Pages * page, U1_byte mode) : Field_List(page, mode) {}
	
void Fld_TTlist::set_active_index (S1_ind index) {
	Collection::set_active_index(index);
	displ_data().dspl_TT_pos = index;
}

char * Fld_TTlist::listStream(S1_ind item) const {
	item = dpStream().getVal(E_firstTT) + item;
	return ttStream().ttLstStr(item,inEdit());
}

S1_byte Fld_TTlist::lastInList() const {
	U1_ind thisTT = dpStream().getVal(E_firstTT);
	U1_count lastTT = thisTT + dpStream().getVal(E_noOfTTs);
	S1_byte count = 0;
	while (thisTT < lastTT && f->timeTemps[thisTT].getVal(E_ttTime) != SPARE_TT) {
		++thisTT; 
		++count;
	}
	return count-1;
}

S1_newPage Fld_TTlist::startEdit(S1_ind index) {
	return ttStream().ttSelect(false);
}

S1_newPage Fld_TTlist::saveEdit(S1_ind item){
	ttStream().saveTT();
	U1_ind thisDP = displ_data().dspl_DP_pos + Zone_Stream::currZdtData.currDP; // + zStream().getVal(E_firstDP);
//cout << "Saved TT. First Ind: " << (int)(dpStream().getVal(E_firstTT)) << " Time: " << (int)f->timeTemps[(dpStream().getVal(E_firstTT))].getVal(E_ttTime) << " TT[16] Time: " << (int)f->timeTemps[16].getVal(E_ttTime) << '\n';	
	TT_EpMan::sortTTs(thisDP);
//cout << "Saved TT: First Time: " << (int)f->timeTemps[(dpStream().getVal(E_firstTT))].getVal(E_ttTime) << " TT[16] Time: " << (int)f->timeTemps[16].getVal(E_ttTime) << '\n';	
	S1_byte count = lastInList();
	if (get_active_index() > count) {set_active_index(count);} // on deleting list object at cursor
	return 0;
}

bool Fld_TTlist::onUpDown(S1_inc move) {
	// create new TT in edit mode
	U1_ind ttToCopy  = displ_data().dspl_TT_pos;
	U1_ind thisDP = displ_data().dspl_DP_pos + Zone_Stream::currZdtData.currDP; // + zStream().getVal(E_firstDP);
	U1_ind newTT = TT_EpMan::copyItem(thisDP,ttToCopy,move);
	move = (move <= 0 ? 0:1);
	displ_data().dspl_TT_pos = displ_data().dspl_TT_pos + move; // move -1(up) inserts before,  1 (down) inserts after.
	set_active_index(get_active_index() + move);
	if (newTT == 0) {
	//	//strcpy (Page::MSG1, "Out of memory");
	//	//strcpy (Page::MSG2, "Press <Back>");
	//	//currPage = pgMsg;
	//	// failed
	}
	
	ttStream().ttSelect(true); // Edit new TT
	return true;
}

void Fld_TTlist::cancelEdit(S1_ind item) const {
	if (editTT.isNew()) {
		U1_ind ttToDelete  = dpStream().getVal(E_firstTT) + displ_data().dspl_TT_pos;
		TT_EpMan::recycle(ttToDelete);
		TT_EpMan::sortTTs(dpStream().index());
		S1_byte count = lastInList();
		if (get_active_index() > count) {const_cast<Fld_TTlist *>(this)->set_active_index(count);} // on deleting list object at cursor
	}
	editTT.exitEdit(); 
}

// *********************
Fld_L_Info::Fld_L_Info(Pages * page, U1_byte mode) : Field_List(page, mode) {}

char * Fld_L_Info::listStream(S1_ind item) const {return dStream().objectFldStr(item,inEdit());}

S1_byte Fld_L_Info::lastInList() const {return dStream().getNoOfStreamItems()-1;}

#if defined DEBUG_INFO
void debugDPPos(char * msg) {
	U1_ind currDT = Zone_Stream::currZdtData.currDT;
	U1_ind currDP = getFactory()->dateTimesS(currDT).dpIndex(); 
	U1_ind debug = Zone_Stream::currZdtData.currDP;
	cout << msg <<" currZdtData.currDT:" << (int)currDT << " DT.DPindex:" << (int)currDP << " currZdtData.currDP:" << (int)debug << '\n';
}
#endif