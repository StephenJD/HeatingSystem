#if !defined (b_pages_)
#define b_pages_

#include "A_Collection.h"
#include "A_Displays_Data.h"

class Fields_Base;

class Displays;
// This file contains the Pages classes defining all the pages

// ********************* Pages *******************
// Single Responsibility: Abstract class specialising the collections interface for Collections of Fields
// It delegates unhandled method calls to the Fields.
// The base class active_index is the field with the cursor, whilst count is the number of fields.
// It maintains references to the parent display, and the object supplying data for the fields.

/*enum pageTypes {pgMsg,pgSetup,pgDate,pgZoneTemps,pgZoneProgs,pgProgTemps,pgZoneDef,pgOverideDP,pgDayOffDP,
	pgProgName, pgNewDT, pgDayOffSelect,
	pgDisplaySetup,pgNoOfSetup,pgThmStrSetup,pgMixValveSetup,pgRelaySetup,pgTempSensSetup,pgZoneSetup,pgTwlRadSetup,pgOccaslHtrsSetup,
	pgNoOfPages};
*/

// ********************* Pages *******************
class Pages : public Collection
{
public:
	char *			stream (S1_ind list_index) const;
	B_to_next_fld	left_right_key (S1_inc move);
	const P_Data &	page_data() const;
	friend class Fields_Base;
	virtual Fields_Base & getField(S1_ind index);
protected:
	D_Data &		displ_data();
	const D_Data &	displ_data() const;

	Pages(Displays * display, Group & ed_group, U1_ind defaultFld =0);
	static FactoryObjects * f;
	friend void setFactory(FactoryObjects *);
private:
	P_Data p_data;
};
#endif