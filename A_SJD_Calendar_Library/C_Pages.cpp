#include "C_Pages.h"
#include "D_Factory.h"
#include "B_Displays.h"
#include "Zone_Run.h"
#include "ThrmStore_Run.h"


// ******************************************************************************
// ************************* Specific pages *************************************
//*******************************************************************************

// ********************* Name_List_Pages *******************
Name_List_Pages::Name_List_Pages(Displays * display, Group & ed_group, const char* lbl_text, bool has_name) 
	:Pages(display,ed_group), f_lbl(this, lbl_text), f_t_Name(this), f_Lst_members(this)
{	
	pageFields[0] = &f_lbl;
	if (has_name) {
		set_noOf(3);
		pageFields[1] = &f_t_Name;
		pageFields[2] = &f_Lst_members;
	} else {
		set_noOf(2);
		pageFields[1] = &f_Lst_members;
	}
}

Object_List_Pages::Object_List_Pages(Displays * display, Group & ed_group, const char* lbl_text) 
	:Pages(display,ed_group), f_lbl(this, lbl_text), f_Lst_members(this)
{	
	set_noOf(2);
	pageFields[0] = &f_lbl;
	pageFields[1] = &f_Lst_members;
	Zone_Run::checkProgram(true);
	f->thermStoreR().check();
}

Info_List_Pages::Info_List_Pages(Displays * display, Group & ed_group, const char* lbl_text) 
	:Pages(display,ed_group), f_lbl(this, lbl_text), f_Lst_members(this)
{	
	set_noOf(2);
	pageFields[0] = &f_lbl;
	pageFields[1] = &f_Lst_members;
}

// ********************* Start_Page *******************
Start_Page::Start_Page(Displays * display) 
	: Pages(display, f->dateTimes), // f->numberOf unused Group ref.
	f_l_ZP (this,"Zone Programmer"),
	f_l_V(this,"Version " VERSION), // adjacent literal concaternation
	f_l_H1(this,"- Press $ to scroll_  through screens -"),
	secondsOnPage(SECONDS_ON_VERSION_PAGE)
	{
	set_noOf(3);
	pageFields[0] = &f_l_ZP;
	pageFields[1] = &f_l_V;
	pageFields[2] = &f_l_H1; 
}

char * Start_Page::stream (S1_ind list_index) const {
	secondsOnPage--;
	if (secondsOnPage < 0) {
		page_data().display->newPage(mp_ZoneTemps);
	}
	return Pages::stream (list_index);
};

// ********************* Zone_UsrSet_Page *******************
Zone_UsrSet_Page::Zone_UsrSet_Page(Displays * display)
	:Pages(display,f->zones,6), 
	f_lbl_ZN(this,"Zone$ "),
	f_t_Name(this),
	f_lbl_ZA(this,"Abbrev:"),
	f_t_Abbrev(this),
	f_lbl_ZO(this,"Offset:",~NEW_LINE),
	f_t_ZOffset(this),
	f_Lst_ZUsrSet(this)
{
	set_noOf(E_noOfFlds);
	pageFields[znLbl] = &f_lbl_ZN;
	pageFields[name] = &f_t_Name;
	pageFields[abbrLbl] = &f_lbl_ZA; 
	pageFields[E_zabbrev] = &f_t_Abbrev;
	pageFields[osLbl] = &f_lbl_ZO;
	pageFields[offset] = &f_t_ZOffset;
	pageFields[usrSet] = &f_Lst_ZUsrSet;
}

