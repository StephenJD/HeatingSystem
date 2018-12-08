#include "debgSwitch.h"
#include "Zone_Stream.h"
#include "D_Factory.h"
#include "A_Displays_Data.h"
#include "Zone_Run.h"
#include "Relay_Run.h"
//#include "Zone_Sequence.h"
#include "A_Stream_Utilities.h"
#include "D_Edit.h"
#include "DailyProg_Stream.h"
#include "Event_Stream.h"
#include <string>

class Zone_EpMan;
EditTempReq editTempReq;

#if defined (ZPSIM)
	#include <iostream>
	#include <sstream>
	#include <iomanip>
#endif

using namespace Utils;

template<>
const char DataStreamT<Zone_EpD, Zone_Run, Zone_EpMan, Z_SET_VALS>::setupPrompts[Z_SET_VALS][13] = {
	"CallRelay:",
	"CallTempS:",
	"FlowTempS:",
	"Max FlowT:", // last item for DHW Zone
	"Mix Valve:",
	"WarmBoost:"};

template<>
ValRange DataStreamT<Zone_EpD, Zone_Run, Zone_EpMan, Z_SET_VALS>::addValToFieldStr(S1_ind item, U1_byte myVal) const{
	switch (item) {
	case zCallRelay:
		strcat(fieldText,f->relays[myVal].dStream().getName());
		return ValRange(0,0,f->relays.last());
	case zCallTempSensr:
	case zFlowTempSensr:
		strcat(fieldText,f->tempSensors[myVal].dStream().getName());
		return ValRange(0,0,f->tempSensors.last());
	case zMaxFlowT:
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,30,90);
	case zMixValveID:
		strcat(fieldText,f->mixingValves[myVal].dStream().getName());
		return ValRange(0,0,f->mixingValves.last());
	case zWarmBoost: // last zone setup val
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,0,20);
	case zOffset: // Used for setup of run-time of Towel Rails
		strcat(fieldText,cIntToStr(myVal,3,'0'));
		return ValRange(3,0,255);
	default:
		return ValRange(0);
	}
}

const char Zone_Stream::userPrompts[][40] = {
	"Zone_group_for_shared_programs:",
	"Room_temp_reduced_when_OS_within:",
	"Auto_predict_warm-up_time:",
	"Manual_Mins_per_1/2_degree_heating:",
	"Auto_Temp_Limit:", // Added to request temp as the limit to which the curve tends
	"Auto_Time_Constant:"
};

ZdtData Zone_Stream::currZdtData = {U1_byte(-1)}; // initialise with non-zero firstDT

void Zone_Stream::setZdata(S1_ind dtPos, S1_inc move) const { // defaults: dtPos = -1, move = 0
	if (getVal(E_firstDT) != currZdtData.firstDT) { // complete refresh
#if defined DEBUG_INFO		
		std::cout << "\nsetZdata:refresh\n\n";
#endif
		currZdtData.firstDT = getVal(E_firstDT) ;
		currZdtData.lastDT = currZdtData.firstDT + getVal(E_NoOfDTs);
		currZdtData.firstDP = getVal(E_firstDP);
		//currZdtData.fromDate = currentTime(true);
		dtPos = 0; // required to refresh 'from' when changing zone to display
	}
	if (dtPos >= 0) currZdtData.viewedDTposition = dtPos; // for update, no DTpos provided
	if (dtPos == 0) 
		currZdtData.fromDate = currentTime(true);
	else if (move < 0) { // moving forward one. From has already found expiryDate, To is next expiryDate.
		currZdtData.fromDate = currZdtData.expiryDate;
	} else if (move > 0) { // going backwards, From is expiryDate of one before this, To is expiryDate of this.
		runI().getNthDT(currZdtData, currZdtData.viewedDTposition-1);
		currZdtData.fromDate = currZdtData.expiryDate;
	} else if (dtPos == CHECK_FROM)  { // -2
		runI().getNthDT(currZdtData, currZdtData.viewedDTposition-1);
		currZdtData.fromDate = currZdtData.expiryDate;
		if (currZdtData.viewedDTposition == 0) currZdtData.fromDate = currentTime(true);
	}

	currZdtData.currDT = runI().getNthDT(currZdtData, VIEWED_DT); // sets currZdtData.expiryDate
	currZdtData.currDP = f->dateTimesS(currZdtData.currDT).dpIndex(); 
	currZdtData.currDPtype = f->dailyProgS(currZdtData.currDP).getDPtype();
}

