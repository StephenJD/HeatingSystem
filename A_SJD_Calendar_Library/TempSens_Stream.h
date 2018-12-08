#pragma once

#include "D_DataStream.h"
#include "D_EpData.h"

class TempSens_EpMan;
class TempSens_Run;

class TempSens_Stream : public DataStreamT<EpDataT<TEMPS_DEF>, TempSens_Run, TempSens_EpMan, TS_COUNT>
{
public:	
	TempSens_Stream(TempSens_Run & run);
private:
	//TempSens_Stream(const TempSens_Stream &); // don't permit copying
	char * objectFldStr(S1_ind activeInd, bool inEdit) const;
	bool editOnUpDown() const;
	S1_newPage startEdit(D_Data & d_data);
	void save(); 
	// ************* Accessor methods for statics needing defining for each sub class 
	const char * getSetupPrompts(S1_ind item) const {return "";}
	static bool showAddress;
};