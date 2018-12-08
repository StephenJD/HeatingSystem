#if !defined (c_fields_)
#define c_fields_

#include "A_Collection.h"
#include "A_Displays_Data.h"
#include "B_Pages.h"
#include "B_Fields.h"
#include "D_EpData.h"

void debugDPPos(char * msg);

// ********************* Concrete Fld_Name Classes *******************
// Single Responsibility: To overload Fld_Name classes for specific text fields.
//enum Field_Types {f_name,f_abbrev,f_setup,f_usr_set,f_usr_Lst,f_z_editName,f_editAbbrev,f_date,f_time,f_z_temps,
//	f_fromDate,f_toDate,f_prog,f_insertFromDate,f_insertToDate};

// ********************* Field_Cmds *******************
// Single Responsibility: Concrete class specialising the Field_Cmd interface for specific commands.

class Fld_Cmd_Insert : public Field_Cmd {
public:
	Fld_Cmd_Insert(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_Delete : public Field_Cmd {
public:
	Fld_Cmd_Delete(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_Prog : public Field_Cmd {
public:
	Fld_Cmd_Prog(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_Commit : public Field_Cmd {
public:
	Fld_Cmd_Commit(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_wk : public Field_Cmd {
public:
	Fld_Cmd_wk(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_do : public Field_Cmd {
public:
	Fld_Cmd_do(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_io : public Field_Cmd {
public:
	Fld_Cmd_io(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_Hr : public Field_Cmd {
public:
	Fld_Cmd_Hr(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_10m : public Field_Cmd {
public:
	Fld_Cmd_10m(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

class Fld_Cmd_Min : public Field_Cmd {
public:
	Fld_Cmd_Min(Pages * page, const char * label_text, U1_byte mode = SELECTABLE);
	S1_newPage onSelect(S1_byte);
};

// ********************* Derived Fld_Name classes *******************
// Single Responsibility: To overload Fld_Name classes for specific text fields.

// ******************* Next Member on up/down *****************
class Fld_EdZname : public Fld_Name {
public:
	Fld_EdZname(Pages * page, U1_byte mode = EDITABLE);
private:
	S1_newPage onSelect(S1_byte); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
};

class Fld_ZAbbrev : public Fld_Name {
public:
	Fld_ZAbbrev(Pages * page, U1_byte mode = CHANGEABLE);
	char * stream(S1_ind item) const;
private:
//	S1_newPage onSelect(S1_byte); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
};

class Fld_ProgName : public Fld_Name {
public:
	Fld_ProgName(Pages * page, U1_byte mode = CHANGEABLE);
	char * stream(S1_ind item) const;
	B_fld_desel	back_key (); // process the key
private:
	S1_newPage saveEdit(S1_ind item); 
	S1_newPage onSelect(S1_byte); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	void make_member_active (S1_inc move);
	void setFields() const;
	mutable E_dpTypes pageMode;
};

// ********************* Derived Field_Base classes *******************
// Single Responsibility: To overload Fields_Base classes for specific text fields.
// ********************** Edit on up/down *********************
class Fld_EdZAbbrev : public Fields_Base { 
public:	
	Fld_EdZAbbrev(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item); 
};

class Fld_ZOffset : public Fields_Base {
public:	
	Fld_ZOffset(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_CurrDate : public Fields_Base {
public:	
	Fld_CurrDate(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_CurrTime : public Fields_Base {
public:	
	Fld_CurrTime(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_DSTmode : public Fields_Base {
public:	
	Fld_DSTmode(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_RemoteTemp : public Fields_Base {
public:	
	Fld_RemoteTemp(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

// ************** Diary Page Fields *******************
class Fld_Zfrom : public Fields_Base {
public:	
	Fld_Zfrom(Pages * page, U1_byte mode = CHANGEABLE);
	char * stream(S1_ind item) const;
	B_fld_isSel	up_down_key (S1_inc move);
private:
	S1_newPage onSelect(S1_byte myListPos);
};

class Fld_ZEdfrom : public Fields_Base {
public:	
	Fld_ZEdfrom(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
	B_fld_isSel	up_down_key (S1_inc move);
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_Zto : public Fields_Base {
public:	
	Fld_Zto(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
	B_fld_isSel	up_down_key (S1_inc move);
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_Zprog : public Fields_Base {
public:	
	Fld_Zprog(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage onSelect(S1_byte myListPos);
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

// ************** Program Page Fields *****************

class Fld_DPdays : public Fields_Base {
public:	
	Fld_DPdays(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
	void make_member_active(S1_inc move);
	bool edit_on_updn() const {return false;}

};

class Fld_DPdayOff : public Fields_Base {
public:	
	Fld_DPdayOff(Pages * page, U1_byte mode = EDITABLE);
	char * stream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

// ********************* Field_List types *******************

class Fld_L_Items : public Field_List
{
public:
	Fld_L_Items(Pages * page, U1_byte mode = EDITABLE | NEW_LINE);
	char * listStream(S1_ind item) const;
private:
	S1_newPage startEdit(S1_byte myListPos);// returns DO_EDIT for edit, +ve for change page, 0 do nothing;
};

class Fld_L_Objects : public Field_List
{
public:
	Fld_L_Objects(Pages * page, U1_byte mode = EDITABLE | NEW_LINE);
	char * listStream(S1_ind item) const;
private:	
	S1_byte lastInList() const;
	S1_newPage onSelect(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	bool edit_on_updn() const;
	S1_newPage startEdit(S1_byte myListPos);// returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_L_ZUsr : public Field_List {
public:
	Fld_L_ZUsr(Pages * page, U1_byte mode = EDITABLE | NEW_LINE);	
	char * listStream(S1_ind item) const;
private:	
	S1_byte lastInList() const;
	S1_newPage startEdit(S1_byte myListPos);// returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
};

class Fld_TTlist : public Field_List {
public:
	Fld_TTlist(Pages * page, U1_byte mode = EDITABLE | NEW_LINE);	
	char * listStream(S1_ind item) const;
	void set_active_index (S1_ind index);

private:
	bool onUpDown(S1_inc move);
	S1_newPage startEdit(S1_byte myListPos);// returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item);
	S1_byte lastInList() const;
	bool edit_on_updn() const {return false;};
	void cancelEdit(S1_ind item) const;
};

class Fld_L_Info : public Field_List {
public:
	Fld_L_Info(Pages * page, U1_byte mode = SELECTABLE);	
	char * listStream(S1_ind item) const;
private:	
	S1_byte lastInList() const;
};
#endif