Zone_Stream::Zone_Stream(Zone_Run & run) 
	: DataStreamT<Zone_EpD, Zone_Run, Zone_EpMan, Z_SET_VALS>(run)
	{}

char * Zone_Stream::getAbbrev() const {return epDi().getAbbrev();}

void Zone_Stream::setAutoMode(U1_byte mode) {epDi().setAutoMode(mode);}

U1_byte Zone_Stream::getAutoMode() const {return epDi().getAutoMode();}

U1_byte Zone_Stream::getGroup() const {return epDi().getGroup();}

U1_byte Zone_Stream::getWeatherSetback() const {return epDi().getWeatherSetback();}

const char * Zone_Stream::getUsrPrompts(S1_ind item) const {	
	//if (item > 2 && !getAutoMode()) item = item + 2;
	return userPrompts[item];
}

char * Zone_Stream::abbrevFldStr(bool inEdit, char newLine) const {
	setZdata();
	cursorOffset = 0;
	char * myChar;
	if (inEdit) myChar = editChar.getChars();
	else myChar = getAbbrev();
	cursorOffset = edit.getCursorOffset(cursorOffset,strlen(myChar));
	return createFieldStr(myChar, newLine,cursorOffset);
}

char * Zone_Stream::offsetFldStr(bool inEdit, char newLine) const {
	S1_byte myVal = prepVal(getVal(zOffset),inEdit);
	char * offset = cIntToStr(myVal,1,'0',true);
	cursorOffset = edit.getCursorOffset(cursorOffset,strlen(offset));
	return createFieldStr(offset,0,cursorOffset);
}

S1_newPage Zone_Stream::onNameSelect() {return mp_zoneUsrSet;}

S1_newPage Zone_Stream::nameEdit() {return DataStream::onNameSelect();}

U2_byte Zone_Stream::getUserSetting(S1_ind item) const {
	U2_byte gotVal = 0;
	switch (item){
	case 0:// group
		gotVal = getGroup();
		break;
	case 1://WeatherSB
		gotVal = getWeatherSetback();
		break;							
	case 2://Auto Mode
		gotVal = getAutoMode();
		break;					
	case zManHeat - Z_SET_VALS+1:
		gotVal = getVal(zManHeat);
		break;	
	case zAutoFinalTmp - Z_SET_VALS+1:
		gotVal = getVal(zAutoFinalTmp);
		break;					
	case zAutoTimeConst - Z_SET_VALS+1:
		// saved as ln(val) * 31 + 0.5
		gotVal = epDi().getAutoConstant();
		break;						
	}
	return gotVal;
}

void Zone_Stream::setUserSetting(S1_ind item, S2_byte myVal) {
	switch (item){
	case 0:// group
		epDi().setGroup(myVal);
		break;
	case 1://WeatherSB
		epDi().setWeatherSetback(myVal);
		break;							
	case 2://Auto Mode
		setAutoMode(myVal);
		break;					
	case zManHeat - Z_SET_VALS+1:
		setVal(zManHeat, myVal);
		break;			
	case zAutoFinalTmp - Z_SET_VALS+1:
		setVal(zAutoFinalTmp, myVal);
		break;					
	case zAutoTimeConst - Z_SET_VALS+1:
		epDi().setAutoConstant(myVal);
		break;	
	}
}

char * Zone_Stream::usrFldStr(S1_ind item, bool inEdit) const {
	strcpy(fieldText,getUsrPrompts(item));
	if (getNoOfVals() > item) {
		U2_byte myVal = prepVal(getUserSetting(item),inEdit);
		cursorOffset = strlen(fieldText);
		addUsrValToFieldStr(item, myVal);
		cursorOffset = edit.getCursorOffset(cursorOffset,strlen(fieldText));
	}
	fieldCText[0] = cursorOffset + 1;
	fieldCText[1] = NEW_LINE_CHR;
	return fieldCText;
}

