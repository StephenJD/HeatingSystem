#if !defined (collection_)
#define collection_

#include "A__Constants.h"
#include "D_EpManager.h"
struct D_Data;
class FactoryObjects;
// ********************* Collection *******************
// Single Responsibility: To provide the interface and default behaviour for collections
// It maintains the active member and number of members in the collection

class Collection
{
public:
	virtual char *			stream (S1_ind list_index = 0) const; // list_index used by Field_List::stream
	virtual B_fld_isSel		up_down_key (S1_inc move); // returns true if field is selected
	virtual B_to_next_fld	left_right_key (S1_inc move); // returns true if object changes active index
	virtual S1_newPage		select_key(); // returns DO_EDIT if the field data goes into / out of edit mode, returns new page number.
	virtual B_fld_desel		back_key(); // returns true if object is deselected
	virtual void			set_active_index (S1_ind index){active_index = index;}
	virtual ~Collection(){} // confirmed requirement to be virtual
protected:
	friend class Pages;
	friend class Displays;
	virtual S1_ind			lastMembIndex() const; // returns no of members of the Collection -1
	virtual Collection &	get_member_at (S1_ind index) const = 0; // returns a reference to the member at index
	virtual void			make_member_active (S1_inc move); // makes the next member active
	virtual B_fld_isSel		change_to_selected(); // returns true if sucessful
	
	Collection &			get_active_member() const; // returns a reference to the active member
	void					change_to_unselected();
	S1_ind					get_active_index() const {return active_index;}
	
	Collection();

	virtual B_selectable	validate_active_index (S1_inc direction); // returns true if can be made active
	B_fld_isSel				is_selected() const;
	void					set_noOf(U1_count noOf) {count = noOf;}

	virtual D_Data &		displ_data() = 0;
	virtual const D_Data &	displ_data() const = 0;
private:
	S1_ind			active_index; // widely used index of active member of Collection	
	U1_count		count;	// number of members in collection
};

#endif