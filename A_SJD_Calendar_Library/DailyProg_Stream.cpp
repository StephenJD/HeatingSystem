#include "debgSwitch.h"
#include "DailyProg_Stream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "DailyProg_EpMan.h"
#include "B_Fields.h"
#include "TimeTemp_EpMan.h"
using namespace Utils;

#if defined DEBUG_INFO
	#include <iostream>
	#include <string>
	using namespace std;
#endif

void debugDPPos(char * msg); // fwd declare. In C_Fields.cpp

template<>
const char DataStreamT<EpDataT<DLYPRG_DEF>, DailyProg_Run, DP_EpMan, 1>::setupPrompts[1][13] = {};

template<>
ValRange DataStreamT<EpDataT<DLYPRG_DEF>, DailyProg_Run, DP_EpMan, 1>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}

DailyProg_Stream::DailyProg_Stream(DailyProg_Run & run)
	: DataStreamT<EpDataT<DLYPRG_DEF>, DailyProg_Run, DP_EpMan, 1>(run) {
}

//U1_byte const DailyProg_Stream::fullWeekValue;
//U1_byte const DailyProg_Stream::MonValue;
//U1_byte const DailyProg_Stream::M_ThValue;
//U1_byte const DailyProg_Stream::Tu_FrValue;
//U1_byte const DailyProg_Stream::We_SaValue;
//U1_byte const DailyProg_Stream::Th_SuValue;

void DailyProg_Stream::newDPData(){
	strcpy(fieldText,"DProg");
	strcat(fieldText,cIntToStr(index()+1,2,'0'));
	setName(fieldText);
	setVal(E_noOfTTs,1);
	setVal(E_firstTT,0);
	setVal(E_days,255);
}

void DailyProg_Stream::saveName(S1_ind parentZ){
	U1_ind dpInd = index();
	U1_ind newdpInd = dpInd;
	S1_byte ref;
	if (getDPtype() == E_dpNew) { // create new New Prog
		int debug = dpInd;
		newdpInd = DP_EpMan::copyItem(parentZ,dpInd);
		if (debug != dpInd) 
			debug = debug;
		switch (Field_Cmd::command()) {
		case  Field_Cmd::C_weekly: { // create TT for this DP		
			TT_EpMan::copyItem(newdpInd,0,0);
			ref = f->dailyProgS(dpInd).refCount();
			f->dailyProgS(newdpInd).setVal(E_dpType,E_dpWeekly);
			f->dailyProgS(newdpInd).refCount(0);
			f->dailyProgS(newdpInd).setDays(ALL_DAYS);
			} break;
		case Field_Cmd::C_dayOff:
			f->dailyProgS(newdpInd).setVal(E_dpType,E_dpDayOff);
			break;
		case Field_Cmd::C_inout:
			f->dailyProgS(newdpInd).setVal(E_dpType,E_dpInOut);
			break;
		default:
			;		
		}
		ref = f->dailyProgS(dpInd).refCount();
		f->dailyProgS(newdpInd).setName(editChar.getChars());

		if (f->dateTimesS(Zone_Stream::currZdtData.currDT).dpIndex() != Zone_Stream::currZdtData.currDP) { // true if we got here direct from edit page. False if we first went to Program page
			f->dateTimesS(Zone_Stream::currZdtData.currDT).setDPindex(newdpInd); // required when newProg selected on Edit page.
		}
#if defined (DEBUG_INFO)
		debugDPPos("Save Name - post move DPindex");
		debugDTs(parentZ, "Save Name DTs - post move DPindex");
		std::cout << debugDPs("Save Name - post move DPindex");
#endif

	}
	// save the name
	U1_ind endGroup = newdpInd;
	while (!f->dailyProgS(endGroup).endOfGroup()) {++endGroup;}
	while (f->dailyProgS(newdpInd).getDays() < MONDAY) {--newdpInd;}
	while (newdpInd <= endGroup) { // change names of all DP's of this name
		f->dailyProgS(newdpInd).setName(editChar.getChars());
		if (editChar.getChars()[0] == ' ') { // delete Prog
			DP_EpMan::recycleDP(parentZ, newdpInd);
		}		
		++newdpInd;
	}
	ref = f->dailyProgS(newdpInd).refCount();
	Field_Cmd::command(Field_Cmd::C_none);
}

void DailyProg_Stream::setDays(U1_byte myDays) {
	setVal(E_days,(getVal(E_days) & FULL_WEEK) | myDays);
}

void DailyProg_Stream::endOfGroup(bool isFullWeek) {
	if (isFullWeek) { // set bit
		setVal(E_days, getVal(E_days) | FULL_WEEK);
	} else { // clear bit
		setVal(E_days, getVal(E_days) & ALL_DAYS);
	}	
}

U1_byte DailyProg_Stream::endOfGroup() const {
	return getVal(E_days) & FULL_WEEK;
}

bool DailyProg_Stream::startOfGroup() const {
	return ((getVal(E_days) & MONDAY) != 0);
}

U1_ind DailyProg_Stream::getFirstDPinGroup() const {
	U1_ind i = index();
	while (!f->dailyProgS(i).startOfGroup()) {--i;}
	return i;
}

U1_byte DailyProg_Stream::getDays() const {return getVal(E_days) & ALL_DAYS;} // return days only, not fullWeek bit.

//bool DailyProg_Stream::isStartOfGroup() const {return (getVal(E_days) & MonValue) > 0;}

E_dpTypes DailyProg_Stream::getDPtype() const {
	return static_cast<E_dpTypes>(getVal(E_dpType) & (E_dpRefCount * 3));
}