ValRange Zone_Stream::addUsrValToFieldStr(S1_ind item, S2_byte myVal) const {
	switch (item){
	case 0:// group
		strcat(fieldText,cIntToStr(myVal,1));
		return ValRange(1,0,3);
	case 1:// weather setback
		strcat(fieldText,cIntToStr(myVal,1));
		return ValRange(1,0,3);						
	case 2://Auto mode
		strcat(fieldText,myVal == 1?"Y":"N");
		return ValRange(0,0,1);				
	case zManHeat - Z_SET_VALS+1:
		strcat(fieldText,cIntToStr(myVal,3));
		return ValRange(3,0,255);					
	case zAutoFinalTmp - Z_SET_VALS+1:
		strcat(fieldText,cIntToStr(myVal,2));
		return ValRange(2,0,99);					
	case zAutoTimeConst - Z_SET_VALS+1:
		strcat(fieldText,cIntToStr(myVal,4));
		return ValRange(4,1,3736);					
	default:
		return ValRange(0);
	}
}

char * Zone_Stream::objectFldStr(S1_ind activeInd, bool inEdit) const {
	U1_count debug = index();
	U1_byte myVal = runI().getCurrProgTempRequest() + getVal(zOffset); // edits are saved to offset immediatly
	if (myVal < MIN_ON_TEMP) myVal = MIN_ON_TEMP;
	strcpy(fieldText,getName());
	strcat(fieldText," Req$");
	S1_ind cursorOffset = strlen(fieldText) + 1;
	strcat(fieldText,cIntToStr(myVal));

	strcat(fieldText," is:");
	strcat(fieldText,cIntToStr((runI().getFractionalCallSensTemp()+128)/256));
	strcat(fieldText, runI().isHeating() ? "!":" ");

	return formatFieldStr(NEW_LINE_CHR, cursorOffset);
}

char * Zone_Stream::remoteFldStr(S1_ind activeInd, bool inEdit) const {
	U1_count debug = index();
	U1_byte myVal = runI().getCurrProgTempRequest() + getVal(zOffset); // edits are saved to offset immediatly
	strcpy(fieldText,"Request:");
	S1_ind cursorOffset = strlen(fieldText) + 1;
	strcat(fieldText,cIntToStr(myVal));

	strcat(fieldText," is:");
	strcat(fieldText,cIntToStr((runI().getFractionalCallSensTemp()+128)/256));

	return formatFieldStr(NEW_LINE_CHR, cursorOffset);
}

char * Zone_Stream::dtFromFldStr(S1_ind item, bool inEdit, char newLine) const {
	setZdata();
	DTtype fromDate;
	cursorOffset = 0;
	if (inEdit) {
		fromDate = editFromDate.get4Val();
	} else {
		fromDate = currZdtData.fromDate;
	}
	DateTime_Stream::getTimeDateStr(fromDate);
	cursorOffset = edit.getCursorOffset(cursorOffset,0);
	return formatFieldStr(newLine,cursorOffset);
}

char * Zone_Stream::dtToFldStr(S1_ind item, bool inEdit, char newLine) const {
	DTtype toDate;
	cursorOffset = 0;
	if (inEdit) toDate = editToDate.get4Val();
	else {
		toDate = currZdtData.expiryDate;
	}
	DateTime_Stream::getTimeDateStr(toDate);
	cursorOffset = edit.getCursorOffset(cursorOffset,0);
	return formatFieldStr(newLine,cursorOffset);
}

char * Zone_Stream::progFldStr(S1_ind item, bool inEdit, char newLine) const {
	setZdata();
	cursorOffset = 0;
	U1_ind displDPindex;
	if (inEdit) displDPindex = editProg.getVal();
	else displDPindex = currZdtData.currDP;
	strcpy(fieldText,f->dailyProgS(displDPindex).getName());
	strcat(fieldText,cIntToStr(f->dailyProgS(displDPindex).refCount()));
	cursorOffset = edit.getCursorOffset(cursorOffset,strlen(fieldText));
	return formatFieldStr(newLine,cursorOffset);
}

U1_ind Zone_Stream::sameDP(U1_ind currDP, S1_byte move) const {
	S2_byte theLastDP = currZdtData.firstDP + lastDP();
	U1_count refCount = f->dailyProgS(currDP).refCount(); // check refcount because if we have a refcount !=0, we aren't interested in spare DP's of the same name. But if we are looking at an unused program, we want to see all its parts
	char thisName[DLYPRG_N+1];
	strcpy(thisName,f->dailyProgS(currDP).getName());
	do 
		currDP = nextIndex(currZdtData.firstDP,currDP,theLastDP,move);
	while (strcmp(f->dailyProgS(currDP).getName(),thisName) != 0 || f->dailyProgS(currDP).refCount() != refCount);
	return currDP;
}


