#pragma once

#include "D_DataStream.h"
#include "D_EpData.h"
#include "TimeTemp_EpMan.h"

class DTtype;

class TimeTemp_Stream : public DataStreamT<EpDataT<TMTP_DEF>, RunT<EpDataT<TMTP_DEF> >, TT_EpMan, 1>
{
public:	
	TimeTemp_Stream(RunT<EpDataT<TMTP_DEF> > & run);
	void setTemp(U1_byte temp);
	U1_byte getTemp();
	DTtype getTime();
	void saveTT();
	S1_newPage ttSelect(bool insert);
	char * ttLstStr (U1_ind item, bool inEdit) const;
private:	
	// ************* Accessor methods for statics needing defining for each sub class 
	const char * getSetupPrompts(S1_ind item) const {return 0;}
};

////////////////////////////// Edit Specialisations /////////////////////////////////////

class EditTT : public Edit {
public:	
	void setRange(const ValRange & myvRange, U1_byte ttTime,  U1_byte ttTemp, bool insert);
	void nextVal(S2_byte move);
	bool nextCursorPos(S1_inc move); // return true if moving out of field
} extern editTT;