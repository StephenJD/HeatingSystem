#pragma once

#include "D_Edit.h"
#include "A__Constants.h"

class MV_Edit// :	public Edit
{
public:
	MV_Edit(void);
	~MV_Edit(void);
	char setup_edit_parms(char myListPos);
	bool saveEdit();
private:
	U1_byte noOfEditVals;
	U1_byte noOfTempSensors;
	U1_byte noOfRelays;

};