U1_ind Zone_Stream::nextDP(U1_ind currDP, S1_byte move) const { // returns actual index of next uniquly named DP
	S2_byte theLastDP = currZdtData.firstDP + lastDP();
	S1_byte moveSign = (move<0) ? -1 : 1;
	while (move != 0) {
		while (!isFirstNamedDP(currDP)){ // required if moving backwards and don't have FirstNamedDP 
			currDP = nextIndex(currZdtData.firstDP,currDP,theLastDP,S1_byte(-1));
		}
		do 
			currDP = nextIndex(currZdtData.firstDP,currDP,theLastDP,moveSign);
		while (!isFirstNamedDP(currDP));
		move -= moveSign;
	}
	return currDP;
}

bool Zone_Stream::isFirstNamedDP(S1_ind thisDP) const {
	// if DP is not the first non-spare with this name, move to next
	U1_ind i(getVal(E_firstDP));
	char thisName[DLYPRG_N+1];
	strcpy(thisName,f->dailyProgS(thisDP).getName());

	for (;i < thisDP; i++) {
		if (strcmp(f->dailyProgS(i).getName(),thisName) == 0) //  && !f->dailyProgS(i).refCount() == 0)
			return false;
	}
	return true;
}

U1_byte Zone_Stream::lastDT() const {
	return runI().getNthDT(currZdtData, 127) -1;
}

U1_byte Zone_Stream::lastDP() const {
	return getVal(E_NoOfDPs)-1;
}

S1_newPage Zone_Stream::onItemSelect(S1_byte myListPos, D_Data & d_data){
	// edit setup vals on up/down
	edit.setRange(addValToFieldStr(myListPos), getVal(myListPos));
	return DO_EDIT;
}

S1_newPage Zone_Stream::startEdit(D_Data & d_data){
	// edit zone temps on up/down
	editTempReq.setRange(ValRange(0,-9,9),0,this);
	return DO_EDIT;
}

S1_newPage Zone_Stream::onSelect(D_Data & d_data){
	// switch to sequence page on zone-temps select
	return mp_diary;
}

void Zone_Stream::save(){
	// edit zone temps on up/down, saved on each edit.
}

void Zone_Stream::saveItem(S1_ind item){ // setup items
	setVal(item, edit.getVal());
}

S1_newPage Zone_Stream::onUsrSelect(S1_byte myListPos, D_Data & d_data){
	//if (myListPos > 2 && getAutoMode()) 
	//	return 0; // returns DO_EDIT for edit, +ve for change page, 0 do nothing;
	edit.setRange(addUsrValToFieldStr(myListPos), getUserSetting(myListPos));
	return DO_EDIT;
}

void Zone_Stream::saveUsr(S1_ind item){
	setUserSetting(item, edit.get2Val());
}

S1_newPage Zone_Stream::abbrevSelect(){
	editChar.setRange(ValRange(2), getAbbrev());
	return DO_EDIT;
}

void Zone_Stream::abbrevSave(){
	epDi().setAbbrev(editChar.getChars());
}

S1_newPage Zone_Stream::offsetSelect(){
	edit.setRange(ValRange(-1,-15,15), static_cast<S1_byte>(getVal(zOffset)));
	return DO_EDIT;
}

void Zone_Stream::offsetSave(){
	setVal(zOffset,edit.getVal());
}

S1_newPage Zone_Stream::fromSelect(){
	editFromDate.setRange(ValRange(15,0,255), this);
	return DO_EDIT;
}

