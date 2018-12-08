#include "TowelR_Stream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "TowelR_Run.h"
#include "Zone_Run.h"
#include "D_Operations.h"

using namespace Utils;

TowelR_Stream::TowelR_Stream(Zone_Run & run) : Zone_Stream(run) {}

const char TowelR_Stream::setupPrompts[TR_COUNT][13] = {
	"CallRelay:",
	"CallTempS:",
	"FlowTempS:",
	"Max FlowT:",
	"Mix Valve:",
	"Run-time:"};

ValRange TowelR_Stream::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	if (item == tr_RunTime) item = zOffset;
	return Zone_Stream::addValToFieldStr(item,myVal);
}

char * TowelR_Stream::objectFldStr(S1_ind activeInd, bool inEdit) const {
	U1_ind index = epD().index();
	TowelR_Run & twlR_Run = static_cast<TowelR_Run &>(f->towelRads[index].run());
	switch (index) {
	case 0: strcpy(fieldText, "Fm:"); break;
	case 1: strcpy(fieldText, "En:"); break;
	case 2: strcpy(fieldText, "Ft:"); break;
	}
	strcat(fieldText, cIntToStr(twlR_Run.getCallSensTemp(),2,'0'));
	strcat(fieldText, " Call:");
	strcat(fieldText, cIntToStr(twlR_Run._callFlowTemp,2,'0'));
	strcat(fieldText, " Tm:");
	strcat(fieldText, cIntToStr(twlR_Run._timer/10,3,'0'));
	fieldCText[0] = cursorOffset + 1;
	fieldCText[1] = 0;
	return fieldCText;
}