#include "TempSens_Stream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "TempSens_Run.h"
#include "D_Edit.h"

using namespace Utils;

template<>
const char DataStreamT<EpDataT<TEMPS_DEF>, TempSens_Run, TempSens_EpMan, TS_COUNT>::setupPrompts[TS_COUNT][13] = {};

TempSens_Stream::TempSens_Stream(TempSens_Run & run) 
	: DataStreamT<EpDataT<TEMPS_DEF>, TempSens_Run, TempSens_EpMan, TS_COUNT>(run)
	{}

template<>
ValRange DataStreamT<EpDataT<TEMPS_DEF>, TempSens_Run, TempSens_EpMan, TS_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	return DataStream::addValToFieldStr(item, myVal);
}

bool TempSens_Stream::showAddress = false;

char * TempSens_Stream::objectFldStr(S1_ind activeInd, bool inEdit) const {
	if (inEdit) {
		if (showAddress) strcpy(fieldText, cIntToStr(edit.getVal(),3,'0'));
		else strcpy(fieldText, editChar.getChars());
	} else {
		if (activeInd == index() ? showAddress : false) strcpy(fieldText, cIntToStr(getVal(0),3,'0'));
		else strcpy(fieldText, getName());
	}
	cursorOffset = edit.getCursorOffset(0,0);

	strcat (fieldText," ");
	U1_byte temperature = runI().getSensTemp();
	if (temp_sense_hasError == true) strcat (fieldText, "Er");
	else strcat (fieldText,cIntToStr(temperature,2,'0'));
	
	fieldCText[0] = cursorOffset + 1;
	fieldCText[1] = 0;
	return fieldCText;
}

bool TempSens_Stream::editOnUpDown() const {
	showAddress = !showAddress;
	return false;
}

S1_newPage TempSens_Stream::startEdit(D_Data & d_data){
	if (showAddress) {
		edit.setRange(ValRange(3,0,127),getVal(0));
	} else {
		editChar.setRange(ValRange(4),getName());
	}
	return DO_EDIT;
}

void TempSens_Stream::save(){
	if (showAddress) setVal(0,edit.getVal());
	else setName(editChar.getChars());
}