void Zone_Stream::fromSave(){
	DTtype originalFromDate = currZdtData.fromDate; // dateTime before edit
	DTtype requestedFromDate = editToDate.get4Val();
	if (originalFromDate == requestedFromDate) return;
	
	U1_ind prevDT = runI().getNthDT(currZdtData,currZdtData.viewedDTposition-1);
	
	//check this DT type
	if (f->dateTimesS(prevDT).getDPtype() != E_dpWeekly) { // from set by end of prev special
		DTtype prevFrom = f->dateTimesS(prevDT).getDateTime();
		DTtype newPeriod = DTtype::timeDiff(prevFrom,requestedFromDate);
		U1_byte prevDP = f->dateTimesS(prevDT).dpIndex();
		f->dailyPrograms[prevDP].setVal(E_period,newPeriod.U1());
	} else {
		if (originalFromDate <= currentTime(true)) {
			insertNewDT(-1);
		}
		f->dateTimesS(currZdtData.currDT).setDateTime(requestedFromDate);
	}
	refreshDTsequences();
	commit();
	setZdata(CHECK_FROM);
	//synchroniseZones(originalFromDate,currZdtData.expiryDate);
}

S1_newPage Zone_Stream::toSelect(){
	if (currZdtData.expiryDate == DTtype::JUDGEMEMT_DAY){
		#if defined DEBUG_INFO
			debugDTs(0,"\nPre-Insert\n");
		#endif		
		insertNewDT(1);
		#if defined DEBUG_INFO
			debugDTs(0,"Post-Insert");
			std::cout << debugDPs("Post-Insert");
		#endif
		refreshDTsequences();
		setZdata(CHECK_FROM);
	}
	editToDate.setRange(ValRange(15,0,255), this);
	return DO_EDIT;
}

void Zone_Stream::toSave(){
	// get displayed 'To' time/date = expiry date displayed.
	U1_byte thisFromDT = currZdtData.currDT;
	DTtype originalToDate = currZdtData.expiryDate; // dateTime before edit
	DTtype requestedToDate = editToDate.get4Val();
	if (originalToDate == requestedToDate) return;

	// get next DT and get its start-time/date.
	U1_byte dtToEdit = runI().getNthDT(currZdtData, currZdtData.viewedDTposition+1);
	DTtype nextStartDate = f->dateTimesS(dtToEdit).getDateTime();
	
	//check this DT type
	if (currZdtData.currDPtype == E_dpWeekly) { // fromDT is weekly, so edit next start time
		DTtype thisStartDate = f->dateTimesS(thisFromDT).getDateTime();
		if (thisStartDate == requestedToDate){// if nextStartDate == prev startDate then set prev startDate to 0.
			f->dateTimesS(thisFromDT).setDateTime(DTtype());
			f->dateTimesS(thisFromDT).save();
		} else f->dateTimesS(dtToEdit).setDateTime(requestedToDate); // set next start date to the toDate.
	} else { // adjust duration of current special DT
		dtToEdit = thisFromDT;
		U1_byte thisDP = f->dateTimesS(dtToEdit).dpIndex();
		DTtype newPeriod;
		if (requestedToDate <= currentTime(true))
			f->dateTimesS(dtToEdit).setDateTime(newPeriod); // recycle unwanted DT
		else {
			newPeriod = DTtype::timeDiff(f->dateTimesS(dtToEdit).getDateTime(),requestedToDate);
			if (newPeriod == DTtype())
				f->dateTimesS(dtToEdit).setDateTime(newPeriod);
			else { // shift start of current DT to now if required. Non-current DT's are prevented from exceeding allowable period.
				if (newPeriod > DTtype::MAX_PERIOD) { // not allowed unless startTime < currentTime
					f->dateTimesS(dtToEdit).setDateTime(currentTime());
					newPeriod = DTtype::MAX_PERIOD;
				}
				f->dailyPrograms[thisDP].setVal(E_period,newPeriod.U1());
			}
		}
	}
	f->dateTimesS(dtToEdit).save();
	refreshDTsequences();
	commit();
	setZdata(CHECK_FROM); // to reset after DT creation
	//
	//synchroniseZones(currZdtData.fromDate, originalToDate);
}

void Zone_Stream::synchroniseZones(DTtype originalFromDate, DTtype originalToDate) {
	S2_byte noOfZones = Factory::noOf(noZones) -1;
	S1_ind tryZone = nextIndex(0,S1_byte(index()),noOfZones,1);
	U1_ind thisIndex = index();
	U1_ind thisGroup = epDi().getGroup();
	while (tryZone != thisIndex) {
		if (f->zoneS(tryZone).epDi().getGroup() == thisGroup) {
			logToSD("Synchronise Zone: ", int(tryZone)); // << " DP: " << currName << '\n';
			S1_ind dtInd = f->zoneS(tryZone).hasMatchingDT(originalFromDate, originalToDate, currZdtData.currDP);
			if (dtInd != -1) {

			}
		}
		tryZone = nextIndex(0,tryZone,noOfZones,1);
	}
}

