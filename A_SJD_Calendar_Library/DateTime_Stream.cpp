#include "debgSwitch.h"
#include "DateTime_Stream.h"
#include "A_Stream_Utilities.h"
#include "EEPROM_Utilities.h"
#include "D_Edit.h"
#include "D_Factory.h"
//#include "Zone_Sequence.h"
#include "DailyProg_Stream.h"
#include "DateTime_Run.h"

FactoryObjects * getFactory(FactoryObjects * fact);

using namespace Utils;
using namespace EEPROM_Utilities;

EditTime editTime;
EditCurrDate editCurrDate;
EditToDate editToDate;
EditFromDate editFromDate;

///////////////////////////// CurrentDateTime_Stream /////////////////////////////////////
CurrentDateTime_Stream & currTime() {
	static CurrentDateTime_Stream currTime;
	return currTime;
};

CurrentDateTime_Stream::CurrentDateTime_Stream() {}

U1_byte CurrentDateTime_Stream::getDTyr() const {return getYr();}

U1_byte CurrentDateTime_Stream::getDTmnth() const {return getMnth();}

U2_byte CurrentDateTime_Stream::getTimeVal() const {
	return (getHrs() << 7) + (get10Mins() << 4) + minUnits();
}

U2_byte CurrentDateTime_Stream::getDateVal() const {
	return (getYr() << 9) + (getMnth() << 5) + getDay();
}

char * CurrentDateTime_Stream::dateFldStr(bool inEdit) const {
	const_cast<CurrentDateTime_Stream *>(this)->loadTime();
	U2_byte myVal = Format::prepVal(getDateVal(),inEdit);
	S1_byte year = myVal >> 9;
	S1_byte month = myVal >> 5 & 15;
	S1_byte day = myVal & 31;
	cursorOffset = edit.getCursorOffset(cursorOffset,0);

	strcpy(fieldText,getDayStr(getDayNoOfDate(day,month,year)));
	strcat(fieldText," ");
	strcat(fieldText,cIntToStr(day,2,'0'));
	strcat(fieldText," ");
	strcat(fieldText,getMonthStr(month));
	strcat(fieldText," 20");
	strcat(fieldText,cIntToStr(year,2,'0'));
	return formatFieldStr(NEW_LINE_CHR,cursorOffset);
}

char * CurrentDateTime_Stream::timeFldStr(bool inEdit) const {
	U2_byte myVal = Format::prepVal(getTimeVal(),inEdit);
	U1_byte hrs = myVal >> 7;
	U1_byte mins = ((myVal >> 4) & 7 ) * 10 + (myVal& 15);
	cursorOffset = edit.getCursorOffset(cursorOffset,0);
	strcpy(fieldText,cIntToStr(displayHrs(hrs),2,'0'));
	strcat(fieldText,":");
	strcat(fieldText,cIntToStr(mins,2,'0'));
	strcat(fieldText,(hrs < 12) ? "am" : "pm");
	return formatFieldStr(NEW_LINE_CHR,cursorOffset);
}

char * CurrentDateTime_Stream::dstFldStr(bool inEdit) const {
	U1_byte myVal = Format::prepVal(autoDST(),inEdit);
	strcpy(fieldText, myVal ? "Auto":"Off");
	cursorOffset = edit.getCursorOffset(cursorOffset,strlen(fieldText));
	return formatFieldStr(0,cursorOffset);
}

S1_newPage CurrentDateTime_Stream::dateSelect(){
	editCurrDate.setRange(ValRange(15,0,255), getDateVal());
	return DO_EDIT;
}

void CurrentDateTime_Stream::dateSave(){
	S2_byte myVal = edit.get2Val();
	U1_byte nyear = myVal >> 9;
	U1_byte nmonth = myVal >> 5 & 15;
	U1_byte nday = myVal & 31;
	setDayMnthYr(nyear,nmonth,nday);
	saveCurrDateTime();
}

S1_newPage CurrentDateTime_Stream::timeSelect(){
	editTime.setRange(ValRange(7,0,255), getTimeVal());
	return DO_EDIT;
}

void CurrentDateTime_Stream::timeSave(){
	S2_byte myVal = edit.get2Val();
	U1_byte myhrs = myVal >> 7;
	U1_byte mymins = ((myVal >> 4) & 7 ) * 10 + (myVal& 15);
	setHrMin(myhrs,mymins);
	saveCurrDateTime();
}

