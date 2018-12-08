#include "MixValve_Stream.h"
#include "D_DataStream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "MixValve_Run.h"
#include "TempSens_Run.h"

using namespace Utils;

template<>
const char DataStreamT<EpDataT<MIXV_DEF>, MixValve_Run, MixValve_EpMan, MV_COUNT>::setupPrompts[MV_COUNT][13] = { // setup Items
	"Store Snsr:"};

template<>
ValRange DataStreamT<EpDataT<MIXV_DEF>, MixValve_Run, MixValve_EpMan, MV_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	switch (item) { // mvFlowSensID, mvStoreSensor
	case mvStoreSensor: // TempSensors
		strcat(fieldText,f->tempSensors[myVal].dStream().getName());
		return ValRange(0,0,f->tempSensors.last());
	default:
		return ValRange(0);
	}
}

MixValve_Stream::MixValve_Stream(MixValve_Run & run) : DataStreamT<EpDataT<MIXV_DEF>, MixValve_Run, MixValve_EpMan, MV_COUNT> (run){}

char * MixValve_Stream::objectFldStr(S1_ind activeInd, bool inEdit) const {
	// C39_F36_H140_R25_M15 = CallTemp, FlowTemp, Heat/Cool/Off+valvePos, Ratio, Motor/Wait+seconds
	U1_ind mvInd = epD().index();
	strcpy(fieldText, mvInd == 0 ? "D" : "U");
	strcat(fieldText, cIntToStr(f->mixValveR(mvInd).readFromValve(Mix_Valve::flow_temp),2,'0'));
	strcat(fieldText, " C");
	strcat(fieldText, cIntToStr(f->mixValveR(mvInd)._mixCallTemp,2,'0'));
	S1_byte state = f->mixValveR(mvInd).readFromValve(Mix_Valve::state);
	if (state == 0) strcat(fieldText, " V");
	else if (state < 0) strcat(fieldText, " C");
	else strcat(fieldText, " H");
	strcat(fieldText, cIntToStr(f->mixValveR(mvInd).readFromValve(Mix_Valve::valve_pos),2,'0'));
	strcat(fieldText, " R");
	strcat(fieldText, cIntToStr(f->mixValveR(mvInd).readFromValve(Mix_Valve::ratio),2,'0'));
	
	Mix_Valve::Mode mode = Mix_Valve::Mode(f->mixValveR(mvInd).readFromValve(Mix_Valve::mode));
	switch (mode) {
	case Mix_Valve::e_Checking: 
		if (state == -2) strcat(fieldText, " C'l");
		else strcat(fieldText, " OFF");
		break;
	case Mix_Valve::e_Wait:
		strcat(fieldText, " W");
		strcat(fieldText, cIntToStr(abs(state),2,'0'));
		break;
	case Mix_Valve::e_Moving:
		strcat(fieldText, " M");
		strcat(fieldText, cIntToStr(abs(state),2,'0'));
		break;
	case  Mix_Valve::e_Mutex:
		strcat(fieldText, " X");
		break;
	}
	fieldCText[0] = cursorOffset + 1;
	fieldCText[1] = 0;
	return fieldCText;
}