S1_ind Zone_Stream::hasMatchingDT(DTtype originalFromDate, DTtype originalToDate, S1_ind currDP) const {
	S1_byte position = 0;
	U1_byte thisDT = runI().findDT(originalFromDate, originalToDate);
	if (thisDT != 127) {
		std::string currDPname = f->dailyPrograms[currDP].epD().getName();
		U1_byte thisDP = f->dateTimesS(thisDT).dpIndex();
		std::string thisDPname = f->dailyPrograms[thisDP].epD().getName();
		if (thisDPname == currDPname) {
			#ifdef DEBUG_INFO
			std::cout << "Synchronise DP: " << thisDPname << '\n';
			#endif
			return thisDT;
		}	
	}
	return -1;
}

//S1_ind Zone_Stream::hasDPname(const std::string & name) const {
//	for (int i = getVal(E_firstDP); i < getVal(E_firstDP) + getVal(E_NoOfDPs); ++i) {
//		std::string thisName = f->dailyPrograms[i].epD().getName();
//		if (name == thisName) return i;
//	}
//	return -1;
//}

S1_newPage Zone_Stream::progEdit(){
	S1_byte theLastDP = currZdtData.firstDP + lastDP();
	editProg.setRange(ValRange(0,currZdtData.firstDP,theLastDP), currZdtData.currDP, this);
	return DO_EDIT;
}

S1_newPage Zone_Stream::progSave(){
	U1_ind toDPind(editProg.getVal());
	E_dpTypes toType = f->dailyProgS(toDPind).getDPtype();
	S1_newPage returnVal = 0;
	if (toType == E_dpNew) {
		currZdtData.prevDP = currZdtData.currDP;
		f->dateTimesS(currZdtData.currDT).setDPindex(toDPind);
		returnVal = mp_prog; // new prog page
	} else if (currZdtData.currDPtype == E_dpNew) {
		f->dateTimesS(currZdtData.currDT).setDPindex(toDPind);
	} else if (currZdtData.currDPtype == E_dpWeekly && toType == E_dpWeekly) { // Change one weekly to another - easy
		f->dateTimesS(currZdtData.currDT).setDPindex(toDPind);
	} else if (currZdtData.currDPtype != E_dpWeekly && toType != E_dpWeekly) { // Change one special to another
		// Get period of old DP
		U1_byte period = f->dailyPrograms[currZdtData.currDP].getVal(E_period);
		// Create new special DP
		U1_ind newDP = DP_EpMan::copyItem(index(),toDPind);
		// Set period of new DP
		f->dailyProgS(newDP).setVal(E_period,period);
		// Point to new DP
		f->dateTimesS(currZdtData.currDT).setDPindex(newDP);
	} else if (currZdtData.currDPtype != E_dpWeekly && toType == E_dpWeekly) { // Change special to weekly
		// Requires a new DT to end the period, pointing to the same DP as the next DT
		// Create new DT for end of period, copied from Next DT
		U1_ind nextDTind = runI().getNthDT(currZdtData,currZdtData.viewedDTposition + 1);
		U1_ind newDT = DT_EpMan::copyItem(index(),nextDTind);
		// Set start to end of special period
		DTtype toDate = f->dateTimesS(currZdtData.currDT).addDateTime(f->dailyProgS(currZdtData.currDP).getVal(E_period));
		f->dateTimesS(newDT).setDateTime(toDate);
		// point to new DP
		f->dateTimesS(currZdtData.currDT).setDPindex(toDPind);
	} else if (currZdtData.currDPtype == E_dpWeekly && toType != E_dpWeekly) {// Change weekly to special
			// Requires a new DT to start the period, pointing to a special DP
			// Obtain a new DT for start of period
			DTtype newDate = currZdtData.fromDate;
			DTtype currDTtime = f->dateTimesS(currZdtData.currDT).getDateTime();
			U1_ind currDT = currZdtData.currDT;
			U1_ind newDT = DT_EpMan::copyItem(index(),currDT);
			// Set start to end of previous period
			if (newDate.getDateTime() == currDTtime) {
				newDate = newDate.addDateTime(1);
			}
			f->dateTimesS(newDT).setDateTime(newDate);
			// create new special DP
			U1_ind newDP = DP_EpMan::copyItem(index(),toDPind);
			// Set period to match next DT if possible
			DTtype period = DTtype::timeDiff(currZdtData.fromDate,currZdtData.expiryDate);
			if (period > DTtype::MAX_PERIOD) period = DTtype::MAX_PERIOD;			
			
			f->dailyProgS(newDP).setVal(E_period,period.U1());
			// point to new DP
			f->dateTimesS(newDT).setDPindex(newDP);
	}
	commit();
	refreshDTsequences(); // force recycle of duplicate weekly DTs
	S1_byte dtPos = currZdtData.viewedDTposition;
	setZdata(CHECK_FROM); // to reset after DT creation
	currZdtData.viewedDTposition = dtPos;
	setZdata(CHECK_FROM); // To set DT pos

#if defined DEBUG_INFO
		debugDTs(index(),"ProgSave Commit");
		std::cout << debugDPs("");
#endif

	currZdtData.lastDT = currZdtData.firstDT + getVal(E_NoOfDTs);
	setZdata();
	return returnVal;
}

