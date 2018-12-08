#include "TimeTemp_Stream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"

using namespace Utils;

template<>
const char DataStreamT<EpDataT<TMTP_DEF>, RunT<EpDataT<TMTP_DEF> >, TT_EpMan, 1>::setupPrompts[1][13] = {};

template<>
ValRange DataStreamT<EpDataT<TMTP_DEF>, RunT<EpDataT<TMTP_DEF> >, TT_EpMan, 1>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}

TimeTemp_Stream::TimeTemp_Stream(RunT<EpDataT<TMTP_DEF> > & run)
	: DataStreamT<EpDataT<TMTP_DEF>, RunT<EpDataT<TMTP_DEF> >, TT_EpMan, 1>(run)
	{}

void TimeTemp_Stream::setTemp(U1_byte temp) { // temp in whole degrees
	setVal(E_ttTemp, temp + 10);	
}

U1_byte TimeTemp_Stream::getTemp() {
	return getVal(E_ttTemp) % 128 -10;
}

DTtype TimeTemp_Stream::getTime() {
	return getVal(E_ttTime);
}

char * TimeTemp_Stream::ttLstStr (U1_ind item, bool inEdit) const {
	cursorOffset = 0;
	U1_byte myTTtime;
	S1_byte myTTtemp;
	if (inEdit) {
		myTTtime = editTT.get2Val() >> 8;
		myTTtemp = editTT.get2Val() & 255;

	} else {
		myTTtime = f->timeTemps[item].getVal(E_ttTime);
		myTTtemp = f->timeTemps[item].getVal(E_ttTemp);
	}
	myTTtemp = myTTtemp % 128 -10;
	strcpy(fieldText,cIntToStr(DTtype::displayHrs(myTTtime/8),2,'0'));
	strcat(fieldText,cIntToStr(myTTtime % 8 *10,2,'0'));
	if (inEdit && editTT.isNew()) {
		strcat(fieldText, "DEL");
	} else {
		strcat(fieldText, (myTTtime / 8 < 12) ? "a" : "p");
		strcat(fieldText,cIntToStr(myTTtemp,2,'0'));
	}

	cursorOffset = edit.getCursorOffset(cursorOffset, strlen(fieldText));
	return createFieldStr(fieldText, 0, cursorOffset);
}


S1_newPage TimeTemp_Stream::ttSelect(bool insert) {
	editTT.setRange(ValRange(7), getVal(E_ttTime),getVal(E_ttTemp),insert);
	return DO_EDIT;
}

void TimeTemp_Stream::saveTT(){
	setVal(E_ttTime, edit.get2Val() >> 8);
	setVal(E_ttTemp, edit.get2Val() & 255);
}

EditTT editTT;

void EditTT::setRange(const ValRange & myvRange, U1_byte ttTime,  U1_byte ttTemp, bool insert){
	val = ttTime * 256 + ttTemp;
	vRange = myvRange;
	vRange.cursorOffset = 1;
	editPtr = &editTT;
	if (insertMode == 0) insertMode = insert;
	timeInEdit = EDIT_TIME;
}

void EditTT::nextVal(S2_byte move){
	if (insertMode == 1) {
		insertMode = -1;
		return;
	} else if (insertMode == -1) insertMode = 0;

	U1_byte time = get2Val() >> 8;
	U1_byte temp = get2Val() & 255;
	switch (vRange.cursorOffset) {
	case 4: // am/pm
		move = move * 12;
	case 1: {// Hrs
		U1_byte hrs = time / 8;
		hrs = nextIndex(0,hrs,23,S1_byte(move));
		time = (time & 7) + hrs * 8;
		}
		break;
	case 3: {// Mins
		U1_byte mins = time % 8;
		mins = nextIndex(0,mins,5,S1_byte(move));
		time = (time & ~7) + mins;
		}
		break;
	case 5:  // Temp 10's
		move = move * 10;
	case 6:  // Temp
		temp = nextIndex(1,temp,109,S1_byte(move));
		break;
	}
	val = (time << 8) + temp;
}

bool EditTT::nextCursorPos(S1_inc move){
	if (isNew()) insertMode = 0;
	vRange.cursorOffset =  vRange.cursorOffset + move;
	if (vRange.cursorOffset == 2) vRange.cursorOffset =  vRange.cursorOffset + move;
	if (vRange.cursorOffset > 6 || vRange.cursorOffset < 1) return true;
	else return false;
}