// ********************* Cur_DateTime_Page *******************
Cur_DateTime_Page::Cur_DateTime_Page(Displays * display)
	:Pages(display, f->numberOf,1), // f->numberOf unused Group ref.
	f_t_CurrDate(this),
	f_t_CurrTime(this),
	f_lbl_DST(this,"DST:",~NEW_LINE),
	f_t_DSTmode(this),
	f_lbl_editHelp(this,"- <>/$ to edit -"),
	f_cmd_hr(this,"+Hr ",NEW_LINE | SELECTABLE),
	f_cmd_10m(this,"+10Mins "),
	f_cmd_min(this,"+Min ")
{	
	set_noOf(8);
	pageFields[0] = &f_t_CurrDate;
	pageFields[1] = &f_t_CurrTime;
	pageFields[2] = &f_lbl_DST; 
	pageFields[3] = &f_t_DSTmode;
	pageFields[4] = &f_lbl_editHelp;
	pageFields[5] = &f_cmd_hr;
	pageFields[6] = &f_cmd_10m;
	pageFields[7] = &f_cmd_min;
}

// ********************* Diary_Page *******************
Diary_Page::Diary_Page(Displays * display)
	:Pages(display,f->zones,3), 
	f_lbl_zone(this,"Diary for$ "),
	f_t_Name(this),
	f_lbl_from(this,"From$"),
	f_t_fromDate(this),
	f_lbl_to(this,"To:  "),
	f_t_toDate(this,~SELECTABLE),
	f_lbl_prog(this,"Prog: "),
	f_t_prog(this,SELECTABLE)
{	
	set_noOf(E_DnoOfFlds);
	pageFields[E_DzoneLbl] = &f_lbl_zone;
	pageFields[E_Dname] = &f_t_Name;
	pageFields[E_DfromLbl] = &f_lbl_from; 
	pageFields[E_Dfrom] = &f_t_fromDate;
	pageFields[E_DtoLbl] = &f_lbl_to;
	pageFields[E_Dto] = &f_t_toDate; 
	pageFields[E_DprogLbl] = &f_lbl_prog;
	pageFields[E_Dprog] = &f_t_prog;
}

// ********************* EditDiary_Page *******************
EditDiary_Page::EditDiary_Page(Displays * display)
	:Pages(display,f->zones,E_from), 
	f_lbl_edit(this,"Edit "),
	f_cmd_insert(this,"@Ins "),
	f_cmd_del(this,"@Del "),
	f_cmd_prog(this,"@Prg"),
	f_cmd_commit(this,"@Delete Diary Entry ",SELECTABLE|HIDDEN),
	f_t_abbrev(this,CHANGEABLE|SELECTABLE|NEW_LINE), // added CHANGEABLE
	f_lbl_prog(this,"Prog: ",~NEW_LINE),
	f_t_prog(this), // removed ~SELECTABLE
	f_lbl_from(this,"From$"),
	f_t_fromDate(this),
	f_lbl_to(this,"To: "),
	f_t_toDate(this) // removed ~SELECTABLE 
{	
	set_noOf(E_noOfFlds);
	pageFields[E_edit] = &f_lbl_edit;
	pageFields[E_ins] = &f_cmd_insert;	
	pageFields[E_del] = &f_cmd_del;	
	pageFields[E_prgPge] = &f_cmd_prog;	
	pageFields[E_commit] = &f_cmd_commit;		
	pageFields[E_zabbrev] = &f_t_abbrev;
	pageFields[E_progLbl] = &f_lbl_prog;
	pageFields[E_prog] = &f_t_prog;
	pageFields[E_fromLbl] = &f_lbl_from; 
	pageFields[E_from] = &f_t_fromDate;
	pageFields[E_toLbl] = &f_lbl_to;
	pageFields[E_to] = &f_t_toDate;
}

Fields_Base & EditDiary_Page::getField(S1_ind index) {
	return reinterpret_cast<Fields_Base &> (* pageFields[index]);
}

B_fld_desel	EditDiary_Page::back_key() { // returns true if object is deselected
	get_active_member().back_key();
	if (Field_Cmd::command() != Field_Cmd::C_none) {
		if (Field_Cmd::command() == Field_Cmd::C_delete) {
			setFields();
		}
		Field_Cmd::command(Field_Cmd::C_none);
		return true;
	}
	for (int i=0; i < Factory::noOf(noZones); i++) {
		f->zoneR(i).refreshCurrentTT(*f);
	}
	return false;
}