S1_newPage CurrentDateTime_Stream::dstSelect(){
	edit.setRange(ValRange(0,0,1),autoDST());
	return DO_EDIT;
}

void CurrentDateTime_Stream::dstSave(){
	saveAutoDST(edit.getVal());
	saveCurrDateTime();
}

void CurrentDateTime_Stream::saveAutoDST(U1_byte setAuto) {
	DPIndex = (DPIndex & ~16) | (setAuto * 16);
}

////////////////////////////// DateTime_ser /////////////////////////////////////

template<>
ValRange DataStreamT<EpDataT<DTME_DEF>, DateTime_Run, DT_EpMan, DT_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
} 

template<>
const char DataStreamT<EpDataT<DTME_DEF>, DateTime_Run, DT_EpMan, DT_COUNT>::setupPrompts[DT_COUNT][13] = {};

DateTime_Stream::DateTime_Stream(DateTime_Run & run) : DataStreamT<EpDataT<DTME_DEF>, DateTime_Run, DT_EpMan, DT_COUNT>(run) {
	// can't load from EEprom here because it requires index to have been set
}

//DateTime_Stream & DateTime_Stream::operator= (U4_byte longDT){
//	const void * vPtr = &longDT; // void pointer at longDT
//	const DateTime_Stream * rhs = (reinterpret_cast<const DateTime_Stream*>(vPtr)); // cast void pointer as pointer to DTtype and assign. 
//	mins = rhs->mins;
//	hrs = rhs->hrs;
//	day = rhs->day;
//	mnth = rhs->mnth;
//	year = rhs->year;
//	DPIndex = rhs->DPIndex; // doubles as minute units for CurrentDateTime	
//	return *this;
//}

void DateTime_Stream::setDTdate(U1_byte dthrs, U1_byte dtminsTens, U1_byte dtday, U1_byte dtmnth, U1_byte dtYear, U1_ind dpIndex){
	mins = dtminsTens;
	hrs = dthrs;
	day = dtday;
	mnth = dtmnth;
	year = dtYear;
	DPIndex = dpIndex;
	saveDT(epD().eepromAddr());
}

void DateTime_Stream::setDPindex(U1_byte newDPindex){
	// cannot be called during program startup
	// decrement reference on DP previously pointed to
	f->dailyProgS(DPIndex).incrementRefCount(-1);
	moveDPindex(newDPindex);
	f->dailyProgS(DPIndex).incrementRefCount(1);
}

void DateTime_Stream::moveDPindex(U1_byte newDPindex){
	DPIndex = newDPindex;
	saveDT(epD().eepromAddr());
}

void DateTime_Stream::recycleDT() {
	f->dailyProgS(DPIndex).incrementRefCount(-1);
	setDateTime(DTtype());

#if defined DEBUG_INFO
		std::cout << "\nRecycled DT at " << (int)index() << "\n\n";
		debugDTs(0,"");
#endif
}

E_dpTypes DateTime_Stream::getDPtype() const {
	return f->dailyProgS(DPIndex).getDPtype();
}

//void DateTime_Stream::setDPtype(U1_byte thisDPtype) {
//	f->dailyProgS(DPIndex).setDPtype(thisDPtype);
//}

char * DateTime_Stream::getTimeDateStr(DTtype dt) {
	if (currentTime(true) >= dt){
		strcpy(fieldText,"Now");
	} else {

		strcpy(fieldText,cIntToStr(displayHrs(dt.getHrs()),2,'0'));
		strcat(fieldText,":");
		strcat(fieldText,cIntToStr(dt.get10Mins()*10,2,'0'));
		strcat(fieldText,(dt.getHrs() < 12)  ? "am" : "pm");
		DTtype gotDate = dt.getDate();
		if (gotDate == currTime().getDate()){
			strcat(fieldText, " Today");
		} else if (gotDate == DTtype::nextDay(currTime().getDate())){
			strcat(fieldText, " Tomor'w");
		} else if (gotDate == DTtype::JUDGEMEMT_DAY) {
			strcpy(fieldText,"Judgement Day");
		} else {
			strcat(fieldText," ");
			strcat(fieldText,cIntToStr(dt.getDay(),2,'0'));
			strcat(fieldText,DTtype::getMonthStr(dt.getMnth()));
		}
		if (dt.getYr() != 127 && dt.getYr() > currTime().getDTyr()) strcat(fieldText,cIntToStr(dt.getYr(),2,'0'));
	}
	return fieldText;
}

