#include "MixValve_Edit.h"
#include "D_EpDat_A.h"


MV_Edit::MV_Edit(void)
{
}

char MV_Edit::setup_edit_parms(char myListPos){// Returns -1 for edit, +ve for change page, 0 do nothing
	// On entering edit Mode sets editVal1, minVal, noOfEditVals & editLength if > 1.
	//switch (myListPos) { // mvFlowSensID, mvStoreSensor
	//case mvStoreSensor: // TempSensors
	//case mvFlowSensID:		
	//	noOfEditVals = noOfTempSensors;
	//	break;
	//}
	return -1;
}

bool MV_Edit::saveEdit(){
	//if (editField == f_name) {
	//	copyTrimmed(editText,editText);
	//	setName(editText);
	//} else {
	//	setVal(editListPos,(byte) editVal1);
	//	switch (editListPos) {
	//	case mvStoreSensor:
	//		strcpy(tempText,"MVs");
	//		tempText[3] = ep_index + 48;
	//		tempText[4] = 0;
	//		if (tempSensors[U1_byte(editVal1)].hasNoName()) tempSensors[U1_byte(editVal1)].setName(tempText);
	//		break;
	//	}
	//}
	//editField = f_nothing;
	return false;
}