U1_ind DailyProg_Stream::getNewProg(U1_ind startPos) {
	E_dpTypes dpType;
	do {
		dpType = f->dailyProgS(startPos).getDPtype();
	} while (dpType != E_dpNew && ++startPos);
	return startPos;
}

S1_byte DailyProg_Stream::refCount() const {
	return getVal(E_dpType) % E_dpRefCount;
}

void DailyProg_Stream::refCount(S1_byte count) {
	U1_byte dpType = getDPtype();
	if (count <= 0) {
		count = 0;
		//if (refCount() > 0) {
			//DP_EpMan::recycleDP(255,index());
		//}
	}
	setVal(E_dpType,dpType + count);
}

S1_byte DailyProg_Stream::incrementRefCount(S1_byte add) {
	// for each DP in a group - same name, same refcount if Weekly Type
	U1_ind startGroup = DP_EpMan::startOfGroup(index());
	U1_ind endGroup = DP_EpMan::lastInGroup(index());
	U1_ind oldCount = refCount();
	S1_byte newCount = oldCount + add;
	for (int i = startGroup; i<= endGroup; ++i) {
		f->dailyProgS(i).refCount(newCount);
	}
	if (oldCount > 0 && newCount == 0) {
		DP_EpMan::recycleDP(255,index());
	}
	return newCount;
}

U1_byte DailyProg_Stream::getDayOff() const { // returns day number of regular "day off"
	return getVal(E_days) & ~FULL_WEEK;
}

char *DailyProg_Stream::dpDayOffStr(bool inEdit, char newLine) const {
	cursorOffset = 0;
	U1_byte myDayOff;
	if (inEdit) myDayOff = edit.getVal();
	else myDayOff = getDayOff();

	switch (myDayOff){
	case 64: strcpy(fieldText,"Monday"); break;
	case 32: strcpy(fieldText,"Tuesday"); break;
	case 16: strcpy(fieldText,"Wednesday"); break;
	case 8: strcpy(fieldText,"Thursday"); break;
	case 4: strcpy(fieldText,"Friday"); break;
	case 2: strcpy(fieldText,"Saturday"); break;
	default: strcpy(fieldText,"Sunday"); break;
	}
	cursorOffset = edit.getCursorOffset(cursorOffset, strlen(fieldText));
	return formatFieldStr(newLine, cursorOffset);
}

S1_newPage DailyProg_Stream::dayOffEdit(){
	editDayOff.setRange(ValRange(0), getDayOff());
	return DO_EDIT;
}

void DailyProg_Stream::dayOffSave(){
	setVal(E_days,edit.getVal());
}

char *DailyProg_Stream::daysToFullStr(bool inEdit, char newLine) const {
	cursorOffset = 0;
	U1_byte myDays;
	if (inEdit) myDays = edit.getVal();
	else myDays = getVal(E_days);

	if (myDays & 64) strcpy(fieldText,"M"); else strcpy(fieldText,&EDIT_SPC_CHAR);
	if (myDays & 32) strcat(fieldText,"T"); else strcat(fieldText,&EDIT_SPC_CHAR);
	if (myDays & 16) strcat(fieldText,"W"); else strcat(fieldText,&EDIT_SPC_CHAR);
	if (myDays & 8) strcat(fieldText,"T"); else strcat(fieldText,&EDIT_SPC_CHAR);
	if (myDays & 4) strcat(fieldText,"F"); else strcat(fieldText,&EDIT_SPC_CHAR);
	if (myDays & 2) strcat(fieldText,"S"); else strcat(fieldText,&EDIT_SPC_CHAR);
	if (myDays & 1) strcat(fieldText,"S"); else strcat(fieldText,&EDIT_SPC_CHAR);	
	cursorOffset = edit.getCursorOffset(cursorOffset, strlen(fieldText));
	return formatFieldStr(newLine, cursorOffset);
}

S1_newPage DailyProg_Stream::daysSelect(){
	editDays.setRange(ValRange(7), getDays());
	return DO_EDIT;
}

void DailyProg_Stream::daysSave(){
	setDays(edit.getVal());
}

////////////////////// Edit setRange specialisations ///////////////////

EditProg editProg;
EditDays editDays;
EditDayOff editDayOff;

void EditProg::setRange(const ValRange & myvRange, U1_byte currDP, Zone_Stream * czs){
	val = currDP;
	vRange = myvRange;
	vRange.cursorOffset = -1;
	zs = czs;
	editPtr = &editProg;
	timeInEdit = EDIT_TIME;
}

void EditProg::nextVal(S2_byte move){
	val = zs->nextDP(getVal(),static_cast<S1_byte>(move));
}

void EditDays::setRange(const ValRange & myvRange, U1_byte currDays){
	val = currDays;
	vRange = myvRange;
	editPtr = &editDays;
	timeInEdit = EDIT_TIME;
}

void EditDays::nextVal(S2_byte move){
	U1_byte mask = 64 >> vRange.cursorOffset;
	U1_byte newVal = (val & mask);
	val = (val | mask) ^ newVal;
}

void EditDayOff::setRange(const ValRange & myvRange, U1_byte currDayOff){
	val = currDayOff;
	vRange = myvRange;
	editPtr = &editDayOff;
	timeInEdit = EDIT_TIME;
}

void EditDayOff::nextVal(S2_byte move){
	if (move < 0)
		val = val / 2;
	else val = val * 2;
	if (val < 1) val = 64;
	if (val > 64) val = 1;
}