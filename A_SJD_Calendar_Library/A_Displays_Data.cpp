#include "A_Displays_Data.h"

B_fld_isSel	D_Data::displayInEdit = false;	

D_Data::D_Data() :
	dspl_Parent_index(0), 
	dspl_DT_pos(0),
	dspl_DP_pos(0),
	dspl_TT_pos(0),
	bpIndex(0),
	hasCursor(false),
    thisIteminEdit(false)
	{
		backPage[0] = 0;
		backField[0] = 0;
		backList[0] = 0;
	}