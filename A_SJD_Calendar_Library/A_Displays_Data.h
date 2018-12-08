#if !defined (DISPLAY_DATA_)
#define DISPLAY_DATA_

#include "A__Constants.h"

class Pages;
class Displays;
class Group;

enum testScreens {tsRoomTemps,tsZoneErrors,tsMixValves,tsTowelRads,tsThermStore,tsDebug,noOftestScreens};

enum E_Main_Pages {mp_start,mp_sel_setup, // Bootup pages
	mp_currDTime,mp_ZoneTemps,mp_diary, // home pages
	mp_set_displ,mp_set_nosOf,mp_set_thmStore,mp_set_mixVlves,mp_set_relays,mp_set_tempSnsrs,mp_set_zones,mp_set_twlRads,mp_set_occHtrs, // Initial setup pages 
	mp_zoneUsrSet, mp_edit_diary, mp_prog,// selected user pages
	mp_infoHome, mp_infoStore, mp_infoZones, mp_infoMixValves, mp_infoTowelRails, mp_infoEvents, mp_infoTest,
	MP_NO_OF_MAIN_PAGES}; 

enum E_Remote_Pages {rp_ZoneTemp, rp_diary, rp_edit_diary, rp_prog, RP_NO_OF_REMOTE_PAGES}; 
//********************************************************************
//**************** Collection Data Structure *************************
//********************************************************************

/*
Every object derived from Collection has an active_index and count data mamber.
Each derived class also has its own data structure as a member object. Display has D_Data, Page has P_Data and Field has F_Data.
Children of Display have a parent reference (& p_data, & f_data etc), so that all children have access to all ancestor data.
In addition, all classes have a displ_data() function returning the D_Data struct for the root display.
The Data for most classes in the hierarchy also includes a pointer to either an array of child pointers, or to the first member of a child list.
Where the number of children is known at compile time, both the array, and the child-members are embedded as class member data (see more below).

A list-Field active_index is the index of the list item with the cursor. Non-List fields have active_index set to -2. This indicates
a non-list field to Page.stream, saving the overhead of a dynamic cast to determin object type.

For Displays we want array access to the member pages for ease of page switching. Each element of an array must be the same size.
We can either have an array of Pages or an array of pointers to Pages. Since we know at compile time the size of the array, 
we prefer to have the array as a member object, rather than off the heap. For consistancy we build pages the same way with arrays of fields,
and again we know the array size at compile time, so prefer a member array. 
Embedding the fields in the page as member objects means that pages will not be the same size. 
Thus our arrays cannot be arrays of objects, but must be arrays of pointers to embedded objects.

An alternative is to have each data struct contain a pointer to the first member object, with each member object having a pointer to the next,
forming a forward-linked list. The space requirement is the same, with the array pointers being moved to the member objects. 
A linked list is easier to manage for child classes where the number of objects is not a compile-time constant.

Thus, Displays, Pages and will be implemented with known sized arrays of pointers. But fields, zones and below will have array size1
pointing to the first member of a single linked list.

Since the abstract data classes do not know how big the array is, delay adding that data mamber to the concrete classes.

Display, Page and Field data is abstracted to these structs so that fields can access display data via the struct pointer.
*/

struct F_Data;
struct D_Data;
struct P_Data;

struct D_Data { // activeIndex is current page no, count is the number of pages.
	D_Data();
	S1_ind			dspl_Parent_index;  // index of page ed_group, because each field needs to relate to the same Zone
	S1_ind			dspl_DT_pos;		// offest of displayed DateTime sequence from current DT
	S1_ind			dspl_DP_pos;		// offset of displayed Daily Program from current DP, (duplicate named DPs are counted, not skipped)
	S1_ind			dspl_TT_pos;		// offset of displayed TempTime from current TT
	S1_ind			bpIndex;			// current index in backPage list
	B_fld_isSel		hasCursor;			// the display is selected, i.e. has a cursor showing somewhere
	static B_fld_isSel	displayInEdit;		
 	mutable B_fld_isSel	thisIteminEdit;		// the currently serialising field element is in edit
    S1_ind			backPage[10];		// index of page numbers visted
    S1_ind			backField[10];		// index of active fields
	S1_ind			backList[10];		// index of list position
};

struct P_Data { // activeIndex is field with cursor, count is the number of fields.
	P_Data(Displays * myParent, Group & ed_group, U1_ind defaultFld) : display(myParent), ed_group(ed_group), defaultField(defaultFld) {}
	Displays *		display;
	Group &	ed_group; // ref to first element in an aggregate of data objects supporting [] indexing.
	U1_ind	defaultField; // Field initially receiving cursor when page selected
};

struct F_Data {// activeIndex is item in list with cursor or -2 for non-list fields, count : 0=selectable,  1 = unselectable, 2 = hidden,
	F_Data(Pages * myParent) : page(myParent) {}
	// data mambers
	Pages *		page;
};

#endif