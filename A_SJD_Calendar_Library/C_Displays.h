#if !defined (c_display_concrete)
#define c_display_concrete

#include "debgSwitch.h"
#include "B_Displays.h"
#include "B_Pages.h"
#include "C_Pages.h"
#include "Keypad.h"

// includes for users of this class
//#include "Setup.h"
//#include "MixingValve.h"
//#include "TempSensor.h"
//#include "Zone.h"

// ********************* Main_display *******************
// Single Responsibility: To specialise the Displays collections defining the pages for the main display
// Maintains an array of member pages.

//enum E_Main_Pages {mp_start,mp_sel_setup,mp_set_displ,
//	mp_set_nosOf,mp_set_thmStore,mp_set_mixVlves,mp_set_relays,mp_set_tempSnsrs,mp_set_zones,mp_set_twlRads,mp_set_occHtrs,
//	mp_zoneUsrSet,MP_NO_OF_MAIN_PAGES};**************  defined in EditableData ****************

class Main_display : public Displays
{
public:
	Main_display(MultiCrystal * lcd, Keypad * keypad);
	bool processKeys();
	#if defined (ZPSIM)
	void setKey(S1_byte key);
	#endif
protected:
	Collection &	get_member_at(S1_ind index) const;
private:
	void setBLformular();
};

void streamToLCD(Displays * display, MultiCrystal * lcd);

//// ********************* Remote_display *******************
class Remote_display : public Displays {
public:
	Remote_display(MultiCrystal * lcd, Keypad * keypad, U1_ind zone);
	bool processKeys();
	void resetDisplay() {_lastTemp = 0;}
	#if defined (ZPSIM)
	void setKey(S1_byte key);
	#endif
protected:
	Collection &	get_member_at(S1_ind index) const;
private:
	U1_ind _zone;
	U1_byte _lastTemp;
};

#endif