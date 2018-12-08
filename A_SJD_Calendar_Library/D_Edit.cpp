#include "D_Edit.h"
#include "A_Stream_Utilities.h"
#include "Zone_Run.h"
#include "A_Displays_Data.h"

using namespace Utils;

Edit edit;
EditChar editChar;

ValRange Edit::vRange;
S4_byte Edit::val;
char Edit::name[MAX_LINE_LENGTH+1];
Zone_Stream * Edit::zs;
S1_byte Edit::insertMode;
S1_byte Edit::timeInEdit;

// editWidth is max alphachars or the minimum displayed numerical characters, excluding sign and dec point. 
// Set editWidth = 0 to prevent multi-position edit.
// for string edit only set editWidth. For leading + negate width.
ValRange::ValRange(S1_byte myEditWidth, S1_byte minVal, S2_byte maxVal, S1_byte noOfDecPlaces) 
	: editWidth(abs(myEditWidth)),
	minVal(minVal),  
	maxVal(maxVal),
	noOfDecPlaces(noOfDecPlaces),
	currWidth(myEditWidth) {
	if (maxVal == minVal) cursorOffset = 0; // multi-edit a string from left
	else cursorOffset = -1; // put cursor on last character
}

Edit::Edit(){timeInEdit = 10;}

////////////////////// setRange specialisations ///////////////////
void Edit::checkTimeInEdit(S1_byte keyPress) {
	if (timeInEdit > 0) {
		if (keyPress >= 0) timeInEdit = EDIT_TIME;
		--timeInEdit;
		if (timeInEdit == 0) {
			D_Data::displayInEdit = false;
			vRange.cursorOffset = 0;
		}
	}
}

void Edit::setRange(const ValRange & myvRange, S2_byte currVal){
	val = currVal;
	vRange = myvRange;
	setCurrWidth();
	editPtr = &edit;
	insertMode = 0;
	timeInEdit = EDIT_TIME;
}

void EditChar::setRange(const ValRange & myvRange, const char* currName){
	vRange = myvRange;
	strcpy(name,currName);
	U1_byte blank = strlen(name);
	for (U1_byte i(0); i < blank; ++i) 
		if (name[i] == ' ') name[i] = EDIT_SPC_CHAR;
	while (blank < vRange.editWidth) name[blank++] = EDIT_SPC_CHAR;
	name[blank] = 0;
	setCurrWidth();
	editPtr = &editChar;
	timeInEdit = EDIT_TIME;
}

//////////////////////// nextVal specialisations /////////////////

void Edit::nextVal(S2_byte move){
	if (vRange.editWidth!=0) {
		S1_byte mult = maxCursorOffset() - vRange.cursorOffset; 
		while (mult > 0){
			move = move * 10;
			--mult;
			if (mult == vRange.noOfDecPlaces) --mult;
		} 
	}
	val = nextIndex(vRange.minVal,get2Val(),vRange.maxVal,move);
	// move cursor to new unit position if we have gained/lost a digit
	vRange.cursorOffset = vRange.cursorOffset + setCurrWidth();
}

void EditChar::nextVal(S2_byte move){// edit a character
 	name[vRange.cursorOffset] = nextCharacter(name[vRange.cursorOffset],move);
}

///////////////////////// nextCursorPos specialisations ////////////////////

bool Edit::nextCursorPos(S1_inc move){
	if (vRange.editWidth > 0) {
		bool shortField = (vRange.editWidth < 4);
		U1_byte minCsr = minCursorOffset();
		U1_byte maxCsr = maxCursorOffset();
		bool moveRightOutOfField = (vRange.cursorOffset + move > maxCsr);
		bool moveLeftOutOfField = (vRange.cursorOffset + move < minCsr);
		if (shortField && (moveRightOutOfField || moveLeftOutOfField)) return true;
		do 
			vRange.cursorOffset = nextIndex(minCsr,vRange.cursorOffset,maxCsr-minCsr+1,move);
		while (vRange.cursorOffset == abs(vRange.currWidth) + minCsr - vRange.noOfDecPlaces);
		return false;
	}
	return true;
}

bool EditChar::nextCursorPos(S1_inc move){
	 vRange.cursorOffset = nextIndex(0,vRange.cursorOffset,vRange.editWidth-1,move);
	return false;
}

//////////////////////// edit methods /////////////////

S1_byte Edit::getCursorOffset(S1_byte preValOffset, S1_byte postValOffset) {
	if (vRange.cursorOffset == -1) return postValOffset -1;
	else{
		setCurrWidth();
		return preValOffset + vRange.cursorOffset;
	}
}

S1_byte Edit::setCurrWidth(){
	S1_byte newWidth = abs(vRange.currWidth);
	S1_byte oldWidth = newWidth;
	if (oldWidth > 0 && vRange.maxVal> vRange.minVal) {
		S1_byte showPlus = (vRange.currWidth < 0);
		if (abs(val) < 10) newWidth = 1;
		else if (abs(val) < 100) newWidth = 2;
		else newWidth = 3;
		if (newWidth < vRange.noOfDecPlaces + 1) newWidth = vRange.noOfDecPlaces + 1;
		if (newWidth < vRange.editWidth) newWidth = vRange.editWidth;
		vRange.currWidth = showPlus ? -newWidth : newWidth;
		if (vRange.cursorOffset == -1 || (vRange.cursorOffset > maxCursorOffset())) vRange.cursorOffset = maxCursorOffset();
	}
	S1_byte widthChange = newWidth - oldWidth;
	U1_byte minCsr = minCursorOffset();
	if (vRange.cursorOffset + widthChange < minCsr) widthChange = 0;
	return widthChange;
}

void Edit::exitEdit() {
	for (U1_byte i(0); i < vRange.editWidth; ++i) 
		if (name[i] == EDIT_SPC_CHAR) name[i] = ' ';
	vRange.cursorOffset = 0;
	insertMode = 0;
}

////////////////// helper methods ////////////////////

char EditChar::nextCharacter(char thisChar, int move){
	static int lastcursorOffset(0);
	char result;
	if (move > 0 && (thisChar >= 'a' && thisChar <= 'z' )) // change to upper case
		result= thisChar - ('a'-'A');
	else if (move < 0 && (thisChar >= 'A' && thisChar <= 'Z' )) // change to lower case
		result= thisChar + ('a'-'A');
	else if (thisChar == '|' && vRange.cursorOffset != lastcursorOffset) { // start with previous character when just moved to the right
		lastcursorOffset = vRange.cursorOffset;
		if (vRange.cursorOffset > 0) {
			result = name[vRange.cursorOffset-1];
			if (result == '|') result = ' ';
		} else result = ' ';		
	} else result = thisChar + move;

	// | A-Z-/0-9
	switch (result){
	case 31:
	case ':':
		result = '|';
		break;
	case ',':
		result = 'z';
		break;
	case '[':
		result = '-';
		break;
	case '.':
		if (move > 0) result = '/'; else result = '-';
		break;	
	case '{':
		if (move > 0) result = '-'; else result = '9';
		break;
	case '}':
	case 96:
	case '@':
		result = ' ';
		break;
	case '!':
		result = 'A';
	}
	return result;
}

U1_byte Edit::minCursorOffset(){return (vRange.currWidth < 0) || val < 0;}

U1_byte Edit::maxCursorOffset(){return abs(vRange.currWidth) + (vRange.noOfDecPlaces > 0) + minCursorOffset() -1;}
