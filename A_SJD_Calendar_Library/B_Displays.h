#if !defined (b_displays_)
#define b_displays_
#define mocking

#include "A_Collection.h"
#include "A_Displays_Data.h"
#include "Keypad.h"

// This file contains the Display Collection classes defining the pages they contain.
// A Display collection defines a single Displays by pointing to an array of page Collection pointers, each handling a single page
// The base class active_index is the current Page, whilst count is the number of Pages.


// ********************* Displays *******************
// Single Responsibility: Abstract class specialising the collections interface for Collections of Pages
// It delegates unhandled method calls to the Pages.
// activeIndex is the page currently displayed
// The D_Data struct maintains the edit status, page history for the Back key, and Currently displayed indexes for the fields on a page.

class Displays : public Collection
{
public:
    // overloaded methods
	B_fld_isSel		up_down_key (S1_inc move);
	B_to_next_fld	left_right_key (S1_inc move);
	S1_newPage		select_key(); // returns true if the field data goes into / out of edit mode
	B_fld_desel		back_key(); // process the key
	void			newPage(S1_ind newPage); // required by setup-key
	B_selectable	validate_active_index (S1_inc direction); // returns true if can be made active
	// Display Virtual Functions
	virtual bool processKeys() = 0;
	virtual ~Displays();

	//New Functions
	#if defined (ZPSIM)
	virtual void setKey(S1_byte key) =0;
	#endif

	U1_byte streamToLCD(bool showCursor);
	int rows() {return _lcd->_numlines;}
	int cols() {return _lcd->_numcols;}
	MultiCrystal * lcd() {return _lcd;}
	Keypad * keypad;
protected:
	Displays(MultiCrystal * lcd, Keypad * keypad ); //protected prevents creation of an unspecialised Displays
	
	friend class Pages;
	D_Data &		displ_data(); // {return d_data;}
	const D_Data &	displ_data() const; // {return d_data;}
	void	set_page_default_field(S1_ind index, bool defaultField = true);

	D_Data d_data;
	MultiCrystal * _lcd;
	mutable Collection * page;
	mutable U1_ind pageIndex;
	static FactoryObjects * f;
	friend void setFactory(FactoryObjects *);
};

#endif