void EditDiary_Page::setFields() {
	// modify field modes on this page
	getField(E_edit).mode(~HIDDEN);
	getField(E_ins).mode(~HIDDEN);	
	getField(E_del).mode(~HIDDEN);
	getField(E_prgPge).mode(~HIDDEN);
	getField(E_commit).mode(HIDDEN);	
	set_active_index(E_ins);
}

// ********************* Program_Page *******************
Program_Page::Program_Page(Displays * display)
	:Pages(display,f->zones,2),
	f_t_abbrev(this),
	f_lbl_prog(this,"Prog$ ",~NEW_LINE),
	f_t_prog(this,EDITABLE),
	f_lbl_days(this,"Days$ "),
	f_t_days(this),
	f_lbl_dayOff(this,"Day Off: ",HIDDEN),
	f_t_dayOff(this,EDITABLE|HIDDEN),
	f_lbl_tts(this,"TT: "),
	f_l_tts(this,EDITABLE),
	f_lbl_dp_type(this,"Program Type: ",HIDDEN),
	f_cmd_weekly(this,"@Weekly ",SELECTABLE|HIDDEN),
	f_cmd_dayOff(this,"@Day Off ",SELECTABLE|HIDDEN),
	f_cmd_inOut(this,"@Temporary In/Out",SELECTABLE|HIDDEN)
{	
	set_noOf(E_pnoOfFlds);
	pageFields[E_pzabbrev] = &f_t_abbrev;
	pageFields[E_pprogLbl] = &f_lbl_prog;
	pageFields[E_pprog] = &f_t_prog;
	pageFields[E_daysLbl] = &f_lbl_days; 
	pageFields[E_pdays] = &f_t_days;
	pageFields[E_DOlbl] = &f_lbl_dayOff;
	pageFields[E_DO] = &f_t_dayOff;
	pageFields[E_TTlbl] = &f_lbl_tts;
	pageFields[E_TTs] = &f_l_tts;
	pageFields[E_dpTypeLbl] = &f_lbl_dp_type;
	pageFields[E_Cmd_Wk] = &f_cmd_weekly;
	pageFields[E_Cmd_DO] = &f_cmd_dayOff;
	pageFields[E_Cmd_IO] = &f_cmd_inOut;
}

Fields_Base & Program_Page::getField(S1_ind index) {
	return reinterpret_cast<Fields_Base &> (* pageFields[index]);
}

//B_fld_desel	Program_Page::back_key() { // returns true if object is deselected
//	return (Fld_Cmd_Edit::command() != Fld_Cmd_Edit::C_none);
//}
// ********************* Test_I2C_Page *******************
//enum Test_I2C_Page_fldIndex {E_lbl_Addr, E_lbl_Relay, E_r_Result, E_noOfFlds};
Test_I2C_Page::Test_I2C_Page(Displays * display) 
	:Pages(display,f->zones,E_r_Result),
	f_lbl_Addr(this,"^v Addr",~NEW_LINE),
	f_lbl_Relay(this,"<> Reg, Sel(on/off)"),
	f_i_Result(this)
{
	set_noOf(E_noOfFlds);
	pageFields[E_lbl_Addr] = &f_lbl_Addr;
	pageFields[E_lbl_Relay] = &f_lbl_Relay; 
	pageFields[E_r_Result] = &f_i_Result;
}

Fields_Base & Test_I2C_Page::getField(S1_ind index) {
	return reinterpret_cast<Fields_Base &> (* pageFields[index]);
}

// ********************* Remote_Temp_Page *******************
Remote_Temp_Page::Remote_Temp_Page(Displays * display, Group & ed_group, const char* lbl_text) 
	:Pages(display,ed_group), f_lbl(this, lbl_text), f_remoteTemps(this)
{	
	set_noOf(2);
	pageFields[0] = &f_lbl;
	pageFields[1] = &f_remoteTemps;
}