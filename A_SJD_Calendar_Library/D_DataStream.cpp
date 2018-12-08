#include "D_DataStream.h"
#include "A_Stream_Utilities.h"
#include "D_EpDat_A.h"
#include "D_Operations.h"
#include "D_Edit.h"

using namespace Utils;

////////////////////////////// Format /////////////////////////////////////

char Format::fieldCText[DISPLAY_BUFFER_LENGTH];
char Format::postText[MAX_LINE_LENGTH +4];
char * Format::fieldText = (fieldCText + 2);
U1_ind Format::cursorOffset = 0;


char * Format::createFieldStr(char * fldStr, char newLine, S1_ind cursorOffset) { // used to format simple return strings from EEPROM
	strcpy(fieldText,fldStr);
	return formatFieldStr(newLine, cursorOffset);
}

char * Format::formatFieldStr(char newLine, S1_ind cursorOffset) { // used to format simple return strings from EEPROM
	U1_ind tempLen = strlen(fieldText);
	fieldText[tempLen] = SPACE_CHAR;
	fieldText[tempLen+1] = 0;
	if (cursorOffset == -1) cursorOffset = tempLen;
	fieldCText[0] = cursorOffset + 1;
	fieldCText[1] = newLine;
	return fieldCText;
}

U1_byte Format::prepVal(U1_byte currentVal, bool inEdit) {
	cursorOffset = 0;
	if (inEdit) return edit.getVal();
	else return currentVal;
}

U2_byte Format::prepVal(U2_byte currentVal, bool inEdit) {
	cursorOffset = 0;
	if (inEdit) return edit.get2Val();
	else return currentVal;
}
////////////////////////////// DataStream /////////////////////////////////////

FactoryObjects * DataStream::f;

DataStream::DataStream(Run & runRef) : runRef(runRef) {}

char * DataStream::nameFldStr(bool inEdit, char newLine) const {
	char * name;
	if (inEdit) name = editChar.getChars();
	else name = epD().getName();
	return createFieldStr(name, newLine, edit.getCursorOffset(0,0));
}

EpData & DataStream::epD() const {
	return runRef.epD();
}

//EpManager & DataStream::epM() const {return epManager;}

char * DataStream::itemFldStr(S1_ind item, bool inEdit) const {
	cursorOffset = 0; // required
	strcpy(fieldText,getSetupPrompts(item));
	if (epD().getNoOfVals() > item) {
		U1_byte myVal = prepVal(epD().getVal(item),inEdit);
		cursorOffset = strlen(fieldText); // required
		addValToFieldStr(item, myVal);
		cursorOffset = edit.getCursorOffset(cursorOffset,strlen(fieldText));
	}
	fieldCText[0] = cursorOffset + 1;
	fieldCText[1] = 0;
	return fieldCText;
}

ValRange DataStream::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	strcat(fieldText,cIntToStr(myVal,2,'0'));
	return ValRange(2,0,99);
}

U1_ind DataStream::index() const {return epD().index();}

void DataStream::setName(const char * name) {epD().setName(name);}

char * DataStream::getName() const {return epD().getName();}

void DataStream::setVal(U1_ind vIndex, U1_byte val) {epD().setVal(vIndex, val);}

U1_byte DataStream::getVal(U1_ind vIndex) const {return epD().getVal(vIndex);}

const U1_byte DataStream::getNoOfVals() const {return epD().getNoOfVals();}

const U1_byte DataStream::getNoOfStreamItems() const {return epD().getNoOfItems();}

S1_newPage DataStream::onItemSelect(S1_byte myListPos, D_Data & d_data){
	// edit setup vals on up/down
	U1_byte myVal = getVal(myListPos);
	edit.setRange(addValToFieldStr(myListPos),myVal);
	return DO_EDIT;
}

S1_newPage DataStream::onNameSelect() {// returns DO_EDIT for edit, +ve for change page, 0 do nothing
	U1_byte nameWidth = epD().getNameSize();
	editChar.setRange(ValRange(nameWidth),getName());
	return DO_EDIT;
}

void DataStream::saveItem(S1_ind item){
	setVal(item, edit.getVal());
}

void DataStream::saveName(){
	setName(editChar.getChars());
}

void DataStream::cancel() const {
} 
//////////////////////////// Test object /////////////////
