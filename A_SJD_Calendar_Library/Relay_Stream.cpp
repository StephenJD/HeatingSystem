#include "D_DataStream.h"
#include "A_Stream_Utilities.h"
#include "D_Factory.h"
using namespace Utils;

template<>
const char DataStreamT<EpDataT<RELAY_DEF>, Relay_Run, Relay_EpMan, RL_COUNT>::setupPrompts[RL_COUNT][13] = { // setup Items
		"Adr:",
		"Prt:",
		"Active:"
};

template<>
ValRange DataStreamT<EpDataT<RELAY_DEF>, Relay_Run, Relay_EpMan, RL_COUNT>::addValToFieldStr(S1_ind item, U1_byte myVal) const {
	switch (item) { // rDeviceAddr, rPortNo, rActiveState,
	case rDeviceAddr:
	case rPortNo:
		strcat(fieldText,cIntToStr(myVal,3,'0'));
		return ValRange(3,0,127);
	case rActiveState:
		strcat(fieldText,cIntToStr(myVal,1));
		return ValRange(1,0,1);
	default:
		return ValRange(0);
	}
}