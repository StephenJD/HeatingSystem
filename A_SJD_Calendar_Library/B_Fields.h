#if !defined (b_fields_)
#define b_fields_

#include "A_Collection.h"
#include "A_Displays_Data.h"
#include "B_Pages.h"
//#include <string>

class Zone_Stream;
class DataStream;
class DailyProg_Stream;
class TimeTemp_Stream;

// ********************* Fields_Base *******************
// Single Responsibility: Abstract class specialising the collections interface for Collections of field members.
// It delegates unhandled method calls to the objects supplying the data.
// The base class active_index is either NOT_A_LIST, LIST_NOT_ACTIVE or the index of the list item with the cursor.
// count is re-used as the display mode for the field.
// It maintains references to the parent page, and the field type.

// ********************* Fields_Base *******************
class Fields_Base : public Collection
{
public:
	// specialised methods
	B_fld_isSel		up_down_key (S1_inc move);
	B_to_next_fld	left_right_key (S1_inc move);
	S1_newPage		select_key ();
	B_fld_desel		back_key (); // returns true if object is deselected
	bool			modeIs(U1_byte tryMode) const;
	void			mode(U1_byte newMode);
	virtual S1_newPage onSelect(S1_byte myListPos) {return startEdit(myListPos);} // returns DO_EDIT for edit, +ve for change page, 0 do nothing;

protected:	
	Fields_Base (Pages * page, U1_byte mode); // protected constructor makes class abstact

	const F_Data &	field_data() const {return f_data;}
	Pages &			page() const {return *(f_data.page);}
	D_Data &		displ_data();
	const D_Data &	displ_data() const;
	Group &			group() const ; 
	DataStream &	dStream() const;
	Zone_Stream &	zStream() const;
	DailyProg_Stream &	dpStream() const;
	TimeTemp_Stream &	ttStream() const;
	S1_ind			parent_index() const;
	void			set_parent_index(S1_ind index);

	Collection &	get_member_at (S1_ind index) const; // never called
	void			make_member_active (S1_inc move);
	B_selectable	validate_active_index (S1_inc direction); // returns true if can be made active
	// new methods
	bool inEdit() const;
	char lineMode() const;	
	// new virtual methods
	virtual bool	edit_on_updn() const {return true;}
	virtual S1_newPage startEdit(S1_byte myListPos) {return 0;} // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	virtual S1_byte lastInList() const;
	virtual	S1_newPage saveEdit(S1_ind item);
	virtual	void cancelEdit(S1_ind item) const;

	F_Data f_data;
	static FactoryObjects * f;
	friend void setFactory(FactoryObjects *);
private:
	virtual bool onUpDown(S1_inc move);
	S1_newPage save_Edit(S1_ind item);
	U1_byte mode() const;
};

// ********************* Field_Cmd *******************
// Single Responsibility: Abstract class specialising the Fields_Base interface for Command fields.
// Maintains the Command text.
class Field_Cmd : public Fields_Base
{
public:
	enum Command {C_none,C_edit,C_insert,C_delete,C_prog,C_commit,C_weekly,C_dayOff,C_inout};
	char * stream (S1_ind list_index = 0) const;
	~Field_Cmd();
	static void command(Command newCmd) {cmd = newCmd;}
	static Command command() {return cmd;}
protected:	
	Field_Cmd(Pages * page, const char * label_text, U1_byte mode);
private:
	const char * text;
	static Command cmd;
};

// ********************* Fld_Label *******************
// Single Responsibility: Concrete class specialising the Field_Cmd interface for Label fields.
class Fld_Label : public Field_Cmd
{
public:
	Fld_Label(Pages * page, const char * label_text, U1_byte mode = NEW_LINE);
};

// ********************* Fld_Name *******************
// Single Responsibility: Concrete base class specialising the Fields_Base interface for Name fields.
class Fld_Name : public Fields_Base // Up/Down moves to next member
{
public:
	Fld_Name(Pages * page, U1_byte mode = EDITABLE);
	char *	stream (S1_ind list_index = 0) const;
private:
	S1_newPage onSelect(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_newPage saveEdit(S1_ind item); 
	S1_ind lastMembIndex() const; // number of members in a group 
	bool  edit_on_updn() const {return false;}
};

// ********************* Field_List *******************
// Single Responsibility: Abstract base class specialising the Fields_Base interface for List fields.
// Maintains index of first and last visible item in the list

class Field_List : public Fields_Base
{
public:
	char *	stream (S1_ind list_index = 0) const;
	S1_newPage saveEdit(S1_ind item); 
	// new list virtual
	virtual char * listStream(S1_ind item) const = 0;

protected:
	Field_List(Pages * page, U1_byte mode = EDITABLE | NEW_LINE);
	friend class Pages;
	B_selectable validate_active_index(S1_inc direction); // returns true if can be made active, active_index set to number of members - 1.
	S1_newPage	 startEdit(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;

	mutable S1_ind first_list_index;
	mutable S1_ind last_list_index;
};

// ********************* Fld_I2C_Addr *******************
// Single Responsibility: Concrete base class specialising the Fields_Base interface for I2C_Addr fields.
class Fld_I2C_Addr : public Fields_Base // Up/Down moves to next member
{
public:
	Fld_I2C_Addr(Pages * page, U1_byte mode = CHANGEABLE);
	char *	stream (S1_ind list_index = 0) const;
	B_fld_isSel		up_down_key (S1_inc move);
	B_to_next_fld	left_right_key (S1_inc move);
	S1_newPage		select_key ();
private:
	//S1_newPage onSelect(S1_byte myListPos); // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	S1_ind lastMembIndex() const; // number of members in a group 
	bool  edit_on_updn() const {return false;}
	int addrIndex;
	int regIndex;
	bool relayOn;
};

#endif