void DateTime_Stream::copy(DateTime_Stream from) {
	setDateTime(from);
	setDPindex(from.dpIndex());
}

void DateTime_Stream::loadAllDTs(FactoryObjects & fact){
	U1_ind end = fact.numberOf[0].getVal(noAllDTs);
	for (U1_ind i = 0; i < end; ++i) {
		U2_epAdrr loadAddr = fact.dateTimesS(i).epD().eepromAddr();
		fact.dateTimesS(i).loadDT(loadAddr);
	}
}

void DateTime_Stream::saveAllDTs(){
	U1_ind end = f->numberOf[0].getVal(noAllDTs);
	for (U1_ind i = 0; i < end; ++i) {
		U2_epAdrr loadAddr = f->dateTimesS(i).epD().eepromAddr();
		f->dateTimesS(i).saveDT(loadAddr);
	}
}
////////////////////// Edit setRange specialisations ///////////////////

S1_byte EditToDate::editOffset;

void EditTime::setRange(const ValRange & myvRange, S2_byte currVal){
	val = currVal;
	vRange = myvRange;
	vRange.cursorOffset = 4;
	editPtr = &editTime;
	timeInEdit = EDIT_TIME;
}

void EditCurrDate::setRange(const ValRange & myvRange, S2_byte currVal){
	val = currVal;
	vRange = myvRange;
	vRange.cursorOffset = 5;
	editPtr = &editCurrDate;
	timeInEdit = EDIT_TIME;
}

void EditToDate::setRange(const ValRange & myvRange, Zone_Stream * czs){
	val = czs->currZdtData.expiryDate.S4();
	vRange = myvRange;
	vRange.cursorOffset = 9;
	editOffset = vRange.cursorOffset;
	nextCursorPos(0);
	editPtr = &editToDate;
	zs = czs;
	timeInEdit = EDIT_TIME;
}

void EditFromDate::setRange(const ValRange & myvRange, Zone_Stream * czs){
	EditToDate::setRange(myvRange,czs);
	val = czs->currZdtData.fromDate.S4();
	vRange.cursorOffset = 9;
	nextCursorPos(0);
	editPtr = &editFromDate;
}

//////////////////////// Edit nextVal specialisations /////////////////

void EditTime::nextVal(S2_byte move){
 	S2_byte hrs = get2Val() >> 7;
	S2_byte pm = hrs >= 12;
	hrs = DTtype::displayHrs((U1_byte)hrs);
	S2_byte mins = ((val >> 4) & 7 ) * 10 + (val& 15);

	switch (vRange.cursorOffset){
	case 1: // Hour
		hrs = nextIndex(1,hrs,12,move);
		break;
	case 3:
	case 4: // Min
		if (vRange.cursorOffset == 3) move = move * 10;
		mins = nextIndex(0,mins,59,move);
		break;
	case 6: // am/pm
		pm = nextIndex(0,pm,1,move);
		break;
	}
	hrs = hrs + 12 * pm;
	if (hrs == 24) hrs = 0;
	val = (hrs << 7) + ((mins / 10) << 4) + mins % 10;
}

void EditCurrDate::nextVal(S2_byte move){
	S1_byte year = get2Val() >> 9;
	S1_byte month = val >> 5 & 15;
	S1_byte day = val & 31;

	if ((vRange.cursorOffset == 4) || (vRange.cursorOffset == 13)) move = move * 10;

	switch (vRange.cursorOffset){
	case 4:
	case 5: // day
		day = nextIndex(1,day,DTtype::daysInMnth(month,year),S1_byte(move));
		break;
	case 9: // month
		month = nextIndex(1,month,12,S1_byte(move));
		break;
	case 13:
	case 14: // year
		year = nextIndex(0,year,99,S1_byte(move));
		break;
	}
	val = (year << 9) + (month << 5) + day;
}

