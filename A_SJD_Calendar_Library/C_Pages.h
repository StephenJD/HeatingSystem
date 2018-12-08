#if !defined (c_pages_concrete_)
#define c_pages_concrete_

#include "B_Pages.h"
#include "C_Fields.h"
class Group;
// ******************************************************************************
// ************************* Concrete Pages *************************************
//*******************************************************************************

// ********************* List_Pages *******************
// Single Responsibility: Concrete classes specialising Pages.

class Name_List_Pages : public Pages
{
public:
	Name_List_Pages(Displays * display, Group & ed_group, const char* lbl_text, bool has_name = true);
protected:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
private:
	Collection*		pageFields[3];
	Fld_Label		f_lbl;
	Fld_Name		f_t_Name;
	Fld_L_Items		f_Lst_members;
};

class Object_List_Pages : public Pages
{
public:
	Object_List_Pages(Displays * display, Group & ed_group, const char* lbl_text);
protected:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
private:
	Collection*		pageFields[2];
	Fld_Label		f_lbl;
	Fld_L_Objects	f_Lst_members;
};

class Info_List_Pages : public Pages
{
public:
	Info_List_Pages(Displays * display, Group & ed_group, const char* lbl_text);
protected:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
private:
	Collection*		pageFields[2];
	Fld_Label		f_lbl;
	Fld_L_Info		f_Lst_members;
};

// Single Responsibility: To specialise the Pages collections defining the field collection for each page
// Maintains an array of member fields.

// ********************* Start_Page *******************
class Start_Page : public Pages
{
public:
	Start_Page(Displays * display);
private:
	// specialised methods
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	//B_selectable	validate_active_index (S1_inc direction) {return true;} // all pages are valid
	B_fld_isSel		change_to_selected() {return false;}
	char *			stream (S1_ind list_index = 0) const; 
	// extra data members	
	Collection*		pageFields[3]; // an array of Collection pointers
	Fld_Label		f_l_ZP;
	Fld_Label		f_l_V;
	Fld_Label		f_l_H1;
	mutable S1_byte	secondsOnPage;
};


// ********************* Zone_UsrSet_Page *******************
class Zone_UsrSet_Page : public Pages
{
public:
	Zone_UsrSet_Page(Displays * display);
private:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	enum fldIndex {znLbl,name,abbrLbl,E_zabbrev,osLbl,offset,usrSet,E_noOfFlds};
	Collection*		pageFields[E_noOfFlds];
	Fld_Label		f_lbl_ZN;
	Fld_EdZname		f_t_Name;
	Fld_Label		f_lbl_ZA;
	Fld_EdZAbbrev	f_t_Abbrev;
	Fld_Label		f_lbl_ZO;
	Fld_ZOffset		f_t_ZOffset;
	Fld_L_ZUsr		f_Lst_ZUsrSet;
};

// ********************* Cur_DateTime_Page *******************
class Cur_DateTime_Page : public Pages
{
public:
	Cur_DateTime_Page(Displays * display);
private:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	Collection*		pageFields[8];
	Fld_CurrDate	f_t_CurrDate;
	Fld_CurrTime	f_t_CurrTime;
	Fld_Label		f_lbl_DST;
	Fld_DSTmode		f_t_DSTmode;
	Fld_Label		f_lbl_editHelp;
	Fld_Cmd_Hr		f_cmd_hr;
	Fld_Cmd_10m		f_cmd_10m;
	Fld_Cmd_Min		f_cmd_min;
};

// ********************* Diary_Page *******************
class Diary_Page : public Pages
{
public:
	Diary_Page(Displays * display);
private:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	enum fldIndex {E_DzoneLbl,E_Dname,E_DfromLbl,E_Dfrom,E_DtoLbl,E_Dto,E_DprogLbl,E_Dprog,E_DnoOfFlds};
	Collection*		pageFields[E_DnoOfFlds];
	Fld_Label		f_lbl_zone;	
	Fld_Name		f_t_Name;
	Fld_Label		f_lbl_from;	
	Fld_Zfrom		f_t_fromDate;
	Fld_Label		f_lbl_to;
	Fld_Zto			f_t_toDate;
	Fld_Label		f_lbl_prog;
	Fld_Zprog		f_t_prog;
};

