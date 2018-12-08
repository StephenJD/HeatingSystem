#include "D_DataStream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
#include "OccasHtr_Run.h"

using namespace Utils;

template<>
const char DataStreamT<EpDataT<OCCHTR_DEF>, OccasHtr_Run, OccasHtr_EpMan, OH_COUNT>::setupPrompts[OH_COUNT][13] = {
	"FlowTS:",
	"ThrmStrTS:",
	"MinTempDiff:",
	"MinFlowTemp:",
	"OHRelay:"};

template<>
ValRange DataStreamT<EpDataT<OCCHTR_DEF>, OccasHtr_Run, OccasHtr_EpMan, OH_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	switch (item) {
	case FlowTS:
	case ThrmStrTS:// TempSensors
		strcat(fieldText,f->tempSensors[myVal].dStream().getName());
		return ValRange(0,0,f->tempSensors.last());
	case OHRelay:// Relays
		strcat(fieldText,f->relays[myVal].dStream().getName());
		return ValRange(0,0,f->relays.last());
	case MinTempDiff:
	case MinFlowTemp:
		strcat(fieldText,cIntToStr(myVal,2,'0'));
		return ValRange(2,0,80);
	default:
		return ValRange(0);
	}
}