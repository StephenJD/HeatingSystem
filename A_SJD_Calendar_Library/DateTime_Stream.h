#pragma once

#include "A__Constants.h"
#include "D_DataStream.h"
#include "D_EpData.h"
#include "DateTime_Run.h"
#include "Zone_Stream.h"
#include "DateTime_EpMan.h"
//class FactoryObjects;

///////////////////////////// CurrentDateTime_Stream /////////////////////////////////////

class CurrentDateTime_Stream : public Curr_DateTime_Run, public Format 
{
public:
	CurrentDateTime_Stream();
	
	char * dateFldStr(bool inEdit) const;
	char * timeFldStr(bool inEdit) const;
	char * dstFldStr(bool inEdit) const;
	U1_byte getDTyr() const;
	U1_byte getDTmnth() const;

	S1_newPage dateSelect();
	void dateSave();
	S1_newPage timeSelect();
	void timeSave();
	S1_newPage dstSelect();
	void dstSave();
	void saveAutoDST(U1_byte);

protected:
	U2_byte getTimeVal() const;
	U2_byte getDateVal() const;
};
CurrentDateTime_Stream & currTime();
////////////////////////////// DateTime_Stream /////////////////////////////////////

class DateTime_Stream : public DTtype, public DataStreamT<EpDataT<DTME_DEF>, DateTime_Run, DT_EpMan, DT_COUNT>
{
public:
	DateTime_Stream(DateTime_Run & run);
	// specialised methods

	// extra queries
	U1_byte dpIndex() const {return DPIndex;}
	static char * getTimeDateStr(DTtype dt);
	E_dpTypes getDPtype() const;

	// extra modifiers
	void setDTdate(U1_byte hrs, U1_byte minsTens, U1_byte day, U1_byte mnth, U1_byte year,U1_ind dpIndex);
	void setDPindex(U1_byte newDPindex);
	void moveDPindex(U1_byte newDPindex);
	void copy(DateTime_Stream from);
	void recycleDT();
	static void loadAllDTs(FactoryObjects & fact);
	static void saveAllDTs();
	// additional queries
};

////////////////////////////// Edit Specialisations /////////////////////////////////////

class EditTime : public Edit {
public:	
	void setRange(const ValRange & vRange, S2_byte currVal);
	void nextVal(S2_byte move);
	bool nextCursorPos(S1_inc move);
} extern editTime;

class EditCurrDate : public Edit {
public:	
	void setRange(const ValRange & vRange, S2_byte currVal);
	void nextVal(S2_byte move);
	bool nextCursorPos(S1_inc move);
} extern editCurrDate;

class EditToDate : public Edit {
public:	
	void setRange(const ValRange & vRange, Zone_Stream * czs);
	DTtype get4Val() {return DTtype(val);}
	void nextVal(S2_byte move);
	bool nextCursorPos(S1_inc move);
private:
	virtual void setValidDate(DTtype newDate);
	static S1_byte editOffset; // position being edited, as opposed to visible cursor position
} extern editToDate;

class EditFromDate : public EditToDate {
public:	
	void setRange(const ValRange & vRange, Zone_Stream * czs);
private:
	void setValidDate(DTtype newDate);
} extern editFromDate;