// ********************* EditDiary_Page *******************
enum editDiaryfldIndex {E_edit,E_ins,E_del,E_prgPge,E_commit,E_zabbrev,E_progLbl,E_prog,E_fromLbl,E_from,E_toLbl,E_to,E_noOfFlds};
class EditDiary_Page : public Pages
{
public:
	EditDiary_Page(Displays * display);
	Fields_Base & getField(S1_ind index);
	B_fld_desel	back_key(); // returns true if object is deselected
	void setFields();
private:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	Collection*		pageFields[E_noOfFlds];
	Fld_Label		f_lbl_edit;	
	Fld_Cmd_Insert	f_cmd_insert;	
	Fld_Cmd_Delete	f_cmd_del;	
	Fld_Cmd_Prog	f_cmd_prog;	
	Fld_Cmd_Commit	f_cmd_commit;		
	Fld_ZAbbrev		f_t_abbrev;
	Fld_Label		f_lbl_prog;	
	Fld_Zprog		f_t_prog;	
	Fld_Label		f_lbl_from;	
	Fld_ZEdfrom		f_t_fromDate;
	Fld_Label		f_lbl_to;
	Fld_Zto			f_t_toDate;
};

// ********************* Program_Page *******************
enum Program_PagefldIndex {E_pzabbrev,E_pprogLbl,E_pprog,E_daysLbl,E_pdays,E_DOlbl,E_DO,E_TTlbl,E_TTs,E_dpTypeLbl,E_Cmd_Wk,E_Cmd_DO,E_Cmd_IO, E_pnoOfFlds};
class Program_Page : public Pages
{
public:
	Program_Page(Displays * display);
	Fields_Base & getField(S1_ind index);
//	B_fld_desel	back_key(); // returns true if object is deselected

private:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	Collection*		pageFields[E_pnoOfFlds];
	Fld_ZAbbrev		f_t_abbrev;
	Fld_Label		f_lbl_prog;	
	Fld_ProgName	f_t_prog;	
	Fld_Label		f_lbl_days;	
	Fld_DPdays		f_t_days;
	Fld_Label		f_lbl_dayOff;	
	Fld_DPdayOff	f_t_dayOff;
	Fld_Label		f_lbl_tts;	
	Fld_TTlist		f_l_tts;
	Fld_Label		f_lbl_dp_type;	
	Fld_Cmd_wk		f_cmd_weekly;	
	Fld_Cmd_do		f_cmd_dayOff;	
	Fld_Cmd_io		f_cmd_inOut;	
};

// ********************* Test_I2C_Page *******************
class Test_I2C_Page : public Pages
{
public:
	enum Test_I2C_Page_fldIndex {E_lbl_Addr, E_lbl_Relay, E_r_Result, E_noOfFlds};
	Test_I2C_Page(Displays * display);
	Fields_Base & getField(S1_ind index);
	//B_fld_desel	back_key(); // returns true if object is deselected
	//void setFields();
private:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
	Collection*		pageFields[E_noOfFlds];
	Fld_Label		f_lbl_Addr;	
	Fld_Label		f_lbl_Relay;	
	Fld_I2C_Addr	f_i_Result;
};

class Remote_Temp_Page : public Pages
{
public:
	Remote_Temp_Page(Displays * display, Group & ed_group, const char* lbl_text);
protected:
	Collection &	get_member_at(S1_ind index) const {return * pageFields[index];}
private:
	Collection*		pageFields[2];
	Fld_Label		f_lbl;
	Fld_RemoteTemp	f_remoteTemps;
};

#endif