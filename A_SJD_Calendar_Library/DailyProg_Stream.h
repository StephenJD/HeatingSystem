#pragma once

#include "D_DataStream.h"
#include "D_EpData.h"
#include "DailyProg_Run.h"
#include "DailyProg_EpMan.h"

const U1_byte ALL_DAYS = 127;
const U1_byte FULL_WEEK = 128;
const U1_byte MONDAY = 64; // /2 for each day, Sunday = 1

class DailyProg_Stream : public DataStreamT<EpDataT<DLYPRG_DEF>, DailyProg_Run, DP_EpMan, 1>
{
public:	
	DailyProg_Stream(DailyProg_Run & run);
	void saveName(S1_ind parentZ);

	// extra query methods
	U1_byte endOfGroup() const; // returns 128 for end of group
	bool startOfGroup() const;
	E_dpTypes getDPtype() const;
	S1_byte refCount() const;
	char * daysToFullStr(bool inEdit, char newLine) const;
	char * dpDayOffStr(bool inEdit, char newLine) const;
	U1_byte getDays() const; // return days only, not fullWeek bit.
	U1_byte getDayOff() const;
	static U1_ind getNewProg(U1_ind startPos);

	// extra modifier methods
	void newDPData();
	S1_byte incrementRefCount(S1_byte add);
	void refCount(S1_byte count); // refCount for all DP's in a group need be correct
	S1_newPage daysSelect();
	S1_newPage dayOffEdit();
	void daysSave();
	void dayOffSave();
	void setDays(U1_byte myDays);
	void endOfGroup(bool isFullWeek);
	U1_ind getFirstDPinGroup() const;

private:
	// extra methods
	void setDayOff(U1_byte day); // Set the day to use as Day-off program.
	//static const U1_byte MonValue = 64;
	//static const U1_byte M_ThValue = 64+32+16+8;
	//static const U1_byte Tu_FrValue = 32+16+8+4;
	//static const U1_byte We_SaValue = 16+8+4+2;
	//static const U1_byte Th_SuValue = 8+4+2+1;

};

////////////////////////////// Edit Specialisations /////////////////////////////////////

class EditProg : public Edit {
public:	
	void setRange(const ValRange & vRange, U1_byte currDP, Zone_Stream * czs);
	void nextVal(S2_byte move);
private:
} extern editProg;

class EditDays : public Edit {
public:	
	void setRange(const ValRange & vRange, U1_byte currDays);
	void nextVal(S2_byte move);
private:
} extern editDays;

class EditDayOff : public Edit {
public:	
	void setRange(const ValRange & vRange, U1_byte currDayOff);
	void nextVal(S2_byte move);
private:
} extern editDayOff;