void EditToDate::nextVal(S2_byte move){
	DTtype newDate(get4Val());
	switch (editOffset) {
	case 0: return; // judgement day
	case 1:	newDate = newDate.addDateTimeNoCarry(move*8); // hrs
		break;
	case 3: newDate = newDate.addDateTimeNoCarry(move); // 10's mins or Now
		break;
	case 5: newDate = newDate.addDateTimeNoCarry(move*96); // am/pm
		break;
	case 8:	newDate = newDate.addDateTimeNoCarry((move*10)<<8); // 10's Day of month
		break;
	case 9:	newDate = newDate.addDateTimeNoCarry(move<<8); // Day of month
		break;
	case 10: newDate = newDate.addDateTimeNoCarry(move<<13); // month
		break;
	}
	setValidDate(newDate);
	nextCursorPos(0);
}

void EditToDate::setValidDate(DTtype newDate) {
	FactoryObjects * f = getFactory();
	DTtype fromDate = f->dateTimesS(zs->currZdtData.currDT).getDateTime();
	if (newDate < fromDate) newDate = fromDate;
	if (zs->currZdtData.currDPtype != E_dpWeekly){ // limit date/time length
		DTtype nextToDate = f->dateTimesS(zs->currZdtData.currDT).getDateTime();
		if (nextToDate <= currentTime(true)){
			nextToDate = currentTime();
			f->dateTimesS(zs->currZdtData.currDT).setDateTime(nextToDate);
		}
		DTtype newPeriod = DTtype::timeDiff(nextToDate,newDate);
		if (newPeriod > DTtype::MAX_PERIOD) { // max 31 hrs 50 mins
			newDate.setDateTime(nextToDate);
			newDate = newDate.addDateTime(DTtype::MAX_PERIOD);
		}
	}
	val = newDate.S4();	
}

void EditFromDate::setValidDate(DTtype newDate) {
	if (newDate <= currentTime(true)){
		newDate = currentTime();
	}
	val = newDate.S4();	
}

///////////////////////// Edit nextCursorPos specialisations ////////////////////

bool EditTime::nextCursorPos(S1_inc move){
	vRange.cursorOffset = nextIndex(1,vRange.cursorOffset,6,move);
	switch (vRange.cursorOffset){
	case 0:
		vRange.cursorOffset = 6;
		break;
	case 2:
		if (move > 0) vRange.cursorOffset = 3;
		else vRange.cursorOffset = 1;
		break;
	case 5:
		if (move > 0) vRange.cursorOffset = 6;
		else vRange.cursorOffset = 4;
		break;
	}
	return false;
}

bool EditCurrDate::nextCursorPos(S1_inc move){
	vRange.cursorOffset = nextIndex(4,vRange.cursorOffset,15,move);
	switch (vRange.cursorOffset){
	case 6: vRange.cursorOffset = 9;	break;
	case 8: vRange.cursorOffset = 5;	break;
	case 10: vRange.cursorOffset = 13;	break;
	case 12: vRange.cursorOffset = 9; break;
	}
	return false;
}

bool EditToDate::nextCursorPos(S1_inc move){
	vRange.cursorOffset = nextIndex(0,vRange.cursorOffset,10,move);
	bool isToday = get4Val().getDate() == currTime().getDate();
	isToday = isToday || (get4Val().getDate() == DTtype::nextDay(currTime().getDate()));
	bool isJudgementDay = get4Val().getDate() == DTtype::JUDGEMEMT_DAY;

	switch (vRange.cursorOffset) {
	case 0: if (move == 0) {
			vRange.cursorOffset = editOffset; // restore vRange.cursorOffset when moving off Now
		} else vRange.cursorOffset = nextIndex(0,vRange.cursorOffset,10,move);
		break;
	case 2:
	case 4: vRange.cursorOffset += move; break; // : or min units so move again
	case 7:	vRange.cursorOffset = 5; break; // between time and date to am/pm	
	case 6: vRange.cursorOffset = 8; break; 
	case 8: // Today or Day 10's
		if (move == 0) vRange.cursorOffset = editOffset; // restore vRange.cursorOffset when moving off Today
		break; 
	case 9: if (move>0 && isToday) vRange.cursorOffset = 1; break;
	}
	editOffset = vRange.cursorOffset;
	DTtype editDate = get4Val().getDateTime();
	if (currentTime() >= editDate)
		vRange.cursorOffset = 0; // Now
	else if (isToday && vRange.cursorOffset >= 8) { // If moving onto Today, change day units, not tens
		if (move != 0) editOffset = 9; 
		vRange.cursorOffset = 8;
	} else if (isJudgementDay) {
		vRange.cursorOffset = 0;
		editOffset = 0;
	}
	return false;
}