void Zone_Stream::userRequestTempChange(S1_byte increment) {
	S2_byte tempIs = runI().getFractionalCallSensTemp() >> 8;
	U1_byte tempReq = runI().modifiedCallTemp(runI().getCurrProgTempRequest()); // edits are saved to offset immediatly
	logToSD("Remote TempChange by : is : request : ",increment , tempIs, tempReq);
	if (tempReq + increment <= tempIs + 1) {
		setVal(zOffset,getVal(zOffset) + increment);
	}
};

void Zone_Stream::refreshDTsequences() {
	U1_ind noOfZones = Factory::noOf(noZones);
	for (U1_ind i = 0; i < noOfZones;++i) {
		f->zoneR(i).loadDTSequence();
	}
}

U1_ind Zone_Stream::insertNewDT(S1_inc move) {
	// Look for a weekly DP which is not the current, nor the previous.
	U1_ind curr_DP = currZdtData.currDP;
	U1_ind prev_DP = curr_DP;
	if (move == -1)	prev_DP = f->dateTimesS(runI().getNthDT(currZdtData, currZdtData.viewedDTposition-1)).dpIndex();
	U1_ind nextDT = runI().getNthDT(currZdtData, currZdtData.viewedDTposition+1);
	U1_ind newDP = curr_DP;
	if (nextDT < 127) newDP = f->dateTimesS(nextDT).dpIndex();
	U1_ind tryDP = newDP;
	while (f->dailyProgS(newDP).getDPtype() != E_dpWeekly
		|| newDP == prev_DP || newDP == curr_DP) {
		newDP = nextDP(newDP,1);
		if (newDP == tryDP) { // no weekly alternative found
			//newDP = nextDP(newDP,1);
			while (f->dailyProgS(newDP).getDPtype() != E_dpNew
				|| newDP == prev_DP || newDP == curr_DP) {
					newDP = nextDP(newDP,1);
					if (newDP == tryDP) {newDP = nextDP(newDP,1); break;}
			}
			break;
		}
	}

	U1_ind originalDT = currZdtData.currDT;
	DTtype newDate = currZdtData.fromDate;
	U1_ind zoneIndex = index();
int debug = originalDT;
	U1_ind newDT = DT_EpMan::copyItem(index(),originalDT);
if (debug != originalDT) { 
	debug = debug;
}
	// Need to get current display pos reset to what it was before create new item.
	S1_byte dtPos = currZdtData.viewedDTposition;
	setZdata(CHECK_FROM); // to reset after DT creation
	currZdtData.viewedDTposition = dtPos;
	setZdata(CHECK_FROM); // To set DT pos
	if (newDT == originalDT) {
		originalDT = findDT(newDate, curr_DP); // never happens
	}
	currZdtData.currDT = originalDT;

	f->dateTimesS(newDT).setDPindex(newDP);

	if (newDate < currentTime(true)) {
		newDate = currentTime();
	}	
	if (f->dateTimesS(originalDT).getDPtype() == E_dpWeekly) {
		newDate = newDate.addDateTime(1); // move original DT time on 10 minutes to insert new event;
		if (move < 0) {
			f->dateTimesS(originalDT).setDateTime(newDate);
		} else {
			f->dateTimesS(newDT).setDateTime(newDate);
		}
	} else {
		f->dateTimesS(newDT).setDateTime(newDate);
	}
	commit();
	return newDT;
}

