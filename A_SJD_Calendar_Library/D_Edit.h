#pragma once
#include "A__Constants.h"

class Zone_Stream;

// formatting struct, mostly used for creation of temporary objects
// used as argument to Edit class. Edit holds instance of a ValRange.
// editWidth is max alphachars or the minimum displayed numerical characters, excluding sign and dec point. 
// Set editWidth = 0 to prevent multi-position edit.
// for string edit only set editWidth. For leading + negate width.
// cursorOffset = -1  puts cursor at end.
struct ValRange {
	ValRange(S1_byte editWidth, S1_byte minVal = 0, S2_byte maxVal = 0, S1_byte noOfDecPlaces = 0);
	ValRange(){}
	U1_byte editWidth;
	S1_byte	minVal;
	S2_byte maxVal; // needs to register 256 as well as 0.
	S1_byte cursorOffset;
	S1_byte noOfDecPlaces;
	S1_byte currWidth; // number of digits, excluding sign. -ve for show '+' 
};

class Edit
{
public:
	Edit();
	virtual void setRange(const ValRange & vRange, S2_byte currVal);
	virtual void nextVal(S2_byte move);
	virtual bool nextCursorPos(S1_inc move); // return true if moving out of field
	S1_byte getVal() {return S1_byte(val);}
	S2_byte get2Val() {return S2_byte(val);}
	S1_byte getCursorOffset(S1_byte preValOffset, S1_byte postValOffset);
	virtual void cancelEdit() {exitEdit();}
	void exitEdit();
	bool isNew() {return insertMode != 0;}
	static void checkTimeInEdit(S1_byte keyPress); 
	static S1_byte timeInEdit;
protected:
	S1_byte setCurrWidth(); // returns change in currWidth
	U1_byte minCursorOffset();
	U1_byte maxCursorOffset();
	static ValRange vRange;
	static S4_byte val;
	static char name[MAX_LINE_LENGTH+1];
	static Zone_Stream * zs;
	static S1_byte insertMode;

} extern edit;

class EditChar : public Edit {
public:	
	void setRange(const ValRange & vRange, const char* currName = 0);
	void nextVal(S2_byte move);
	virtual char * getChars() {return name;}
	bool nextCursorPos(S1_inc move);
private:
	char nextCharacter(char thisChar, int move);
} extern editChar;