S1_byte Zone_Stream::findDT(DTtype fromDate, S1_ind DPindex) const {
	int i = currZdtData.firstDT;
	while (i < currZdtData.lastDT
		&& f->dateTimesS(i).getDPtype() != E_dpNew
		&& f->dateTimesS(i).getDateTime() != fromDate
		&& f->dateTimesS(i).dpIndex() != DPindex) {
		++i;
	}
	if (i <currZdtData.lastDT) return i; else return 127;
}


void Zone_Stream::deleteDT(){
	f->dateTimesS(currZdtData.currDT).recycleDT();
	--currZdtData.viewedDTposition;
}

bool Zone_Stream::dtIsCurrWeekly(U1_ind dtInd) const {
	return (currZdtData.currDPtype == E_dpWeekly && currZdtData.fromDate <= currentTime(true));
}

void Zone_Stream::commit(){
	U1_ind item = getVal(E_firstDT);
	U1_ind end = item + getVal(E_NoOfDTs);
	for (;item < end; ++item) {
		U2_epAdrr loadAddr = f->dateTimesS(item).epD().eepromAddr();
		f->dateTimesS(item).saveDT(loadAddr);
	}
}

void Zone_Stream::load(){
	U1_ind item = getVal(E_firstDT);
	U1_ind end = item + getVal(E_NoOfDTs);
	for (;item < end; ++item) {
		U2_epAdrr loadAddr = f->dateTimesS(item).epD().eepromAddr();
		f->dateTimesS(item).loadDT(loadAddr);
	}
}

void EditTempReq::setRange(const ValRange & myvRange, S2_byte currVal, Zone_Stream * currZone) {
	Edit::setRange(myvRange,currVal);
	editPtr = &editTempReq;
	zone = currZone;
	timeInEdit = EDIT_TIME;
}

void EditTempReq::nextVal(S2_byte move) {
	Zone_Run::z_period = 1;
//	f->eventS().newEvent(EVT_TEMP_REQ_CHANGED,S1_byte(move));
	logToSD("Local EditTempReq::nextVal\tTemp Offset Changed\tEVT_TEMP_REQ_CHANGED",move);
	zone->setVal(zOffset,zone->getVal(zOffset) + move);
}

void EditTempReq::cancelEdit() {
	zone->setVal(zOffset,0);
}

#if defined (ZPSIM)
FactoryObjects * getFactory(FactoryObjects * fact);
std::string debugDPs(char* msg) {
	std::ostringstream stream;
	stream  << msg << '\n';
	FactoryObjects * f = getFactory();
	for (int z = 0; z<4;++z) {
		stream  << std::dec << "Zone[" << (int)z << "] DP's: " << (int)f->zones[z].getVal(E_NoOfDPs) << '\n';
		for (int i = f->zones[z].getVal(E_firstDP);i<f->zones[z].getVal(E_NoOfDPs) + f->zones[z].getVal(E_firstDP);++i) {
			stream  << "DP[" << std::setw(2) << int(i) << "] " << std::setw(10) << f->dailyProgS(i).getName()
			<< " FirstTT: " << std::setw(3) << (int)f->dailyPrograms[i].getVal(E_firstTT)
			<< " NoOfTTs: " << (int)f->dailyPrograms[i].getVal(E_noOfTTs) << " RefCount: " << (int)f->dailyProgS(i).refCount()
			<< " Type:" << (int)f->dailyProgS(i).getDPtype() << " days:" << (int)f->dailyProgS(i).getDays() << '\n';
		}
	}
	return stream.str();
}

std::string debugCurrTime() {
	std::ostringstream stream;
	stream  << "CurrTime: " << (int)currentTime().getHrs() << ":" << (int)currentTime().get10Mins() << "0 " << (int)currentTime().getDay() << "/" << (int)currentTime().getMnth() << "/" << (int)currentTime().getYr(); // << " \n";
	return stream.str();
}

#endif